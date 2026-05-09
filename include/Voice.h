/* SPDX-License-Identifier: CC0-1.0 */
/**
 * @file Voice.h
 * @brief Abstract Voice Definition.
 * @author pfburdette <paul.f.burdette@gmail.com>
 *
 * @copyright This work is dedicated to the public domain under CC0 1.0.
 * To the extent possible under law, the author(s) have waived all copyright
 * and related or neighboring rights to this software.
 * See <http://creativecommons.org>
 */
#pragma once

#include <cmath>

#include "daisy_seed.h"
#include "daisysp.h"

#include "Types.h"

static const char* s_BASS = "BASS";
static const char* s_TENOR = "TENOR";
static const char* s_ALTO = "ALTO";
static const char* s_SOPRANO = "SOPRANO";

static const char* s_SIN = "SIN";
static const char* s_TRIANGLE = "TRIANGLE";
static const char* s_SAW = "SAW";
static const char* s_RAMP = "RAMP";
static const char* s_SQUARE = "SQUARE";
static const char* s_POLY_TRIANGLE = "POLY TRIANGLE";
static const char* s_POLY_SAW = "POLY SAW";
static const char* s_POLY_SQUARE = "POLY SQUARE";
static const char* s_UNKNOWN = "????";

class TheVoice
{
public:
  TheVoice(const MySetup& setup_,
           const MyTuningReference& tr,
           int periodOffset,
           float attack,
           float decay,
           float sustain,
           float release,
           const MyWeightMap& weights_)
    : setup_(setup_)
    , tuningReference_(tr)
    , weights_(weights_)
    , config_(periodOffset,
              daisysp::Oscillator::WAVE_TRI,
              attack,
              decay,
              sustain,
              release)
    , gate_(false)
    , currentNoteIndex_(-1)
  {
    events_.Clear();

    const float rootC4Hz = setup_.temperament.FrequencyFromReference(
      music::TemperedPitch(0, 0), tuningReference_);
    const float voiceRootHz =
      rootC4Hz * setup_.temperament.PeriodMultiplier(config_.periodOffset);

    pitchEngine_.SetTemperament(&setup_.temperament);
    pitchEngine_.SetScaleMap(&setup_.scaleMap);
    pitchEngine_.SetRootHz(voiceRootHz);

    noteBuf_.clear();
  }

  virtual void Init(float sample_rate)
  {
    // Set envelope parameters
    osc_.Init(sample_rate);
    flt_.Init(sample_rate);
    ampEnv_.Init(sample_rate);
  }

  virtual float Process()
  {
    osc_.SetWaveform(config_.waveform.Get());
    float env_out = ampEnv_.Process(gate_);
    if (!std::isfinite(env_out))
      env_out = 0.0f;
    osc_.SetAmp(env_out);
    const float sig = osc_.Process();
    return std::isfinite(sig) ? sig : 0.0f;
  }

  virtual void Update()
  {
    if (currentNoteIndex_ < 0 ||
        currentNoteIndex_ >= static_cast<int>(events_.Count())) {
      noteBuf_.clear();
      return;
    }

    // Update our text here outside of the main audio event handler
    if (events_[currentNoteIndex_].note == music::REST) {
      noteBuf_.clear();
    } else {
      char noteName[6];
      setup_.temperament.GetNoteLabel(
        events_[currentNoteIndex_].note, noteName, sizeof(noteName));
      noteBuf_.printf("%s-%d",
                      noteName,
                      4 + config_.periodOffset +
                        events_[currentNoteIndex_].period);
    }
  }

  const char* GetNoteText() const { return noteBuf_.c_str(); }

  virtual const char* GetName() const = 0;

  const char* GetWaveformName() const
  {
    switch (GetWaveform()) {
      case daisysp::Oscillator::WAVE_SIN:
        return s_SIN;
      case daisysp::Oscillator::WAVE_TRI:
        return s_TRIANGLE;
      case daisysp::Oscillator::WAVE_SAW:
        return s_SAW;
      case daisysp::Oscillator::WAVE_RAMP:
        return s_RAMP;
      case daisysp::Oscillator::WAVE_SQUARE:
        return s_SQUARE;
      case daisysp::Oscillator::WAVE_POLYBLEP_TRI:
        return s_POLY_TRIANGLE;
      case daisysp::Oscillator::WAVE_POLYBLEP_SAW:
        return s_POLY_SAW;
      case daisysp::Oscillator::WAVE_POLYBLEP_SQUARE:
        return s_POLY_SQUARE;
      default:
        return s_UNKNOWN;
    }
  }

  void DoPulse(int pulse)
  {
    currentNoteIndex_ = events_.GetEventIndexForPulse(pulse);
    if (currentNoteIndex_ < 0 ||
        currentNoteIndex_ >= static_cast<int>(events_.Count())) {
      gate_ = false;
      return;
    }

    const music::NoteEvent& currentEvent = events_[currentNoteIndex_];
    if (IsEventRisingEdge(pulse))
      gate_ = (currentEvent.note != music::REST);
    else if (IsEventFallingEdge(pulse))
      gate_ = false;

    ampEnv_.SetAttackTime(config_.ampAdsr.attack);
    ampEnv_.SetDecayTime(config_.ampAdsr.decay);
    ampEnv_.SetSustainLevel(config_.ampAdsr.sustain);
    ampEnv_.SetReleaseTime(config_.ampAdsr.release);

    if (currentEvent.note != music::REST) {
      HandleNoteEvent(pulse, currentEvent);
    }
  }

  bool GetGate() const { return gate_; }

  int8_t GetWaveform() const { return config_.waveform.Get(); }
  virtual void SetWaveform(const int8_t value)
  {
    config_.waveform.Set(value);
    osc_.SetWaveform(config_.waveform.Get());
  }

  float GetVolume() const { return config_.volume.Get(); }
  float GetVolumeAs0to1() const { return config_.volume.GetAs0to1(); }
  void SetVolume(float value) { config_.volume.Set(value); }
  void SetVolumeAs0to1(float value) { config_.volume.SetFrom0to1(value); }
  void AppendVolumeToString(daisy::FixedCapStrBase<char>& string) const
  {
    config_.volume.AppentToString(string);
  }

  // void SetWeights(const MyWeightMap &weights_)
  // {
  //     _weights = weights_;
  // }

  float GetAttack() const { return config_.ampAdsr.attack.Get(); }
  float GetAttackAs0to1() const { return config_.ampAdsr.attack.GetAs0to1(); }
  void SetAttack(float value) { config_.ampAdsr.attack.Set(value); }
  void SetAttackAs0to1(float value)
  {
    config_.ampAdsr.attack.SetFrom0to1(value);
  }
  float GetDecay() const { return config_.ampAdsr.decay.Get(); }
  float GetDecayAs0to1() const { return config_.ampAdsr.decay.GetAs0to1(); }
  void SetDecay(float value) { config_.ampAdsr.decay.Set(value); }
  void SetDecayAs0to1(float value) { config_.ampAdsr.decay.SetFrom0to1(value); }
  float GetSustain() const { return config_.ampAdsr.sustain.Get(); }
  float GetSustainAs0to1() const { return config_.ampAdsr.sustain.GetAs0to1(); }
  void SetSustain(float value) { config_.ampAdsr.sustain.Set(value); }
  void SetSustainAs0to1(float value)
  {
    config_.ampAdsr.sustain.SetFrom0to1(value);
  }
  float GetReleaseAs0to1() const { return config_.ampAdsr.release.GetAs0to1(); }
  void SetRelease(float value) { config_.ampAdsr.release.Set(value); }
  void SetReleaseAs0to1(float value)
  {
    config_.ampAdsr.release.SetFrom0to1(value);
  }

  virtual size_t MakeEvents(MyChordEventSet& chords)
  {
    events_.Clear();
    return events_.Count();
  }

protected:
  const MySetup& setup_;
  const MyTuningReference& tuningReference_;
  const MyWeightMap& weights_;

  VoiceConfig config_;
  MyPitchEngine pitchEngine_;

  music::NoteEvent emptyNote_;
  MyNoteEventSet events_;

  daisysp::Oscillator osc_;
  daisysp::MoogLadder flt_;
  daisysp::Adsr ampEnv_;

  float GetBaseFrequency() const { return baseFrequency_; }

  const music::NoteEvent& GetCurrentNote() const
  {
    return events_[currentNoteIndex_];
  }

  virtual void HandleNoteEvent(int pulse, music::NoteEvent ne)
  {
    SetBaseFrequency(GetFreqForNote(ne.note, ne.period));
  }

  music::Note GetWeightedNote(float unitRandom, int& outPeriodOffset)
  {
    return setup_.scaleMap.GetWeightedNote(
      unitRandom, outPeriodOffset, weights_);
  }

  float GetFreqForNote(music::Note n, music::Period p, float fc = 0.0f) const
  {
    return pitchEngine_.Frequency(music::TemperedPitch(n, p, fc));
  }

  music::Degree GetMappedDegreeFromRoot(music::Degree root,
                                        int index,
                                        int& outPeriodOffset) const
  {
    int rootIdx = setup_.scaleMap.GetIndexOfDegree(root);
    return setup_.scaleMap.GetMappedDegree(rootIdx + index, outPeriodOffset);
  }

  bool IsEventRisingEdge(int pulse) const
  {
    return GetEventPulseOffset(pulse) == 0;
  }

  bool IsEventFallingEdge(
    int pulse,
    music::Articulation articulation = music::Articulation::Normal) const
  {
    if (events_.Count() == 0 || pulse < 0)
      return false;

    const int totalPulses = events_.GetTotalEventPulses();
    if (totalPulses <= 0)
      return false;

    int previousPulse = pulse - 1;
    if (pulse == 0) {
      if (totalPulses > 0)
        previousPulse = totalPulses - 1;
    }

    const music::NoteEvent& currentEvent = events_.GetEventForPulse(pulse);
    const music::NoteEvent& previousEvent =
      events_.GetEventForPulse(previousPulse);

    // Always release when the sequence transitions from note to rest.
    if (previousEvent.note != music::REST && currentEvent.note == music::REST)
      return true;

    // Legato keeps the gate high between adjacent note events_.
    if (articulation == music::Articulation::Legato ||
        currentEvent.note == music::REST)
      return false;

    const int eventIdx = events_.GetEventIndexForPulse(pulse);
    if (eventIdx < 0 || eventIdx >= static_cast<int>(events_.Count()))
      return false;

    const int eventPulseOffset = GetEventPulseOffset(pulse);
    if (eventPulseOffset < 0)
      return false;

    const int span = static_cast<int>(events_[eventIdx].value);
    if (span <= 1)
      return true;

    float gateFraction = 0.90f; // Normal articulation.
    if (articulation == music::Articulation::Staccato)
      gateFraction = 0.55f;

    int releasePulseOffset = static_cast<int>(span * gateFraction);
    if (releasePulseOffset < 0)
      releasePulseOffset = 0;
    if (releasePulseOffset > (span - 1))
      releasePulseOffset = span - 1;

    return eventPulseOffset == releasePulseOffset;
  }

  int GetEventPulseOffset(int pulse) const
  {
    if (events_.Count() == 0 || pulse < 0)
      return -1;

    const int totalPulses = events_.GetTotalEventPulses();
    if (totalPulses <= 0)
      return -1;

    const int eventIdx = events_.GetEventIndexForPulse(pulse);
    if (eventIdx < 0 || eventIdx >= static_cast<int>(events_.Count()))
      return -1;

    const int eventStartPulse =
      events_.GetEventStartPulse(static_cast<size_t>(eventIdx));
    return ((pulse % totalPulses) - eventStartPulse + totalPulses) %
           totalPulses;
  }

  // const MyTemperament &GetTemperament() const { return *_t; }
  // const MyScaleMap &GetScaleMap() const { return *_s; }

private:
  float baseFrequency_;
  bool gate_;
  int currentNoteIndex_;
  MString<16> noteBuf_;

  void SetBaseFrequency(float value)
  {
    if (std::isfinite(value) && value > 0.0f) {
      baseFrequency_ = value;
      osc_.SetFreq(baseFrequency_);
    }
  }
};
