#include <daisy_seed.h>
#include <daisysp.h>

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/Tables.h>

#include <App.h>
#include <Pages/MainPage.h>
#include <Pages/MixerPage.h>
#include <Pages/VoicePage.h>

using namespace daisysp;
using namespace daisy;

////////////////////////////////////////////////////////////////////////////////
// Hardware Configuration
////////////////////////////////////////////////////////////////////////////////
constexpr uint32_t DISPLAY_REFRESH_MS = 1000 / 10; // ~30 FPS
constexpr uint32_t APP_REFRESH_MS   = 1000 / 10;  // ~5 FPS
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

UI                 ui;
MainPage           mainPage;
MixerPage          mixerPage;
VoicePage          bassPage(TheApp::THE_BASS);
VoicePage          tenorPage(TheApp::THE_TENOR);
VoicePage          altoPage(TheApp::THE_ALTO);
VoicePage          sopranoPage(TheApp::THE_SOPRANO);
FullScreenItemMenu mainMenu;

// PersistentStorage<SystemConfig> systemStorage(hw.qspi);

Encoder      encoder;
MyDisplay    display;
UiEventQueue eventQueue;
Metro        clock;
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
    { return buttons[buttonID].Pressed(); }
};
ButtonBackend                           buttonBackend;
Switch                                  buttons[BTN_COUNT];
ButtonMonitor<ButtonBackend, BTN_COUNT> btn_mon;

////////////////////////////////////////////////////////////////////////////////
// Runtime status
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
// UI & Menu Structure
////////////////////////////////////////////////////////////////////////////////
AbstractMenu::ItemConfig mainMenuItems[] = {
    {.type = AbstractMenu::ItemType::openUiPageItem,
     .text = "MIXER",
     .asOpenUiPageItem{&mixerPage}},
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
        if(theApp.GetRunning())
        {
            if(clock.Process())
            {
                const int pulse = theApp.DoPulse();

                // Sequence loop boundary: previous cycle ended and pulse wrapped.
                if(lastPulse >= 0 && pulse < lastPulse)
                {
                    theApp.MakeEvents();
                }
                lastPulse = pulse;

                // // Is this a beat boundary?
                // if(theApp.gnome.RisingBeatEdge())
                // {
                //     // beatFlash    = true;
                //     // beatExpireMS = System::GetNow() + BEAT_FLASH_MS;
                //     // dsy_gpio_write(&gate_output, true);
                // }
                // else
                // {
                //     // dsy_gpio_write(&gate_output, false);
                // }
            }

            float sig      = 0.0f;
            float equalMix = 1.0f / (float)theApp.NUM_VOICES;
            for(int i = 0; i < theApp.NUM_VOICES; i++)
            {
                sig += (theApp.GetVoice(i)->Process() * voiceVolumes[i]
                        / 100.0f)
                       * equalMix;
            }

            float wetL = 0.0f;
            float wetR = 0.0f;
            verb.Process(sig, sig, &wetL, &wetR);
            out[i]     = (sig * 0.8f) + (wetL * 0.2f);
            out[i + 1] = (sig * 0.8f) + (wetR * 0.2f);
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

    buttons[BTN_ENCODER].Init(seed::D19);
    buttons[BTN_RUN_STOP].Init(seed::D17);
    buttons[BTN_RANDOM].Init(seed::D18);
    buttons[BTN_PRIOR].Init(seed::D15);
    buttons[BTN_NEXT].Init(seed::D21);

    buttonBackend.buttons = buttons;

    btn_mon.Init(eventQueue, buttonBackend);

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
    special.okBttnId      = BTN_ENCODER;
    special.cancelBttnId  = BTN_RANDOM;
    special.leftBttnId    = BTN_PRIOR;
    special.rightBttnId   = BTN_NEXT;
    special.menuEncoderId = ENCODER_1;

    UiCanvasDescriptor oled_canvas;
    oled_canvas.id_                = 0;
    oled_canvas.handle_            = &display;
    oled_canvas.updateRateMs_      = 30; // ~30 FPS
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
    (void)nowMS;

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

////////////////////////////////////////////////////////////////////////////////
void UpdateApp(uint32_t nowMS)
{    
    static uint32_t lastUpdateMS = 0;
    if(nowMS - lastUpdateMS > APP_REFRESH_MS)
    {
        lastUpdateMS = nowMS;
        theApp.Update();
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

    float    lastBPM       = theApp.GetBPM();
    uint32_t lastRefreshMS = 0;
    while(1)
    {
        uint32_t nowMS = System::GetNow();

        if(lastBPM != theApp.GetBPM())
        {
            lastBPM = theApp.GetBPM();
            SetBPM(lastBPM);
        }

        ProcessControls(nowMS);
        UpdateApp(nowMS);
        

        ui.Process();
    }
}
