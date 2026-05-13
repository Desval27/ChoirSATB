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
  bool flt_key_track;
  int flt_mode;
  float flt_env_sustain;
  float flt_env_release;

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
    , flt_key_track(false)
    , flt_mode(FILTER_MODE_LOW)
    , flt_env_sustain(0.6F)
    , flt_env_release(0.5F)
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
           other.amp_env_amount == amp_env_amount &&
           other.flt_key_track == flt_key_track &&
           other.flt_mode == flt_mode &&
           other.flt_env_sustain == flt_env_sustain &&
           other.flt_env_release == flt_env_release;
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
  //   noise_level = config.noise_level.Get();
  //   flt_freq = config.flt_freq.Get();
  //   flt_key_track = config.flt_key_track;
  //   flt_mode = config.flt_mode.GetIndex();
  //   flt_res = config.flt_res.Get();
  //   flt_env_attack = config.flt_envelope.attack.Get();
  //   flt_env_decay = config.flt_envelope.decay.Get();
  //   flt_env_sustain = config.flt_envelope.sustain.Get();
  //   flt_env_release = config.flt_envelope.release.Get();
  //   flt_env_amount = config.flt_envelope.amount.Get();
  //   amp_level = config.amp_level.Get();
  //   amp_env_attack = config.amp_envelope.attack.Get();
  //   amp_env_decay = config.amp_envelope.decay.Get();
  //   amp_env_sustain = config.amp_envelope.sustain.Get();
  //   amp_env_release = config.amp_envelope.release.Get();
  //   amp_env_amount = config.amp_envelope.amount.Get();
  //   return *this;
  // }

  // operator SynthVoiceConfig() const
  // {
  //   SynthVoiceConfig config;
  //   config.volume = volume;
  //   config.balance = balance;
  //   config.period = period;
  //   config.waveForm = wave_form;
  //   config.noise_level = noise_level;
  //   config.flt_freq = flt_freq;
  //   config.flt_key_track = flt_key_track;
  //   config.flt_mode = flt_mode;
  //   config.flt_res = flt_res;
  //   config.flt_envelope.attack = flt_env_attack;
  //   config.flt_envelope.decay = flt_env_decay;
  //   config.flt_envelope.sustain = flt_env_sustain;
  //   config.flt_envelope.release = flt_env_release;
  //   config.flt_envelope.amount = flt_env_amount;
  //   config.amp_level = amp_level;
  //   config.amp_envelope.attack = amp_env_attack;
  //   config.amp_envelope.decay = amp_env_decay;
  //   config.amp_envelope.sustain = amp_env_sustain;
  //   config.amp_envelope.release = amp_env_release;
  //   config.amp_envelope.amount = amp_env_amount;
  //   return config;
  // }

  static void mapFrom(const SynthVoiceConfig& src, PersistedVoiceConfig& dst)
  {
    dst.volume = src.volume;
    dst.balance = src.balance;
    dst.period = src.period;
    dst.wave_form = src.wave_form;
    dst.noise_level = src.noise_level;
    dst.flt_freq = src.flt_freq;
    dst.flt_key_track = src.flt_key_track;
    dst.flt_mode = src.flt_mode;
    dst.flt_res = src.flt_res;
    dst.flt_env_attack = src.flt_envelope.attack;
    dst.flt_env_decay = src.flt_envelope.decay;
    dst.flt_env_sustain = src.flt_envelope.sustain;
    dst.flt_env_release = src.flt_envelope.release;
    dst.flt_env_amount = src.flt_envelope.amount;
    dst.amp_level = src.amp_level;
    dst.amp_env_attack = src.amp_envelope.attack;
    dst.amp_env_decay = src.amp_envelope.decay;
    dst.amp_env_sustain = src.amp_envelope.sustain;
    dst.amp_env_release = src.amp_envelope.release;
    dst.amp_env_amount = src.amp_envelope.amount;
  }

  static void mapTo(const PersistedVoiceConfig& src, SynthVoiceConfig& dst)
  {
    dst.volume = src.volume;
    dst.balance = src.balance;
    dst.period = src.period;
    dst.wave_form = src.wave_form;
    dst.noise_level = src.noise_level;
    dst.flt_freq = src.flt_freq;
    dst.flt_key_track = src.flt_key_track;
    dst.flt_mode = src.flt_mode;
    dst.flt_res = src.flt_res;
    dst.flt_envelope.attack = src.flt_env_attack;
    dst.flt_envelope.decay = src.flt_env_decay;
    dst.flt_envelope.sustain = src.flt_env_sustain;
    dst.flt_envelope.release = src.flt_env_release;
    dst.flt_envelope.amount = src.flt_env_amount;
    dst.amp_level = src.amp_level;
    dst.amp_envelope.attack = src.amp_env_attack;
    dst.amp_envelope.decay = src.amp_env_decay;
    dst.amp_envelope.sustain = src.amp_env_sustain;
    dst.amp_envelope.release = src.amp_env_release;
    dst.amp_envelope.amount = src.amp_env_amount;
  }
};
using TPersistentStorage = daisy::PersistentStorage<PersistedVoiceConfig>;
