// Not a real main....just something to test ideas out with.

#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <EncoderMonitor.h>

#include <Music/MusicHelpers.h>

using namespace daisysp;
using namespace daisy;
using Display = daisy::OledDisplay<daisy::SSD130xI2c128x64Driver>;

DaisySeed hw;
Display   display;

Metro   clock;
HiHat<> hat;

enum EncoderId
{
    ENCODER_1,
    // ENCODER_2,
    // ENCODER_3,
    // ENCODER_4,
    ENCODER_COUNT
};

struct EncoderConfig
{
    Pin a;
    Pin b;
    Pin click;
};

Encoder                 encoders[ENCODER_COUNT];
constexpr EncoderConfig encoder_config[ENCODER_COUNT] = {
    {seed::D20, seed::D16, seed::D19}, // ENCODER_1
    // {seed::D0, seed::D1, seed::D2},   // ENCODER_1
    // {seed::D3, seed::D4, seed::D5},   // ENCODER_2
    // {seed::D6, seed::D7, seed::D8},   // ENCODER_3
    // {seed::D9, seed::D10, seed::D15}, // ENCODER_4
};
struct EncoderBackend
{
    Encoder* encoders;
    int32_t Increment(uint16_t encoderID)
    { return encoders[encoderID].Increment(); }
};
EncoderMonitor<EncoderBackend, ENCODER_COUNT> encoder_monitor;

enum ButtonId
{
    BTN_ENCODER_1,
    BTN_COUNT
};

struct ButtonBackend
{
    Switch* buttons;
    bool    IsButtonPressed(uint16_t buttonID)
    { return buttons[buttonID].Pressed(); }
};
ButtonBackend button_backend;
Switch        buttons[BTN_COUNT];
constexpr Pin button_config[BTN_COUNT] = {
    seed::D19,
};
ButtonMonitor<ButtonBackend, BTN_COUNT> button_monitor;

UI           ui;
UiEventQueue event_queue;

////////////////////////////////////////////////////////////////////////////////
// Canvas Callbacks for UI
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
static void ClearCanvas(const UiCanvasDescriptor& canvas)
{
    auto* d = static_cast<Display*>(canvas.handle_);
    d->Fill(false);
}

////////////////////////////////////////////////////////////////////////////////
static void FlushCanvas(const UiCanvasDescriptor& canvas)
{
    auto* d = static_cast<Display*>(canvas.handle_);
    d->Update();
}

////////////////////////////////////////////////////////////////////////////////
// Main Audio Loop
////////////////////////////////////////////////////////////////////////////////
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    //Prepare the audio block
    for(size_t i = 0; i < size; i += 2)
    {
        bool trig = clock.Process();

        float sig = hat.Process(trig);

        // Null output for now.
        out[i]     = sig;
        out[i + 1] = sig;
    }
}

////////////////////////////////////////////////////////////////////////////////
void init_hardware()
{
    //
    // Display Initialization.  Clear on startup.
    //
    Display::Config disp_cfg;
    display.Init(disp_cfg);
    display.Fill(false);
    display.Update();

    //
    // Encoder(s) Initialization
    //
    for(size_t i = 0; i < ENCODER_COUNT; i++)
    {
        encoders[i].Init(
            encoder_config[i].a, encoder_config[i].b, encoder_config[i].click);
    }

    //
    // Button Initialization
    //
    for(size_t i = 0; i < BTN_COUNT; i++)
    {
        buttons[i].Init(button_config[i]);
    }
    button_backend.buttons = buttons;
    button_monitor.Init(event_queue, button_backend);

    //
    // UI Initialization
    //
    UI::SpecialControlIds special;
    special.menuEncoderId = ENCODER_1;
    special.okBttnId      = BTN_ENCODER_1;

    UiCanvasDescriptor oled_canvas;
    oled_canvas.id_                = 0;
    oled_canvas.handle_            = &display;
    oled_canvas.updateRateMs_      = 30; // ~30 FPS
    oled_canvas.screenSaverTimeOut = 5000;
    oled_canvas.clearFunction_     = ClearCanvas;
    oled_canvas.flushFunction_     = FlushCanvas;

    ui.Init(event_queue, special, {oled_canvas}, 0);
}

void process_hardware(uint32_t nowMS)
{
    //
    // Debounce all the things
    //
    // Buttons
    for(size_t i = 0; i < BTN_COUNT; i++)
        buttons[i].Debounce();
    // Encoders
    for(size_t i = 0; i < ENCODER_COUNT; i++)
        encoders[i].Debounce();

    //
    // Event Queueing & UI Processing
    //
    button_monitor.Process();
    encoder_monitor.Process();
    ui.Process();
}

////////////////////////////////////////////////////////////////////////////////
int main(void)
{
    hw.Configure();
    hw.Init();

    hw.StartLog(false);

    init_hardware();

    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    //
    // Initialize our Audio Components
    //
    float sample_rate = hw.AudioSampleRate();
    clock.Init(Music::BpmToFreq(60.0f), sample_rate);

    hat.Init(sample_rate);

    hw.PrintLine("Starting Audio...");
    hw.StartAudio(AudioCallback);

    while(1)
    {
        uint32_t nowMS = System::GetNow();

        process_hardware(nowMS);
    }
}
