/* SPDX-License-Identifier: CC0-1.0 */
/**
 * @file Main.cpp
 * @brief
 * @author pfburdette <paul.f.burdette@gmail.com>
 *
 * @copyright This work is dedicated to the public domain under CC0 1.0.
 * To the extent possible under law, the author(s) have waived all copyright
 * and related or neighboring rights to this software.
 * See <http://creativecommons.org>
 */
#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <monkey.hpp>
#include <music/music.hpp>
#include <music/music_tables.hpp>

#include <Pages/SynthVoicePage.h>
#include <ui_overlord.hpp>

#include <ChoirSATB.h>
#include <Pages/MainPage.h>
#include <Pages/MixerPage.h>
#include <PersistedVoiceConfig.h>

using namespace daisysp;
using namespace daisy;

constexpr int ENCODER_COUNT = 1;
constexpr int BUTTON_COUNT = 5;
constexpr int POT_COUNT = 4;
constexpr int VOICE_COUNT = 4;
constexpr uint32_t SLOT_SPACING = 0x1000; // Overkill at 4k, but testing.

////////////////////////////////////////////////////////////////////////////////
// Type Aliases & Simplification
////////////////////////////////////////////////////////////////////////////////
using MyApp = ChoirSATB<VOICE_COUNT>;
using MyMainPage = MainPage<MyApp>;
using MyOverlord = UIOverlord<SSD130xI2c128x64Driver,
                              ENCODER_COUNT,
                              BUTTON_COUNT,
                              POT_COUNT,
                              ENCODER_1, // MenuEncoder
                              BUTTON_1,  // OK Button
                              BUTTON_2,  // Cancel Button
                              true>;

////////////////////////////////////////////////////////////////////////////////
// Hardware Configuration
////////////////////////////////////////////////////////////////////////////////
const MyOverlord::EncoderConfig encoderConfig[ENCODER_COUNT] = {
  { seed::D20, seed::D16 },
};
const MyOverlord::ButtonConfig buttonConfig[BUTTON_COUNT] = {
  { seed::D19 }, // Encoder
  { seed::D17 }, { seed::D18 }, { seed::D15 }, { seed::D21 },
};
const MyOverlord::PotConfig potConfig[POT_COUNT] = {
  { seed::A7 },
  { seed::A8 },
  { seed::A9 },
  { seed::A10 },
};

////////////////////////////////////////////////////////////////////////////////
// Components
////////////////////////////////////////////////////////////////////////////////
DaisySeed hw;
Metro clock;
// ReverbSc verb;

MyApp& theApp = MyApp::get_instance();
MyOverlord uiOverlord;
MyMainPage mainPage;
FullScreenItemMenu mainMenu;
MixerPage mixerPage;
SynthVoicePage voicePage;

TPersistentStorage storage_slots[VOICE_COUNT] = { { hw.qspi },
                                                  { hw.qspi },
                                                  { hw.qspi },
                                                  { hw.qspi } };

///////////////////////////////////////////////////////////////////////////////
// UI & Menu Structure
///////////////////////////////////////////////////////////////////////////////
struct OpenSynthVoicePageContext
{
  SynthVoicePage* page;
  UI* ui;
  SynthVoiceConfig* config;
};

///////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param rawContext
void
open_synth_voice_page(void* rawContext)
{
  auto* context = static_cast<OpenSynthVoicePageContext*>(rawContext);
  context->page->Bind(*context->config);
  context->ui->OpenPage(*context->page);
}

void
load_voice_config(void* rawContext)
{
  PersistedVoiceConfig defaults;
  for (int i = 0; i < VOICE_COUNT; i++) {
    uint32_t offset = i * SLOT_SPACING;
    storage_slots[i].Init(defaults, offset);
    PersistedVoiceConfig& loaded = storage_slots[i].GetSettings();
    PersistedVoiceConfig::mapTo(loaded, theApp.voices[i].config_);
  }
}

void
save_voice_config(void* rawContext)
{
  for (int i = 0; i < VOICE_COUNT; i++) {
    PersistedVoiceConfig& voice_config = storage_slots[i].GetSettings();
    PersistedVoiceConfig::mapFrom(theApp.voices[i].config_, voice_config);
    storage_slots[i].Save();
  }
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
OpenSynthVoicePageContext voicePageContexts[] = {
  { &voicePage, &uiOverlord.GetUi(), &theApp.voices[0].config_ },
  { &voicePage, &uiOverlord.GetUi(), &theApp.voices[1].config_ },
  { &voicePage, &uiOverlord.GetUi(), &theApp.voices[2].config_ },
  { &voicePage, &uiOverlord.GetUi(), &theApp.voices[3].config_ },
};

///////////////////////////////////////////////////////////////////////////////
/// @brief
AbstractMenu::ItemConfig mainMenuItems[] = {
  { .type = AbstractMenu::ItemType::openUiPageItem,
    .text = "MIXER",
    .asOpenUiPageItem{ &mixerPage } },
  { .type = AbstractMenu::ItemType::callbackFunctionItem,
    .text = "BASS",
    .asCallbackFunctionItem{ open_synth_voice_page, &voicePageContexts[0] } },
  { .type = AbstractMenu::ItemType::callbackFunctionItem,
    .text = "TENOR",
    .asCallbackFunctionItem{ open_synth_voice_page, &voicePageContexts[1] } },
  { .type = AbstractMenu::ItemType::callbackFunctionItem,
    .text = "ALTO",
    .asCallbackFunctionItem{ open_synth_voice_page, &voicePageContexts[2] } },
  { .type = AbstractMenu::ItemType::callbackFunctionItem,
    .text = "SOPRANO",
    .asCallbackFunctionItem{ open_synth_voice_page, &voicePageContexts[3] } },
  { .type = AbstractMenu::ItemType::callbackFunctionItem,
    .text = "LOAD",
    .asCallbackFunctionItem{ load_voice_config, 0 } },
  { .type = AbstractMenu::ItemType::callbackFunctionItem,
    .text = "SAVE",
    .asCallbackFunctionItem{ save_voice_config, 0 } },
  { .type = AbstractMenu::ItemType::closeMenuItem, .text = "CLOSE" },
};

///////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param bpm
void
set_bpm(float bpm)
{
  clock.SetFreq(music::BpmToFreq(bpm, PPQN));
}

///////////////////////////////////////////////////////////////////////////////
// Main Audio Loop
///////////////////////////////////////////////////////////////////////////////
void
AudioCallback(AudioHandle::InterleavingInputBuffer in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t size)
{
  uiOverlord.ProcessControls();

  // Prepare the audio block
  for (size_t i = 0; i < size; i += 2) {

    bool tick = clock.Process();

    if (theApp.get_running()) {
      if (tick)
        theApp.handle_pulse();

      auto [sigL, sigR] = theApp.process();

      out[i] = sigL;
      out[i + 1] = sigR;
    } else {
      out[i] = 0.0F;
      out[i + 1] = 0.0F;
    }

    // New pulse
    // if (theApp.GetRunning()) {
    //   if (clock.Process()) {
    //     const int pulse = theApp.DoPulse();
    //   }

    //   float sig = 0.0F;
    //   sig += theApp.Process();

    //   float wetL = 0.0F;
    //   float wetR = 0.0F;
    //   verb.Process(sig, sig, &wetL, &wetR);
    //   out[i] = (sig * 0.8F) + (wetL * 0.2F);
    //   out[i + 1] = (sig * 0.8F) + (wetR * 0.2F);
    // } else {
    //   out[i] = 0.0F;
    //   out[i + 1] = 0.0F;
    // }
  }
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param sample_rate
void
init_components(float sample_rate)
{
  theApp.init(sample_rate);
  clock.Init(music::BpmToFreq(theApp.get_bpm(), PPQN), sample_rate);

  // setup reverb
  // verb.Init(sample_rate);
  // verb.SetFeedback(0.6F);
  // verb.SetLpFreq(18000.0F);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param sample_rate
void
init_ui(float sample_rate)
{
  mainMenu.Init(mainMenuItems,
                ArrayLen(mainMenuItems),
                AbstractMenu::Orientation::leftRightSelectUpDownModify,
                true);

  uiOverlord.Init(
    sample_rate, mainPage, &hw.adc, encoderConfig, buttonConfig, potConfig);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
/// @return
int
main()
{
  hw.Configure();
  hw.Init();

  hw.SetAudioBlockSize(4);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  float sample_rate = hw.AudioSampleRate();
  init_components(sample_rate);
  init_ui(sample_rate);

  hw.StartAudio(AudioCallback);

  // float lastBPM = theApp.GetBPM();
  // uint32_t lastRefreshMS = 0;
  while (1) {
    set_bpm(theApp.get_bpm());
    uiOverlord.ProcessUi(); // Update all Ui elements and and event queues.
    theApp.update(System::GetNow());
    // uint32_t nowMS = System::GetNow();

    // if (lastBPM != theApp.GetBPM()) {
    //   lastBPM = theApp.GetBPM();
    //   set_bpm(lastBPM);
    // }

    // UpdateApp(nowMS);

    // uiOverlord.ProcessUi();
  }
}
