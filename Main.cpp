#include <daisy_seed.h>
#include <daisysp.h>

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/Tables.h>
#include <Music/Gnome.h>

#include "App.h"
#include "VoiceMenu.h"

using namespace daisysp;
using namespace daisy;

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

DaisySeed hw;
// A4 = 440 Hz in 12-EDO where C is degree 0 and A is degree 9.
//TheApp theApp(BARS);
TheApp& theApp = TheApp::instance(BARS);

Encoder      encoder;
MyDisplay    display;
UiEventQueue eventQueue;
Metro        clock;
// dsy_gpio           gate_output;
ReverbSc verb;

struct PotBackend
{
    AdcHandle* adc;

    // Used by PotMonitor
    float GetPotValue(uint16_t potId) { return adc->GetFloat(potId); }
};
PotBackend                        potBackend;
PotMonitor<PotBackend, POT_COUNT> pot_mon;

struct ButtonBackend
{
    Switch* buttons;
    bool    IsButtonPressed(uint16_t buttonID)
    {
        return buttons[buttonID].Pressed();
    }
};
ButtonBackend                           buttonBackend;
Switch                                  buttons[BTN_COUNT];
ButtonMonitor<ButtonBackend, BTN_COUNT> btn_mon;

////////////////////////////////////////////////////////////////////////////////
// Runtime status
////////////////////////////////////////////////////////////////////////////////
MappedIntValue   bpm(1, 300, kDefaultBPM, 1, 10, "bpm");
MappedFloatValue voiceVolumes[theApp.NUM_VOICES] = {
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

UI                 ui;
FullScreenItemMenu mainMenu;
VoiceMenuPage      bassPage(TheApp::THE_BASS);
VoiceMenuPage      tenorPage(TheApp::THE_TENOR);
VoiceMenuPage      altoPage(TheApp::THE_ALTO);
VoiceMenuPage      sopranoPage(TheApp::THE_SOPRANO);

class MainPage : public UiPage
{
  public:
    MainPage() {}

    bool OnMenuEncoderTurned(int16_t  turns,
                             uint16_t stepsPerRevolution) override
    {
        bpm = bpm.Get() + turns;
        return true;
    }

    bool OnPotMoved(uint16_t potID, float newPosition) override
    {
        voiceVolumes[potID].Set(newPosition * 100.0f);
        return true;
    }

    // Buttons doing double duty based on page context.
    bool OnOkayButton(uint8_t numberOfPresses, bool isRetriggering) override
    {
        if(numberOfPresses > 0)
        {
            ui.OpenPage(mainMenu);
        }
        return true;
    }

    bool OnCancelButton(uint8_t numberOfPresses, bool isRetriggering) override
    {
        if(numberOfPresses > 0)
        {
            theApp.SetScaleIndex(
                randomRange(D12StartIndex, D12StartIndex + D12Count));
            theApp.gnome.Reset();
        }
        return true;
    }

    bool OnButton(uint16_t buttonID,
                  uint8_t  numberOfPresses,
                  bool     isRetriggering) override
    {
        if(numberOfPresses > 0)
        {
            switch(buttonID)
            {
                case BTN_RUN_STOP:
                    theApp.SetRunning(!theApp.GetRunning());
                    break;
            }
        }
        return true;
    }

    bool OnArrowButton(ArrowButtonType arrowType,
                       uint8_t         numberOfPresses,
                       bool            isRetriggering) override
    {
        if(numberOfPresses > 0)
        {
            switch(arrowType)
            {
                case ArrowButtonType::left:
                    theApp.SetScaleIndex(theApp.GetScaleIndex() - 1);
                    break;
                case ArrowButtonType::right:
                    theApp.SetScaleIndex(theApp.GetScaleIndex() + 1);
                    break;
                case ArrowButtonType::up: break;
                case ArrowButtonType::down: break;
            }
        }
        return true;
    }

    // inherited from UiPage
    void Draw(const UiCanvasDescriptor& canvas) override
    {
        char  txtBuf[64];
        auto* d = static_cast<MyDisplay*>(canvas.handle_);

        d->Fill(false);

        // if(beatFlash)
        // {
        //     d->DrawRect(0, 0, d->Width() - 1, d->Height() - 1, true);
        // }

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

        const int voiceWidth = d->Width() / theApp.NUM_VOICES;
        const int voiceLineY = d->Height() - Font_6x8.FontHeight - 4;
        const int voiceStartX[theApp.NUM_VOICES] = {
            (voiceWidth * 0) + 6,
            (voiceWidth * 1) + 6,
            (voiceWidth * 2) + 6,
            (voiceWidth * 3) + 6,
        };

        const int barWidth                     = voiceWidth / 2;
        const int barEndY                      = voiceLineY - 2;
        const int barFullHeight                = barEndY - line3Y;
        const int barStartX[theApp.NUM_VOICES] = {
            (voiceWidth * 0) + (barWidth / 2),
            (voiceWidth * 1) + (barWidth / 2),
            (voiceWidth * 2) + (barWidth / 2),
            (voiceWidth * 3) + (barWidth / 2),
        };

        // Line 1: Time Signature, BPM, Bar & Beat
        d->SetCursor(line1X, line1Y);
        snprintf(txtBuf,
                 sizeof(txtBuf),
                 "%2d/%-2d %2d:%-2d",
                 theApp.GetTS()->beats,
                 theApp.GetTS()->GetDenominator(),
                 theApp.gnome.GetBar() + 1,
                 theApp.gnome.GetBeat() + 1);
        d->WriteString(txtBuf, Font_11x18, true);

        d->SetCursor(line2X, line2Y);
        snprintf(txtBuf,
                 sizeof(txtBuf),
                 "%-10s BPM:%3d",
                 SCALE_TABLES[theApp.GetScaleIndex()].name,
                 bpm.Get());
        d->WriteString(txtBuf, Font_7x10, true);

        // Now a line with all the voices current status
        for(int i = 0; i < theApp.NUM_VOICES; i++)
        {
            int barHeight
                = (int)(voiceVolumes[i].Get() / 100.0f * barFullHeight);
            int barStartY = barEndY - barHeight;
            d->DrawRect(barStartX[i],
                        barStartY,
                        barStartX[i] + barWidth,
                        barEndY,
                        true,
                        true);
            d->SetCursor(voiceStartX[i], voiceLineY);
            d->WriteString(theApp.GetVoice(i)->GetNoteText(), Font_6x8, true);
        }

        d->Update();
    }
};

MainPage mainPage;

////////////////////////////////////////////////////////////////////////////////
// UI & Menu Structure
////////////////////////////////////////////////////////////////////////////////
AbstractMenu::ItemConfig mainMenuItems[] = {
    {.type = AbstractMenu::ItemType::openUiPageItem,
     .text = "BASS",
     .asOpenUiPageItem{&bassPage}},
    {.type = AbstractMenu::ItemType::openUiPageItem,
     .text = "TENOR",
     .asOpenUiPageItem{&tenorPage}},
    {.type = AbstractMenu::ItemType::openUiPageItem,
     .text = "ALTO",
     .asOpenUiPageItem{&altoPage}},
    {.type = AbstractMenu::ItemType::openUiPageItem,
     .text = "SOPRANO",
     .asOpenUiPageItem{&sopranoPage}},
    {.type = AbstractMenu::ItemType::closeMenuItem, .text = "CLOSE"},
};


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

    for(int i = 0; i < theApp.NUM_VOICES; i++) {}
}

////////////////////////////////////////////////////////////////////////////////
// Converts our current BPM to a clock frequency based on pulses per quarter note.
inline float BPMToClockFreq(int bpm)
{
    return ((float)bpm * PPQN) / 60.0f;
}

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
        if(theApp.GetRunning())
        {
            if(clock.Process())
            {
                const int pulse = theApp.gnome.DoPulse();

                // Sequence loop boundary: previous cycle ended and pulse wrapped.
                if(lastPulse >= 0 && pulse < lastPulse)
                {
                    theApp.MakeEvents();
                }
                lastPulse = pulse;

                // Is this a beat boundary?
                if(theApp.gnome.RisingBeatEdge())
                {
                    // beatFlash    = true;
                    // beatExpireMS = System::GetNow() + BEAT_FLASH_MS;
                    // dsy_gpio_write(&gate_output, true);
                }
                else
                {
                    // dsy_gpio_write(&gate_output, false);
                }

                // Pass the pulse on to our voices
                theApp.DoPulse(pulse);
            }

            float sig      = 0.0f;
            float equalMix = 1.0f / (float)theApp.NUM_VOICES;
            for(int i = 0; i < theApp.NUM_VOICES; i++)
            {
                sig += (theApp.GetVoice(i)->Process() * voiceVolumes[i]
                        / 100.0f)
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
    // Fixed at four voices/four pots.  TODO: make this more flexible if we want to expand beyond SATB.
    AdcChannelConfig adc_config[POT_COUNT];
    adc_config[0].InitSingle(seed::A7);
    adc_config[1].InitSingle(seed::A8);
    adc_config[2].InitSingle(seed::A9);
    adc_config[3].InitSingle(seed::A10);
    hw.adc.Init(adc_config, POT_COUNT);
    hw.adc.Start();

    potBackend.adc = &hw.adc;

    pot_mon.Init(eventQueue, potBackend);

    buttons[BTN_ENCODER].Init(seed::D19, 1000.0f);
    buttons[BTN_RUN_STOP].Init(seed::D17, 1000.0f);
    buttons[BTN_RANDOM].Init(seed::D18, 1000.0f);
    buttons[BTN_PRIOR].Init(seed::D15, 1000.0f);
    buttons[BTN_NEXT].Init(seed::D21, 1000.0f);

    buttonBackend.buttons = buttons;

    btn_mon.Init(eventQueue, buttonBackend);

    encoder.Init(seed::D20, seed::D16, seed::D19);

    //
    // AdcChannelConfig adc_config[4];
    // adc_config[0].InitSingle(seed::A7);
    // adc_config[1].InitSingle(seed::A8);
    // adc_config[2].InitSingle(seed::A9);
    // adc_config[3].InitSingle(seed::A10);

    // hw.adc.Init(adc_config, 4);
    // hw.adc.Start();

    // const float sample_rate = 1000;
    // const float slew_seconds = 0.002f;

    // for (int i = 0; i < POT_COUNT; i++)
    // {
    //     pot[i].Init(hw.adc.GetPtr(i), sample_rate, true, false, slew_seconds);
    // }

    // btn[BTN_RUN_STOP].Init(seed::D17, sample_rate);
    // btn[BTN_RANDOM].Init(seed::D18, sample_rate);
    // btn[BTN_PRIOR].Init(seed::D15, sample_rate);
    // btn[BTN_NEXT].Init(seed::D21, sample_rate);

    // /** Initialize our Encoder */

    // gate_output.pin  = seed::D15;
    // gate_output.mode = DSY_GPIO_MODE_OUTPUT_PP;
    // gate_output.pull = DSY_GPIO_NOPULL;
    // dsy_gpio_init(&gate_output);
}

////////////////////////////////////////////////////////////////////////////////
void InitUI()
{
    UI::SpecialControlIds special;
    special.okBttnId      = BTN_ENCODER;
    special.cancelBttnId  = BTN_RANDOM;
    special.leftBttnId    = BTN_PRIOR;
    special.rightBttnId   = BTN_NEXT;
    special.menuEncoderId = ENCODER_1;

    UiCanvasDescriptor oled_canvas;
    oled_canvas.id_                = 0;
    oled_canvas.handle_            = &display;
    oled_canvas.updateRateMs_      = 33; // ~30 FPS
    oled_canvas.screenSaverTimeOut = 5000;
    oled_canvas.clearFunction_     = ClearCanvas;
    oled_canvas.flushFunction_     = FlushCanvas;

    ui.Init(eventQueue, special, {oled_canvas}, 0);

    mainMenu.Init(mainMenuItems,
                  ArrayLen(mainMenuItems),
                  AbstractMenu::Orientation::leftRightSelectUpDownModify,
                  true);

    ui.OpenPage(mainPage);
}

////////////////////////////////////////////////////////////////////////////////
void ProcessControls(uint32_t nowMS)
{
    // Debounce all the things that need explicit debouncing.
    for(int i = 0; i < BTN_COUNT; i++)
        buttons[i].Debounce();
    encoder.Debounce();

    // Fill our event queue from our controls
    pot_mon.Process();
    btn_mon.Process();

    static bool encoderActive = false;
    int         inc           = encoder.Increment();
    if(inc != 0)
    {
        if(!encoderActive)
        {
            eventQueue.AddEncoderActivityChanged(ENCODER_1, true);
            encoderActive = true;
        }
        eventQueue.AddEncoderTurned(ENCODER_1, inc, 20);
    }
    else
    {
        if(encoderActive)
        {
            eventQueue.AddEncoderActivityChanged(ENCODER_1, false);
            encoderActive = false;
        }
    }
}

void ProcessVoices(uint32_t nowMS)
{
    static uint32_t lastUpdateMS = 0;
    if(nowMS - lastUpdateMS > VOICE_REFRESH_MS)
    {
        lastUpdateMS = nowMS;
        for(int i = 0; i < theApp.NUM_VOICES; i++)
        {
            TheVoice* v = theApp.GetVoice(i);
            v->Update();
        }
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

    theApp.Init(sample_rate);

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

        ProcessControls(nowMS);
        ProcessVoices(nowMS);

        ui.Process();
    }
}
