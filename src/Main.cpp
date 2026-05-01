#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/Tables.h>

#include <UIOverlord.h>

#include <App.h>
#include <Pages/MainPage.h>
#include <Pages/MixerPage.h>
#include <Pages/VoicePage.h>

using namespace daisysp;
using namespace daisy;

#define ENCODER_COUNT 1
#define BUTTON_COUNT 5
#define POT_COUNT 4

using MyOverlord =
    UIOverlord<SSD130xI2c128x64Driver, ENCODER_COUNT, BUTTON_COUNT, POT_COUNT,
               ENCODER_1, // MenuEncoder
               BUTTON_1,  // OK Button
               BUTTON_2,  // Cancel Button
               true>;

////////////////////////////////////////////////////////////////////////////////
// Hardware Configuration
////////////////////////////////////////////////////////////////////////////////
constexpr uint32_t DISPLAY_REFRESH_MS = 1000 / 10; // ~30 FPS
constexpr uint32_t APP_REFRESH_MS = 1000 / 10;     // ~5 FPS
constexpr uint32_t BEAT_FLASH_MS = 1000 / 20;

////////////////////////////////////////////////////////////////////////////////
// Parameters & Defaults
////////////////////////////////////////////////////////////////////////////////
constexpr int BARS = 8;
constexpr int MAX_EVENTS = 128;
const int kDefaultBPM = 60;

DaisySeed hw;
// A4 = 440 Hz in 12-EDO where C is degree 0 and A is degree 9.
// TheApp theApp(BARS);
TheApp &theApp = TheApp::instance(BARS);
MyOverlord uiOverlord;
MainPage mainPage;
MixerPage mixerPage;
VoicePage bassPage(TheApp::THE_BASS);
VoicePage tenorPage(TheApp::THE_TENOR);
VoicePage altoPage(TheApp::THE_ALTO);
VoicePage sopranoPage(TheApp::THE_SOPRANO);
FullScreenItemMenu mainMenu;

const MyOverlord::EncoderConfig encoderConfig[ENCODER_COUNT] = {
    {seed::D20, seed::D16},
};
const MyOverlord::ButtonConfig buttonConfig[BUTTON_COUNT] = {
    {seed::D19}, // Encoder
    {seed::D17}, {seed::D18}, {seed::D15}, {seed::D21},
};
const MyOverlord::PotConfig potConfig[POT_COUNT] = {
    {seed::A7},
    {seed::A8},
    {seed::A9},
    {seed::A10},
};

Metro clock;
ReverbSc verb;

////////////////////////////////////////////////////////////////////////////////
// Runtime status
////////////////////////////////////////////////////////////////////////////////
MappedFloatValue voiceVolumes[theApp.NUM_VOICES] = {
    MappedFloatValue(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%",
                     0),
    MappedFloatValue(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%",
                     0),
    MappedFloatValue(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%",
                     0),
    MappedFloatValue(1.0f, 100.0f, 100.0f, MappedFloatValue::Mapping::log, "%",
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
// Converts our current BPM to a clock frequency based on pulses per quarter
// note.
inline float BPMToClockFreq(int bpm) { return ((float)bpm * PPQN) / 60.0f; }

////////////////////////////////////////////////////////////////////////////////
void SetBPM(int bpm) {
  float freq = BPMToClockFreq(bpm);
  clock.SetFreq(freq);
}

////////////////////////////////////////////////////////////////////////////////
// Main Audio Loop
////////////////////////////////////////////////////////////////////////////////
void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                   AudioHandle::InterleavingOutputBuffer out, size_t size) {

  uiOverlord.ProcessControls();

  // Prepare the audio block
  for (size_t i = 0; i < size; i += 2) {
    // New pulse
    if (theApp.GetRunning()) {
      if (clock.Process()) {
        const int pulse = theApp.DoPulse();
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

      float sig = 0.0f;
      float equalMix = 1.0f / (float)theApp.NUM_VOICES;
      for (int i = 0; i < theApp.NUM_VOICES; i++) {
        sig += (theApp.GetVoice(i)->Process() * voiceVolumes[i] / 100.0f) *
               equalMix;
      }

      float wetL = 0.0f;
      float wetR = 0.0f;
      verb.Process(sig, sig, &wetL, &wetR);
      out[i] = (sig * 0.8f) + (wetL * 0.2f);
      out[i + 1] = (sig * 0.8f) + (wetR * 0.2f);
    } else {
      out[i] = 0.0f;
      out[i + 1] = 0.0f;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void UpdateApp(uint32_t nowMS) {
  static uint32_t lastUpdateMS = 0;
  if (nowMS - lastUpdateMS > APP_REFRESH_MS) {
    lastUpdateMS = nowMS;
    theApp.Update();
  }
}

////////////////////////////////////////////////////////////////////////////////
int main(void) {
  hw.Configure();
  hw.Init();

  hw.SetAudioBlockSize(4);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  //
  // Initialize our Audio Components
  //
  float sample_rate = hw.AudioSampleRate();
  clock.Init(BPMToClockFreq(kDefaultBPM), sample_rate);

  mainMenu.Init(mainMenuItems, ArrayLen(mainMenuItems),
                AbstractMenu::Orientation::leftRightSelectUpDownModify, true);

  theApp.Init(sample_rate);

  // setup reverb
  verb.Init(sample_rate);
  verb.SetFeedback(0.6f);
  verb.SetLpFreq(18000.0f);

  uiOverlord.Init(sample_rate, mainPage, &hw.adc, encoderConfig, buttonConfig,
                  potConfig);

  hw.StartAudio(AudioCallback);

  float lastBPM = theApp.GetBPM();
  uint32_t lastRefreshMS = 0;
  while (1) {
    uint32_t nowMS = System::GetNow();

    if (lastBPM != theApp.GetBPM()) {
      lastBPM = theApp.GetBPM();
      SetBPM(lastBPM);
    }

    UpdateApp(nowMS);

    uiOverlord.ProcessUi();
  }
}
