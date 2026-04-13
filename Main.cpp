#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/Tables.h>
#include <Music/Gnome.h>

#include "Voices/Bass.h"
#include "Voices/Tenor.h"
#include "Voices/Alto.h"
#include "Voices/Soprano.h"

using namespace daisysp;
using namespace daisy;
using MyDisplay = OledDisplay<SSD130xI2c128x64Driver>;

////////////////////////////////////////////////////////////////////////////////
// Parameters & Defaults
////////////////////////////////////////////////////////////////////////////////
constexpr int BARS        = 8;
constexpr int MAX_EVENTS  = 128;
const float   kDefaultBPM = 60.0f;

DaisySeed          hw;
Encoder            encoder;
MyDisplay          display;
UI                 ui;
UiEventQueue       eventQueue;
FullScreenItemMenu menu;
FullScreenItemMenu scaleMenu;
Metro              clock;
Switch             runStopBtn;
Switch             randomBtn;
dsy_gpio           gate_output;
AnalogControl      pots[4];
ReverbSc           verb;

enum ScaleIndex
{
    IONIAN_ITEM,
    DORIAN_ITEM,
    PHRYGIAN_ITEM,
    LYDIAN_ITEM,
    MIXOLYDIAN_ITEM,
    AEOLIAN_ITEM,
    LOCRIAN_ITEM,
    SCALE_ITEM_COUNT
};

HarmonicMode myMode     = HarmonicMode::Major;
ScaleIndex   myScaleIdx = IONIAN_ITEM;

// A4 = 440 Hz in 12-EDO where C is degree 0 and A is degree 9.
TimeSignature   ts;
TuningReference refA4(440.0f, 9, 0);
Temperament     t;
Gnome           gnome(ts, BARS);
ScaleMap        scale;
TheBass         bass(hw, ts, refA4, t, scale);
TheTenor        tenor(hw, ts, refA4, t, scale);
TheAlto         alto(hw, ts, refA4, t, scale);
TheSoprano      soprano(hw, ts, refA4, t, scale);

////////////////////////////////////////////////////////////////////////////////
// Runtime status
////////////////////////////////////////////////////////////////////////////////
bool menuOpen = false;
MappedFloatValue
     bpm(1.0f, 300.0f, kDefaultBPM, MappedFloatValue::Mapping::lin, "bpm", 1);
bool running = false;
MappedFloatValue
    bassVol(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%", 0);
MappedFloatValue
    tenorVol(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%", 0);
MappedFloatValue
    altoVol(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%", 0);
MappedFloatValue
    sopranoVol(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%", 0);

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
void OpenMainMenu()
{
    if(!menuOpen)
    {
        ui.OpenPage(menu);
        menuOpen = true;
    }
}

////////////////////////////////////////////////////////////////////////////////
void CloseMainMenu()
{
    if(menuOpen)
    {
        ui.ClosePage(menu);
        menuOpen = false;
    }
}

////////////////////////////////////////////////////////////////////////////////
void SetScaleIndex(ScaleIndex scaleIdx)
{
    myScaleIdx                = scaleIdx;
    const Degree* palette     = nullptr;
    size_t        palette_len = 0;
    switch(myScaleIdx)
    {
        default:
        case IONIAN_ITEM:
            palette     = IONIAN_D12;
            palette_len = ArrayLen(IONIAN_D12);
            myMode      = HarmonicMode::Major;
            break;
        case DORIAN_ITEM:
            palette     = DORIAN_D12;
            palette_len = ArrayLen(DORIAN_D12);
            myMode      = HarmonicMode::Minor;
            break;
        case PHRYGIAN_ITEM:
            palette     = PHRYGIAN_D12;
            palette_len = ArrayLen(PHRYGIAN_D12);
            myMode      = HarmonicMode::Minor;
            break;
        case LYDIAN_ITEM:
            palette     = LYDIAN_D12;
            palette_len = ArrayLen(LYDIAN_D12);
            myMode      = HarmonicMode::Major;
            break;
        case MIXOLYDIAN_ITEM:
            palette     = MIXOLYDIAN_D12;
            palette_len = ArrayLen(MIXOLYDIAN_D12);
            myMode      = HarmonicMode::Major;
            break;
        case AEOLIAN_ITEM:
            palette     = AEOLIAN_D12;
            palette_len = ArrayLen(AEOLIAN_D12);
            myMode      = HarmonicMode::Minor;
            break;
        case LOCRIAN_ITEM:
            palette     = LOCRIAN_D12;
            palette_len = ArrayLen(LOCRIAN_D12);
            myMode      = HarmonicMode::Minor;
            break;
    }

    scale.setDegrees(palette, palette_len);
}


////////////////////////////////////////////////////////////////////////////////
// Menu Callbacks
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void SelectScaleCallback(void* context)
{
    SetScaleIndex(static_cast<ScaleIndex>((int)context));
    ui.ClosePage(scaleMenu);
}

////////////////////////////////////////////////////////////////////////////////
void ResetValuesCallback(void* context)
{
    (void)context;
    bpm        = kDefaultBPM;
    bassVol    = 100.0f;
    tenorVol   = 100.0f;
    altoVol    = 100.0f;
    sopranoVol = 100.0f;
}

////////////////////////////////////////////////////////////////////////////////
void CloseMenuCallback(void* context)
{
    (void)context;
    CloseMainMenu();
}

////////////////////////////////////////////////////////////////////////////////
// UI & Menu Structure
////////////////////////////////////////////////////////////////////////////////
enum ButtonIds
{
    BUTTON_OK = 0,
};

enum EncoderIds
{
    ENCODER_MENU = 0,
};

AbstractMenu::ItemConfig topItems[] = {
    {.type = AbstractMenu::ItemType::checkboxItem,
     .text = "RUNNING",
     .asCheckboxItem{&running}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "TEMPO",
     .asMappedValueItem{&bpm}},
    {.type = AbstractMenu::ItemType::openUiPageItem,
     .text = "SCALE",
     .asOpenUiPageItem{&scaleMenu}},
    // {.type = AbstractMenu::ItemType::valueItem,
    //  .text = "BASS",
    //  .asMappedValueItem{&bassVol}},
    // {.type = AbstractMenu::ItemType::valueItem,
    //  .text = "TENOR",
    //  .asMappedValueItem{&tenorVol}},
    // {.type = AbstractMenu::ItemType::valueItem,
    //  .text = "ALTO",
    //  .asMappedValueItem{&altoVol}},
    // {.type = AbstractMenu::ItemType::valueItem,
    //  .text = "SOPRANO",
    //  .asMappedValueItem{&sopranoVol}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "RESET",
     .asCallbackFunctionItem = {ResetValuesCallback, nullptr}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "CLOSE",
     .asCallbackFunctionItem = {CloseMenuCallback, nullptr}},
};

AbstractMenu::ItemConfig scaleItems[] = {
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "IONIAN",
     .asCallbackFunctionItem = {SelectScaleCallback, (void*)IONIAN_ITEM}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "DORIAN",
     .asCallbackFunctionItem = {SelectScaleCallback, (void*)DORIAN_ITEM}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "PHRYGIAN",
     .asCallbackFunctionItem = {SelectScaleCallback, (void*)PHRYGIAN_ITEM}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "LYDIAN",
     .asCallbackFunctionItem = {SelectScaleCallback, (void*)LYDIAN_ITEM}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "MIXOLYDIAN",
     .asCallbackFunctionItem = {SelectScaleCallback, (void*)MIXOLYDIAN_ITEM}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "AEOLIAN",
     .asCallbackFunctionItem = {SelectScaleCallback, (void*)AEOLIAN_ITEM}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "LOCRIAN",
     .asCallbackFunctionItem = {SelectScaleCallback, (void*)LOCRIAN_ITEM}},
};

////////////////////////////////////////////////////////////////////////////////
// Converts our current BPM to a clock frequency based on pulses per quarter note.
inline float bpmToClockFreq(float bpm)
{ return (bpm * PPQN) / 60.0f; }

////////////////////////////////////////////////////////////////////////////////
void setBPM(float bpm)
{
    float freq = bpmToClockFreq(bpm);
    clock.SetFreq(freq);
}

////////////////////////////////////////////////////////////////////////////////
size_t makeChordEvents(Music::ChordEvent* eventsOut, size_t eventsOutLen)
{
    return Music::generateStandardChordEvents(
        ts, scale, BARS, myMode, NoteValue::Whole, eventsOut, eventsOutLen);
    // First start with our "hit" pattern
    // bool   pattern[MAX_EVENTS];
    // size_t patternLen = Music::generatePattern(
    //     ts, BARS, 0.50f, Music::NoteValue::Quarter, pattern, ArrayLen(pattern));
    // return Music::generateChordEventsFromPattern(pattern,
    //                                              patternLen,
    //                                              Music::NoteValue::Quarter,
    //                                              eventsOut,
    //                                              eventsOutLen);
}

////////////////////////////////////////////////////////////////////////////////
void makeEvents()
{
    Music::ChordEvent chordEvents[MAX_EVENTS];
    size_t            chordEventsLen = 0;

    // First establish our harmonic rhythm
    chordEventsLen = makeChordEvents(chordEvents, ArrayLen(chordEvents));

    // Pass that onto our voices
    bass.makeEvents(ts, BARS, chordEvents, chordEventsLen);
    tenor.makeEvents(ts, BARS, chordEvents, chordEventsLen);
    alto.makeEvents(ts, BARS, chordEvents, chordEventsLen);
    soprano.makeEvents(ts, BARS, chordEvents, chordEventsLen);
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
        if(running)
        {
            if(clock.Process())
            {
                const int pulse = gnome.doPulse();

                // Sequence loop boundary: previous cycle ended and pulse wrapped.
                if(lastPulse >= 0 && pulse < lastPulse)
                {
                    // SetScaleIndex(static_cast<ScaleIndex>(
                    //     randomRange(0, static_cast<int>(SCALE_ITEM_COUNT))));
                    makeEvents();
                }
                lastPulse = pulse;

                // Is this a beat boundary?
                if(gnome.RisingBeatEdge())
                {
                    hw.SetLed(true);
                    dsy_gpio_write(&gate_output, true);
                }
                else
                {
                    hw.SetLed(false);
                    dsy_gpio_write(&gate_output, false);
                }

                // Pass the pulse on to our voices
                bass.doPulse(pulse);
                tenor.doPulse(pulse);
                alto.doPulse(pulse);
                soprano.doPulse(pulse);
            }

            float bassSig    = bass.Process() * (bassVol / 100.0f);
            float tenorSig   = tenor.Process() * (tenorVol / 100.0f);
            float altoSig    = alto.Process() * (altoVol / 100.0f);
            float sopranoSig = soprano.Process() * (sopranoVol / 100.0f);
            float sig  = (bassSig * 0.25) + (tenorSig * 0.25) + (altoSig * 0.25)
                         + (sopranoSig * 0.25);
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
void InitUI()
{
    UI::SpecialControlIds special;
    special.okBttnId      = BUTTON_OK;
    special.menuEncoderId = ENCODER_MENU;

    UiCanvasDescriptor oled_canvas;
    oled_canvas.id_                = 0;
    oled_canvas.handle_            = &display;
    oled_canvas.updateRateMs_      = 33; // ~30 FPS
    oled_canvas.screenSaverTimeOut = 5000;
    oled_canvas.clearFunction_     = ClearCanvas;
    oled_canvas.flushFunction_     = FlushCanvas;

    ui.Init(eventQueue, special, {oled_canvas}, 0);
    menu.Init(topItems,
              ArrayLen(topItems),
              AbstractMenu::Orientation::upDownSelectLeftRightModify,
              true);
    scaleMenu.Init(scaleItems,
                   ArrayLen(scaleItems),
                   AbstractMenu::Orientation::upDownSelectLeftRightModify,
                   true);
}

////////////////////////////////////////////////////////////////////////////////
void InitControls()
{
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


    /** Initialize our Encoder */
    encoder.Init(seed::D20, seed::D16, seed::D19, 1000);

    gate_output.pin  = seed::D15;
    gate_output.mode = DSY_GPIO_MODE_OUTPUT_PP;
    gate_output.pull = DSY_GPIO_NOPULL;
    dsy_gpio_init(&gate_output);
}

////////////////////////////////////////////////////////////////////////////////
void ProcessControls()
{
    runStopBtn.Debounce();
    randomBtn.Debounce();
    encoder.Debounce();

    const int32_t inc     = encoder.Increment();
    const bool    press   = encoder.RisingEdge();
    const bool    release = encoder.FallingEdge();

    if(runStopBtn.RisingEdge())
        running = !running;

    if(randomBtn.RisingEdge())
    {
        SetScaleIndex(static_cast<ScaleIndex>(
            randomRange(0, static_cast<int>(SCALE_ITEM_COUNT))));
        makeEvents();
        gnome.reset();
    }

    bassVol.Set(pots[0].Process() * 100.0f);
    tenorVol.Set(pots[1].Process() * 100.0f);
    altoVol.Set(pots[2].Process() * 100.0f);
    sopranoVol.Set(pots[3].Process() * 100.0f);

    if(menuOpen)
    {
        if(inc != 0)
        {
            eventQueue.AddEncoderTurned(ENCODER_MENU, inc, 20);
            eventQueue.AddEncoderActivityChanged(ENCODER_MENU, true);
        }
        else
        {
            // Optional: Tell the UI that the encoder is idle.
            eventQueue.AddEncoderActivityChanged(ENCODER_MENU, false);
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
    }
    else
    {
        // Menu is closed: don't queue menu navigation events.
        // Use encoder click to reopen it.
        if(press)
            OpenMainMenu();
    }
}

////////////////////////////////////////////////////////////////////////////////
int main(void)
{
    hw.Configure();
    hw.Init();

//    hw.StartLog(false);
//    hw.PrintLine("Happy Birthday!!!");


    // Init our music environment
    t.makeEqualDivision(12, 2.0f);
    t.attachNoteLabels(Music::NOTE_NAMES_12, ArrayLen(Music::NOTE_NAMES_12));
    t.attachIntervalLabels(Music::INTERVAL_NAMES_12,
                           ArrayLen(Music::INTERVAL_NAMES_12));

    SetScaleIndex(IONIAN_ITEM);

    // Init our first batch of upcoming events.
    makeEvents();

    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    //
    // Initialize our Audio Components
    //
    float sample_rate = hw.AudioSampleRate();
    clock.Init(bpmToClockFreq(kDefaultBPM), sample_rate);

    // Voices
    bass.Init(sample_rate);
    tenor.Init(sample_rate);
    alto.Init(sample_rate);
    soprano.Init(sample_rate);

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
            setBPM(bpm);
        }

        ProcessControls();
        if(menuOpen)
        {
            ui.Process();
        }
        else
        {
            if(nowMS - lastRefreshMS > 50)
            {
                lastRefreshMS = nowMS;

                char txtBuf[64];

                display.Fill(false);
                display.SetCursor(0, 0);
                snprintf(txtBuf,
                         sizeof(txtBuf),
                         "%d::%d",
                         gnome.getBar() + 1,
                         gnome.getBeat() + 1);
                display.WriteString(txtBuf, Font_11x18, true);
                display.SetCursor(0, 20);
                switch(myScaleIdx)
                {
                    case IONIAN_ITEM:
                        display.WriteString("IONIAN", Font_7x10, true);
                        break;
                    case DORIAN_ITEM:
                        display.WriteString("DORIAN", Font_7x10, true);
                        break;
                    case PHRYGIAN_ITEM:
                        display.WriteString("PHRYGIAN", Font_7x10, true);
                        break;
                    case LYDIAN_ITEM:
                        display.WriteString("LYDIAN", Font_7x10, true);
                        break;
                    case MIXOLYDIAN_ITEM:
                        display.WriteString("MIXOLYDIAN", Font_7x10, true);
                        break;
                    case AEOLIAN_ITEM:
                        display.WriteString("AEOLIAN", Font_7x10, true);
                        break;
                    case LOCRIAN_ITEM:
                        display.WriteString("LOCRIAN", Font_7x10, true);
                        break;
                    default:
                        display.WriteString("??????", Font_7x10, true);
                        break;
                }
                display.Update();
            }
        }
    }
}
