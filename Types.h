#pragma once

#include <daisy_seed.h>
#include <daisysp.h>

enum ButtonIds
{
    BTN_ENCODER = 0,
    BTN_RUN_STOP,
    BTN_RANDOM,
    BTN_PRIOR,
    BTN_NEXT,
    BTN_COUNT
};

enum PotIds
{
    POT_1 = 0,
    POT_2,
    POT_3,
    POT_4,
    POT_COUNT
};

enum EncoderIds
{
    ENCODER_1 = 0,
    ENCODER_COUNT
};


////////////////////////////////////////////////////////////////////////////////
/// @brief
struct SystemConfig
{
    daisy::MappedIntValue bpm;
    daisy::MappedIntValue transpose;

    SystemConfig()
    : bpm(1, 300, 60, 1, 10, "BPM", false),
      transpose(-12, 12, 0, 1, 1, "T", true)
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
             false),
      decay(0.0,
            1.0,
            decay,
            daisy::MappedFloatValue::Mapping::lin,
            "s",
            1,
            false),
      sustain(0.0,
              1.0,
              sustain,
              daisy::MappedFloatValue::Mapping::lin,
              "%",
              1,
              false),
      release(0.0,
              1.0,
              release,
              daisy::MappedFloatValue::Mapping::lin,
              "s",
              1,
              false)
    {
    }

    AdsrConfig() : AdsrConfig(0.1, 0.2, 0.6, 0.3) {}
};

struct VoiceConfig
{
    daisy::MappedIntValue   periodOffset;
    daisy::MappedIntValue   waveform;
    AdsrConfig              lpfAdsr;
    daisy::MappedFloatValue lpf;
    daisy::MappedFloatValue lpfFreq;
    daisy::MappedFloatValue lpfRes;
    daisy::MappedFloatValue amp;
    AdsrConfig              ampAdsr;

    VoiceConfig(int    periodOffset,
                int8_t waveForm,
                float  attack,
                float  decay,
                float  sustain,
                float  release)
    : periodOffset(-5, 5, periodOffset, 1, 1, "O", true),
      waveform(0,
               daisysp::Oscillator::WAVE_LAST,
               daisysp::Oscillator::WAVE_TRI,
               1,
               1,
               "W",
               false),
      lpfAdsr(attack, decay, sustain, release),
      lpf(0.0, 1.0, 0.0, daisy::MappedFloatValue::Mapping::log, "dB", 2, false),
      lpfFreq(0.0f,
              5000.0f,
              2500.0f,
              daisy::MappedFloatValue::Mapping::lin,
              "hz",
              0,
              false),
      lpfRes(0.0f,
             1.0f,
             0.0f,
             daisy::MappedFloatValue::Mapping::lin,
             "r",
             2,
             false),
      amp(0.0f,
          1.0f,
          1.0f,
          daisy::MappedFloatValue::Mapping::log,
          "dB",
          0.2,
          false),
      ampAdsr(attack, decay, sustain, release)
    {
    }
};
