#pragma once

#include <synth_voice.hpp>
#include <daisy.h>

struct PersistedVoiceConfig
{
  float volume;
  float balance;
  int period;
  int wave_form;
  float noise_level;
  float flt_freq;
  float flt_res;
  float flt_env_attack;
  float flt_env_decay;
  float flt_env_amount;
  float amp_level;
  float amp_env_attack;
  float amp_env_decay;
  float amp_env_sustain;
  float amp_env_release;
  float amp_env_amount;

  PersistedVoiceConfig()
    : volume(1.0F)
    , balance(0.0F)
    , period(0)
    , wave_form(1)
    , noise_level(0.0F)
    , flt_freq(10000.0F)
    , flt_res(0.0F)
    , flt_env_attack(0.2F)
    , flt_env_decay(1.0F)
    , flt_env_amount(1.0F)
    , amp_level(0.0F)
    , amp_env_attack(0.1F)
    , amp_env_decay(0.2F)
    , amp_env_sustain(0.6F)
    , amp_env_release(0.5F)
    , amp_env_amount(1.0F)
  //: PersistedVoiceConfig(SynthVoiceConfig())
  {
  }

  bool operator==(const PersistedVoiceConfig& other) const
  {
    return other.volume == volume && other.balance == balance &&
           other.period == period && other.wave_form == wave_form &&
           other.noise_level == noise_level && other.flt_freq == flt_freq &&
           other.flt_res == flt_res && other.flt_env_attack == flt_env_attack &&
           other.flt_env_decay == flt_env_decay &&
           other.flt_env_amount == flt_env_amount &&
           other.amp_level == amp_level &&
           other.amp_env_attack == amp_env_attack &&
           other.amp_env_decay == amp_env_decay &&
           other.amp_env_sustain == amp_env_sustain &&
           other.amp_env_release == amp_env_release &&
           other.amp_env_amount == amp_env_amount;
  }

  bool operator!=(const PersistedVoiceConfig& other) const
  {
    return !(*this == other);
  }

  // PersistedVoiceConfig(const SynthVoiceConfig& config) { *this = config; }

  // PersistedVoiceConfig& operator=(const SynthVoiceConfig& config)
  // {
  //   volume = config.volume.Get();
  //   balance = config.balance.Get();
  //   period = config.period.Get();
  //   wave_form = config.waveForm.GetIndex();
  //   noise_level = config.noiseLevel.Get();
  //   flt_freq = config.fltFreq.Get();
  //   flt_res = config.fltRes.Get();
  //   flt_env_attack = config.fltEnvelope.attack.Get();
  //   flt_env_decay = config.fltEnvelope.decay.Get();
  //   flt_env_amount = config.fltEnvelope.amount.Get();
  //   amp_level = config.ampLevel.Get();
  //   amp_env_attack = config.ampEnvelope.attack.Get();
  //   amp_env_decay = config.ampEnvelope.decay.Get();
  //   amp_env_sustain = config.ampEnvelope.sustain.Get();
  //   amp_env_release = config.ampEnvelope.release.Get();
  //   amp_env_amount = config.ampEnvelope.amount.Get();
  //   return *this;
  // }

  // operator SynthVoiceConfig() const
  // {
  //   SynthVoiceConfig config;
  //   config.volume = volume;
  //   config.balance = balance;
  //   config.period = period;
  //   config.waveForm = wave_form;
  //   config.noiseLevel = noise_level;
  //   config.fltFreq = flt_freq;
  //   config.fltRes = flt_res;
  //   config.fltEnvelope.attack = flt_env_attack;
  //   config.fltEnvelope.decay = flt_env_decay;
  //   config.fltEnvelope.amount = flt_env_amount;
  //   config.ampLevel = amp_level;
  //   config.ampEnvelope.attack = amp_env_attack;
  //   config.ampEnvelope.decay = amp_env_decay;
  //   config.ampEnvelope.sustain = amp_env_sustain;
  //   config.ampEnvelope.release = amp_env_release;
  //   config.ampEnvelope.amount = amp_env_amount;
  //   return config;
  // }

  static void mapFrom(const SynthVoiceConfig& src, PersistedVoiceConfig& dst)
  {
    dst.volume = src.volume;
    dst.balance = src.balance;
    dst.period = src.period;
    dst.wave_form = src.waveForm;
    dst.noise_level = src.noiseLevel;
    dst.flt_freq = src.fltFreq;
    dst.flt_res = src.fltRes;
    dst.flt_env_attack = src.fltEnvelope.attack;
    dst.flt_env_decay = src.fltEnvelope.decay;
    dst.flt_env_amount = src.fltEnvelope.amount;
    dst.amp_level = src.ampLevel;
    dst.amp_env_attack = src.ampEnvelope.attack;
    dst.amp_env_decay = src.ampEnvelope.decay;
    dst.amp_env_sustain = src.ampEnvelope.sustain;
    dst.amp_env_release = src.ampEnvelope.release;
    dst.amp_env_amount = src.ampEnvelope.amount;
  }

  static void mapTo(const PersistedVoiceConfig& src, SynthVoiceConfig& dst)
  {
    dst.volume = src.volume;
    dst.balance = src.balance;
    dst.period = src.period;
    dst.waveForm = src.wave_form;
    dst.noiseLevel = src.noise_level;
    dst.fltFreq = src.flt_freq;
    dst.fltRes = src.flt_res;
    dst.fltEnvelope.attack = src.flt_env_attack;
    dst.fltEnvelope.decay = src.flt_env_decay;
    dst.fltEnvelope.amount = src.flt_env_amount;
    dst.ampLevel = src.amp_level;
    dst.ampEnvelope.attack = src.amp_env_attack;
    dst.ampEnvelope.decay = src.amp_env_decay;
    dst.ampEnvelope.sustain = src.amp_env_sustain;
    dst.ampEnvelope.release = src.amp_env_release;
    dst.ampEnvelope.amount = src.amp_env_amount;
  }
};
using TPersistentStorage = daisy::PersistentStorage<PersistedVoiceConfig>;
