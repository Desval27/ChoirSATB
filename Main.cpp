#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/Tables.h>
#include <Music/Gnome.h>

#include "App.h"

using namespace daisysp;
using namespace daisy;
using MyDisplay = OledDisplay<SSD130xI2c128x64Driver>;

////////////////////////////////////////////////////////////////////////////////
// Hardware Configuration
////////////////////////////////////////////////////////////////////////////////
constexpr uint32_t DISPLAY_REFRESH_MS = 1000 / 10; // ~30 FPS
constexpr uint32_t VOICE_REFRESH_MS   = 1000 / 5;  // ~5 FPS
constexpr uint32_t BEAT_FLASH_MS      = 1000 / 20;

////////////////////////////////////////////////////////////////////////////////
// Parameters & Defaults
////////////////////////////////////////////////////////////////////////////////
constexpr int BARS        = 8;
constexpr int MAX_EVENTS  = 128;
const int     kDefaultBPM = 60;

DaisySeed    hw;
Encoder      encoder;
MyDisplay    display;
UiEventQueue eventQueue;
Metro        clock;
Switch       runStopBtn;
Switch       randomBtn;
Switch       priorBtn;
Switch       nextBtn;
// dsy_gpio           gate_output;
ReverbSc verb;

// A4 = 440 Hz in 12-EDO where C is degree 0 and A is degree 9.
TheApp app(hw, BARS);

AnalogControl pots[app.NUM_VOICES];

enum ButtonIds
{
    BUTTON_OK = 0,
    BUTTON_CANCEL,
};

enum PotIds
{
    POT_VALUE = 0,
};

enum EncoderIds
{
    ENCODER_MENU = 0,
};

////////////////////////////////////////////////////////////////////////////////
// Runtime status
////////////////////////////////////////////////////////////////////////////////
bool beatFlash    = false;
bool beatExpireMS = 0;

MappedIntValue   bpm(1, 300, kDefaultBPM, 1, 10, "bpm");
MappedFloatValue voiceAttack[app.NUM_VOICES] = {
    MappedFloatValue(0.1f, 1.0f, 0.1f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.1f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.1f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.1f, MappedFloatValue::Mapping::lin, "s", 1),
};
MappedFloatValue voiceDecay[app.NUM_VOICES] = {
    MappedFloatValue(0.1f, 1.0f, 0.4f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.4f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.4f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.4f, MappedFloatValue::Mapping::lin, "s", 1),
};
MappedFloatValue voiceSustain[app.NUM_VOICES] = {
    MappedFloatValue(0.0f, 1.0f, 0.4f, MappedFloatValue::Mapping::lin, "%", 1),
    MappedFloatValue(0.0f, 1.0f, 0.7f, MappedFloatValue::Mapping::lin, "%", 1),
    MappedFloatValue(0.0f, 1.0f, 0.7f, MappedFloatValue::Mapping::lin, "%", 1),
    MappedFloatValue(0.0f, 1.0f, 0.4f, MappedFloatValue::Mapping::lin, "%", 1),
};
MappedFloatValue voiceRelease[app.NUM_VOICES] = {
    MappedFloatValue(0.1f, 1.0f, 0.2f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.2f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.2f, MappedFloatValue::Mapping::lin, "s", 1),
    MappedFloatValue(0.1f, 1.0f, 0.2f, MappedFloatValue::Mapping::lin, "s", 1),
};
MappedFloatValue voiceVolumes[app.NUM_VOICES] = {
    MappedFloatValue(1.0f,
                     100.0f,
                     100.0f,
                     MappedFloatValue::Mapping::log,
                     "%",
                     0),
    MappedFloatValue(1.0f,
                     100.0f,
                     100.0f,
                     MappedFloatValue::Mapping::log,
                     "%",
                     0),
    MappedFloatValue(1.0f,
                     100.0f,
                     100.0f,
                     MappedFloatValue::Mapping::log,
                     "%",
                     0),
    MappedFloatValue(1.0f,
                     100.0f,
                     100.0f,
                     MappedFloatValue::Mapping::log,
                     "%",
                     0),
};

extern UI   ui;
extern bool uiOpen;
extern void InitMenus();
extern void OpenMainMenu();


////////////////////////////////////////////////////////////////////////////////
// Canvas Callbacks for UI
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
static void ClearCanvas(const UiCanvasDescriptor& canvas)
{
    auto* d = static_cast<MyDisplay*>(canvas.handle_);
    d->Fill(false);
}

////////////////////////////////////////////////////////////////////////////////
static void FlushCanvas(const UiCanvasDescriptor& canvas)
{
    auto* d = static_cast<MyDisplay*>(canvas.handle_);
    d->Update();
}

////////////////////////////////////////////////////////////////////////////////
void ResetState()
{
    bpm = kDefaultBPM;

    for(int i = 0; i < app.NUM_VOICES; i++)
    {
        voiceVolumes[i] = 100.0f;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Converts our current BPM to a clock frequency based on pulses per quarter note.
inline float BPMToClockFreq(int bpm)
{ return ((float)bpm * PPQN) / 60.0f; }

////////////////////////////////////////////////////////////////////////////////
void SetBPM(int bpm)
{
    float freq = BPMToClockFreq(bpm);
    clock.SetFreq(freq);
}

////////////////////////////////////////////////////////////////////////////////
// Main Audio Loop
////////////////////////////////////////////////////////////////////////////////
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    static int lastPulse = -1;

    //Prepare the audio block
    for(size_t i = 0; i < size; i += 2)
    {
        // New pulse
        if(app.GetRunning())
        {
            if(clock.Process())
            {
                const int pulse = app.gnome.DoPulse();

                // Sequence loop boundary: previous cycle ended and pulse wrapped.
                if(lastPulse >= 0 && pulse < lastPulse)
                {
                    app.MakeEvents();
                }
                lastPulse = pulse;

                // Is this a beat boundary?
                if(app.gnome.RisingBeatEdge())
                {
                    beatFlash    = true;
                    beatExpireMS = System::GetNow() + BEAT_FLASH_MS;
                    // dsy_gpio_write(&gate_output, true);
                }
                else
                {
                    // dsy_gpio_write(&gate_output, false);
                }

                // Pass the pulse on to our voices
                app.DoPulse(pulse);
            }

            float sig      = 0.0f;
            float equalMix = 1.0f / (float)app.NUM_VOICES;
            for(int i = 0; i < app.NUM_VOICES; i++)
            {
                sig += (app.voices[i]->Process() * voiceVolumes[i] / 100.0f)
                       * equalMix;
            }

            // out[i]     = sig;
            // out[i + 1] = sig;

            verb.Process(sig, sig, &out[i], &out[i + 1]);
        }
        else
        {
            out[i]     = 0.0f;
            out[i + 1] = 0.0f;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
void InitDisplay()
{
    MyDisplay::Config disp_cfg;
    display.Init(disp_cfg);
    display.Fill(false);
    display.Update();
}

////////////////////////////////////////////////////////////////////////////////
void InitControls()
{
    // Still fixed at four voices/four pots.  TODO: make this more flexible if we want to expand beyond SATB.
    AdcChannelConfig adc_config[4];
    adc_config[0].InitSingle(seed::A7);
    adc_config[1].InitSingle(seed::A8);
    adc_config[2].InitSingle(seed::A9);
    adc_config[3].InitSingle(seed::A10);

    hw.adc.Init(adc_config, 4);
    hw.adc.Start();

    pots[0].Init(hw.adc.GetPtr(0), 1000);
    pots[1].Init(hw.adc.GetPtr(1), 1000);
    pots[2].Init(hw.adc.GetPtr(2), 1000);
    pots[3].Init(hw.adc.GetPtr(3), 1000);

    runStopBtn.Init(seed::D17, 1000);
    randomBtn.Init(seed::D18, 1000);
    priorBtn.Init(seed::D15, 1000);
    nextBtn.Init(seed::D21, 1000);

    /** Initialize our Encoder */
    encoder.Init(seed::D20, seed::D16, seed::D19);

    // gate_output.pin  = seed::D15;
    // gate_output.mode = DSY_GPIO_MODE_OUTPUT_PP;
    // gate_output.pull = DSY_GPIO_NOPULL;
    // dsy_gpio_init(&gate_output);
}

////////////////////////////////////////////////////////////////////////////////
void InitUI()
{
    UI::SpecialControlIds special;
    special.okBttnId      = BUTTON_OK;
    special.cancelBttnId  = BUTTON_CANCEL;
    special.valuePotId    = POT_VALUE;
    special.menuEncoderId = ENCODER_MENU;

    UiCanvasDescriptor oled_canvas;
    oled_canvas.id_                = 0;
    oled_canvas.handle_            = &display;
    oled_canvas.updateRateMs_      = 33; // ~30 FPS
    oled_canvas.screenSaverTimeOut = 5000;
    oled_canvas.clearFunction_     = ClearCanvas;
    oled_canvas.flushFunction_     = FlushCanvas;

    ui.Init(eventQueue, special, {oled_canvas}, 0);

    InitMenus();
}

////////////////////////////////////////////////////////////////////////////////
void ProcessControls()
{
    static bool        encoderActive       = false;
    static bool        potActive           = false;
    static float       lastPotVal          = 0.0f;
    static uint32_t    encoderPressStartMS = 0;
    constexpr uint32_t kEncoderLongPressMS = 700;

    runStopBtn.Debounce();
    randomBtn.Debounce();
    priorBtn.Debounce();
    nextBtn.Debounce();
    encoder.Debounce();

    const int32_t inc     = encoder.Increment();
    const bool    press   = encoder.RisingEdge();
    const bool    release = encoder.FallingEdge();
    const float   potVal  = pots[0].Process();

    if(priorBtn.RisingEdge())
    {
        app.SetScaleIndex(app.GetScaleIndex() + 1);
    }
    if(nextBtn.RisingEdge())
    {
        app.SetScaleIndex(app.GetScaleIndex() - 1);
    }

    if(uiOpen)
    {
        if(inc != 0)
        {
            eventQueue.AddEncoderTurned(ENCODER_MENU, inc, 20);
            if(!encoderActive)
            {
                eventQueue.AddEncoderActivityChanged(ENCODER_MENU, true);
                encoderActive = true;
            }
        }
        else if(encoderActive)
        {
            eventQueue.AddEncoderActivityChanged(ENCODER_MENU, false);
            encoderActive = false;
        }

        if(potVal != lastPotVal)
        {
            eventQueue.AddPotMoved(POT_VALUE, potVal);
            if(!potActive)
            {
                eventQueue.AddPotActivityChanged(POT_VALUE, true);
                potActive = true;
            }
        }
        else if(potActive)
        {
            eventQueue.AddPotActivityChanged(POT_VALUE, false);
            potActive = false;
        }

        if(press)
        {
            // One press, not a retrigger event
            eventQueue.AddButtonPressed(BUTTON_OK, 1, false);
        }

        if(release)
        {
            eventQueue.AddButtonReleased(BUTTON_OK);
        }

        if(randomBtn.RisingEdge())
            eventQueue.AddButtonPressed(BUTTON_CANCEL, 1, false);
        if(randomBtn.FallingEdge())
            eventQueue.AddButtonReleased(BUTTON_CANCEL);
    }
    else
    {
        for(int i = 0; i < app.NUM_VOICES; i++)
        {
            voiceVolumes[i].Set(pots[i].Process() * 100.0f);
        }

        if(runStopBtn.RisingEdge())
            app.SetRunning(!app.GetRunning());

        if(randomBtn.RisingEdge())
        {
            app.SetScaleIndex(randomRange(D12StartIndex, D12StartIndex + D12Count));
            app.gnome.Reset();
        }

        // No UI open then the encoder simply adjusts BPM.
        if(inc != 0)
        {
            bpm = bpm.Get() + inc;
        }

        if(press)
        {
            OpenMainMenu();
        }
    }
    lastPotVal = potVal;
}

void UpdateVoices(uint32_t nowMS)
{
    static uint32_t lastUpdateMS = 0;
    if(nowMS - lastUpdateMS > VOICE_REFRESH_MS)
    {
        lastUpdateMS = nowMS;
        for(int i = 0; i < app.NUM_VOICES; i++)
        {
            app.voices[i]->SetAttack(voiceAttack[i]);
            app.voices[i]->SetDecay(voiceDecay[i]);
            app.voices[i]->SetSustainLevel(voiceSustain[i]);
            app.voices[i]->SetRelease(voiceRelease[i]);

            app.voices[i]->Update();
        }
    }
}

void UpdateDisplay(uint32_t nowMS)
{
    static uint32_t lastUpdateMS = 0;
    if(nowMS - lastUpdateMS > DISPLAY_REFRESH_MS)
    {
        lastUpdateMS = nowMS;
        char txtBuf[64];

        display.Fill(false);

        if(beatFlash)
        {
            display.DrawRect(
                0, 0, display.Width() - 1, display.Height() - 1, true);
        }

        //////////////////////////////////////////////
        // Current Screen Layout  (WIP)
        //  128x64px
        //
        //  +-----------------+
        //  | 1/4   120   1:2 |  20px
        //  |      IONIAN     |  20px
        //  | X           X   |
        //  | X   X   X   X   |  Remainder = 64 - 40 = 24px;
        //  | B   T   A   S   |  Each 32px wide total; bars 16px?
        //  +-----------------+
        //

        const int line1X = 2;
        const int line1Y = 1;
        const int line2X = 2;
        const int line2Y = line1Y + Font_11x18.FontHeight;
        const int line3Y = line2Y + Font_7x10.FontHeight;

        const int voiceWidth = display.Width() / app.NUM_VOICES;
        const int voiceLineY = display.Height() - Font_6x8.FontHeight - 4;
        const int voiceStartX[app.NUM_VOICES] = {
            (voiceWidth * 0) + 6,
            (voiceWidth * 1) + 6,
            (voiceWidth * 2) + 6,
            (voiceWidth * 3) + 6,
        };

        const int barWidth              = voiceWidth / 2;
        const int barEndY               = voiceLineY - 2;
        const int barFullHeight         = barEndY - line3Y;
        const int barStartX[app.NUM_VOICES] = {
            (voiceWidth * 0) + (barWidth / 2),
            (voiceWidth * 1) + (barWidth / 2),
            (voiceWidth * 2) + (barWidth / 2),
            (voiceWidth * 3) + (barWidth / 2),
        };

        // Line 1: Time Signature, BPM, Bar & Beat
        display.SetCursor(line1X, line1Y);
        snprintf(txtBuf,
                 sizeof(txtBuf),
                 "%2d/%-2d %2d:%-2d",
                 app.ts.beats,
                 app.ts.getDenominator(),
                 app.gnome.GetBar() + 1,
                 app.gnome.GetBeat() + 1);
        display.WriteString(txtBuf, Font_11x18, true);

        display.SetCursor(line2X, line2Y);
        snprintf(txtBuf,
                 sizeof(txtBuf),
                 "%-10s BPM:%3d",
                 SCALE_TABLES[app.GetScaleIndex()].name,
                 bpm.Get());
        display.WriteString(txtBuf, Font_7x10, true);

        // Now a line with all the voices current status
        for(int i = 0; i < app.NUM_VOICES; i++)
        {
            int barHeight
                = (int)(voiceVolumes[i].Get() / 100.0f * barFullHeight);
            int barStartY = barEndY - barHeight;
            display.DrawRect(barStartX[i],
                             barStartY,
                             barStartX[i] + barWidth,
                             barEndY,
                             true,
                             true);
            display.SetCursor(voiceStartX[i], voiceLineY);
            display.WriteString(app.voices[i]->GetNoteText(), Font_6x8, true);
        }

        display.Update();
    }
}

////////////////////////////////////////////////////////////////////////////////
int main(void)
{
    hw.Configure();
    hw.Init();

    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    //
    // Initialize our Audio Components
    //
    float sample_rate = hw.AudioSampleRate();
    clock.Init(BPMToClockFreq(kDefaultBPM), sample_rate);

    // Voices
    for(int i = 0; i < app.NUM_VOICES; i++)
    {
        app.voices[i]->Init(sample_rate);
    }

    //setup reverb
    verb.Init(sample_rate);
    verb.SetFeedback(0.6f);
    verb.SetLpFreq(18000.0f);

    // Initialize our UI Components and Controls
    InitDisplay();
    InitControls();
    InitUI();

    // hw.PrintLine("Starting Audio...");
    hw.StartAudio(AudioCallback);

    float    lastBPM       = bpm;
    uint32_t lastRefreshMS = 0;
    while(1)
    {
        uint32_t nowMS = System::GetNow();

        if(lastBPM != bpm)
        {
            lastBPM = bpm;
            SetBPM(bpm);
        }

        hw.SetLed(beatFlash);

        ProcessControls();
        UpdateVoices(nowMS);
        if(uiOpen)
        {
            ui.Process();
        }
        else
        {
            UpdateDisplay(nowMS);
        }

        beatFlash = beatFlash && (nowMS < beatExpireMS);
    }
}
