#pragma once

#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/MusicTemplates.h>

#include <ChoirSATB.h>

///////////////////////////////////////////////////////////////////////////////
// Aliases
///////////////////////////////////////////////////////////////////////////////
using MyApp = ChoirSATB<4>;
using MyDisplay = daisy::OledDisplay<daisy::SSD130xI2c128x64Driver>;
using MySetup = music::Setup<>;
using MyPitchEngine = music::PitchEngine<>;

using MyTuningReference = music::TuningReference;
using MyWeightMap = music::WeightMap<music::HEPATONIC>;
using MyChordEvent = music::ChordEvent<>;
using MyChordEventSet = music::ChordEventSet<>;
using MyNoteEventSet = music::NoteEventSet<>;
using MyPatternEventSet = music::PatternEventSet<>;
using MyEuclidianPatternGenerator = music::EuclidianPatternGenerator<>;
using MySimpleRandomPatternGenerator = music::SimpleRandomPatternGenerator<>;
using MyStyleANoteGenerator = music::StyleANoteGenerator<>;

#if false
constexpr int BASS_REGISTER = -3;
constexpr int TENOR_REGISTER = -2;
constexpr int ALTO_REGISTER = -1;
constexpr int SOPRANO_REGISTER = 0;
#endif

#if false
////////////////////////////////////////////////////////////////////////////////
/// @brief
struct SystemConfig
{
  daisy::MappedIntValue bpm;
  daisy::MappedIntValue transpose;

  SystemConfig()
    : bpm(30, 120, 60, 1, 10, "BPM", false)
    , transpose(-12, 12, 0, 1, 1, "T", true)
  {
  }
};

struct AdsrConfig
{
  daisy::MappedFloatValue attack;
  daisy::MappedFloatValue decay;
  daisy::MappedFloatValue sustain;
  daisy::MappedFloatValue release;

  AdsrConfig(float attack, float decay, float sustain, float release)
    : attack(0.0,
             1.0,
             attack,
             daisy::MappedFloatValue::Mapping::lin,
             "s",
             1,
             false)
    , decay(0.0,
            1.0,
            decay,
            daisy::MappedFloatValue::Mapping::lin,
            "s",
            1,
            false)
    , sustain(0.0,
              1.0,
              sustain,
              daisy::MappedFloatValue::Mapping::lin,
              "%",
              1,
              false)
    , release(0.0,
              1.0,
              release,
              daisy::MappedFloatValue::Mapping::lin,
              "s",
              1,
              false)
  {
  }

  AdsrConfig()
    : AdsrConfig(0.1, 0.2, 0.6, 0.3)
  {
  }
};

struct VoiceConfig
{
  daisy::MappedIntValue periodOffset;
  daisy::MappedFloatValue volume;
  daisy::MappedIntValue waveform;
  AdsrConfig lpfAdsr;
  daisy::MappedFloatValue lpf;
  daisy::MappedFloatValue lpfFreq;
  daisy::MappedFloatValue lpfRes;
  daisy::MappedFloatValue amp;
  AdsrConfig ampAdsr;

  VoiceConfig(int periodOffset,
              int8_t waveForm,
              float attack,
              float decay,
              float sustain,
              float release)
    : periodOffset(-5, 5, periodOffset, 1, 1, "O", true)
    , volume(0, 1.0, 1.0, daisy::MappedFloatValue::Mapping::log, "dB", 2, false)
    , waveform(0,
               daisysp::Oscillator::WAVE_LAST,
               daisysp::Oscillator::WAVE_TRI,
               1,
               1,
               "W",
               false)
    , lpfAdsr(attack, decay, sustain, release)
    , lpf(0.0, 1.0, 0.0, daisy::MappedFloatValue::Mapping::log, "dB", 2, false)
    , lpfFreq(0.0f,
              5000.0f,
              2500.0f,
              daisy::MappedFloatValue::Mapping::lin,
              "hz",
              0,
              false)
    , lpfRes(0.0f,
             1.0f,
             0.0f,
             daisy::MappedFloatValue::Mapping::lin,
             "r",
             2,
             false)
    , amp(0.0f,
          1.0f,
          1.0f,
          daisy::MappedFloatValue::Mapping::log,
          "dB",
          0.2,
          false)
    , ampAdsr(attack, decay, sustain, release)
  {
  }

};

#endif