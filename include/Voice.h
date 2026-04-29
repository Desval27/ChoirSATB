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

static const char *s_BASS = "BASS";
static const char *s_TENOR = "TENOR";
static const char *s_ALTO = "ALTO";
static const char *s_SOPRANO = "SOPRANO";

static const char *s_SIN = "SIN";
static const char *s_TRIANGLE = "TRIANGLE";
static const char *s_SAW = "SAW";
static const char *s_RAMP = "RAMP";
static const char *s_SQUARE = "SQUARE";
static const char *s_POLY_TRIANGLE = "POLY TRIANGLE";
static const char *s_POLY_SAW = "POLY SAW";
static const char *s_POLY_SQUARE = "POLY SQUARE";
static const char *s_UNKNOWN = "????";

class TheVoice
{
public:
    TheVoice(const MyTimeSignature &ts,
             const MyTuningReference &tr,
             const MyTemperament &t,
             const MyScaleMap &s,
             int periodOffset,
             float attack,
             float decay,
             float sustain,
             float release,
             const MyWeightMap &weights)
        : timeSignature(ts),
          tuningReference(tr),
          temperament(t),
          scaleMap(s),
          weights(weights),
          config(periodOffset, daisysp::Oscillator::WAVE_TRI, attack, decay, sustain, release),
          gate_(false),
          currentNoteIndex_(-1)
    {
        events.Clear();

        const float rootC4Hz = temperament.FrequencyFromReference(Music::TemperedPitch(0, 0), tuningReference);
        const float voiceRootHz = rootC4Hz * temperament.PeriodMultiplier(config.periodOffset);

        pitchEngine.SetTemperament(&temperament);
        pitchEngine.SetScaleMap(&scaleMap);
        pitchEngine.SetRootHz(voiceRootHz);

        noteBuf_.clear();
    }

    virtual void Init(float sample_rate)
    {
        // Set envelope parameters
        osc.Init(sample_rate);
        flt.Init(sample_rate);
        ampEnv.Init(sample_rate);
    }

    virtual float Process()
    {
        osc.SetWaveform(config.waveform.Get());
        float env_out = ampEnv.Process(gate_);
        if (!std::isfinite(env_out))
            env_out = 0.0f;
        osc.SetAmp(env_out);
        const float sig = osc.Process();
        return std::isfinite(sig) ? sig : 0.0f;
    }

    virtual void Update()
    {
        if (currentNoteIndex_ < 0 || currentNoteIndex_ >= static_cast<int>(events.Count()))
        {
            noteBuf_.clear();
            return;
        }

        // Update our text here outside of the main audio event handler
        if (events[currentNoteIndex_].note == Music::REST)
        {
            noteBuf_.clear();
        }
        else
        {
            char noteName[6];
            temperament.GetNoteLabel(events[currentNoteIndex_].note, noteName, sizeof(noteName));
            noteBuf_.printf("%s-%d", noteName, 4 + config.periodOffset + events[currentNoteIndex_].period);
        }
    }

    const char *GetNoteText() const { return noteBuf_.c_str(); }

    virtual const char *GetName() const = 0;

    const char *GetWaveformName() const
    {
        switch (GetWaveform())
        {
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
        currentNoteIndex_ = events.GetEventIndexForPulse(pulse);
        if (currentNoteIndex_ < 0 || currentNoteIndex_ >= static_cast<int>(events.Count()))
        {
            gate_ = false;
            return;
        }

        const Music::NoteEvent &currentEvent = events[currentNoteIndex_];
        if (IsEventRisingEdge(pulse))
            gate_ = (currentEvent.note != Music::REST);
        else if (IsEventFallingEdge(pulse))
            gate_ = false;

        ampEnv.SetAttackTime(config.ampAdsr.attack);
        ampEnv.SetDecayTime(config.ampAdsr.decay);
        ampEnv.SetSustainLevel(config.ampAdsr.sustain);
        ampEnv.SetReleaseTime(config.ampAdsr.release);

        if (currentEvent.note != Music::REST)
        {
            HandleNoteEvent(pulse, currentEvent);
        }
    }

    bool GetGate() const { return gate_; }

    int8_t GetWaveform() const { return config.waveform.Get(); }
    virtual void SetWaveform(const int8_t value)
    {
        config.waveform.Set(value);
        osc.SetWaveform(config.waveform.Get());
    }

    float GetVolume() const { return config.volume.Get(); }
    float GetVolumeAs0to1() const { return config.volume.GetAs0to1(); }
    void SetVolume(float value) { config.volume.Set(value); }
    void SetVolumeAs0to1(float value) { config.volume.SetFrom0to1(value); }
    void AppendVolumeToString(daisy::FixedCapStrBase<char> &string) const
    {
        config.volume.AppentToString(string);
    }

    // void SetWeights(const MyWeightMap &weights)
    // {
    //     _weights = weights;
    // }

    float GetAttack() const { return config.ampAdsr.attack.Get(); }
    float GetAttackAs0to1() const { return config.ampAdsr.attack.GetAs0to1(); }
    void SetAttack(float value) { config.ampAdsr.attack.Set(value); }
    void SetAttackAs0to1(float value)
    {
        config.ampAdsr.attack.SetFrom0to1(value);
    }
    float GetDecay() const { return config.ampAdsr.decay.Get(); }
    float GetDecayAs0to1() const { return config.ampAdsr.decay.GetAs0to1(); }
    void SetDecay(float value) { config.ampAdsr.decay.Set(value); }
    void SetDecayAs0to1(float value)
    {
        config.ampAdsr.decay.SetFrom0to1(value);
    }
    float GetSustain() const { return config.ampAdsr.sustain.Get(); }
    float GetSustainAs0to1() const
    {
        return config.ampAdsr.sustain.GetAs0to1();
    }
    void SetSustain(float value) { config.ampAdsr.sustain.Set(value); }
    void SetSustainAs0to1(float value)
    {
        config.ampAdsr.sustain.SetFrom0to1(value);
    }
    float GetReleaseAs0to1() const
    {
        return config.ampAdsr.release.GetAs0to1();
    }
    void SetRelease(float value) { config.ampAdsr.release.Set(value); }
    void SetReleaseAs0to1(float value)
    {
        config.ampAdsr.release.SetFrom0to1(value);
    }

    virtual size_t MakeEvents(const MyTimeSignature &ts,
                              int bars,
                              MyChordEventSet &chords)
    {
        events.Clear();
        return events.Count();
    }

protected:
    const MyTimeSignature &timeSignature;
    const MyTuningReference &tuningReference;
    const MyTemperament &temperament;
    const MyScaleMap &scaleMap;
    const MyWeightMap &weights;
    VoiceConfig config;

    MyPitchEngine pitchEngine;

    Music::NoteEvent emptyNote;
    MyNoteEventSet events;

    daisysp::Oscillator osc;
    daisysp::MoogLadder flt;
    daisysp::Adsr ampEnv;

    float GetBaseFrequency() const { return baseFrequency_; }

    const Music::NoteEvent &GetCurrentNote() const
    {
        return events[currentNoteIndex_];
    }

    virtual void HandleNoteEvent(int pulse, Music::NoteEvent ne)
    {
        SetBaseFrequency(GetFreqForNote(ne.note, ne.period));
    }

    Music::Note GetWeightedNote(float unitRandom, int &outPeriodOffset)
    {
        return scaleMap.GetWeightedNote(unitRandom, outPeriodOffset, weights);
    }

    float GetFreqForNote(Music::Note n, Music::Period p, float fc = 0.0f) const
    {
        return pitchEngine.Frequency(Music::TemperedPitch(n, p, fc));
    }

    Music::Degree GetMappedDegreeFromRoot(Music::Degree root,
                                          int index,
                                          int &outPeriodOffset) const
    {
        int rootIdx = scaleMap.GetIndexOfDegree(root);
        return scaleMap.GetMappedDegree(rootIdx + index, outPeriodOffset);
    }

    bool IsEventRisingEdge(int pulse) const
    {
        return GetEventPulseOffset(pulse) == 0;
    }

    bool IsEventFallingEdge(int pulse, Music::Articulation articulation = Music::Articulation::Normal) const
    {
        if (events.Count() == 0 || pulse < 0)
            return false;

        const int totalPulses = events.GetTotalEventPulses();
        if (totalPulses <= 0)
            return false;

        int previousPulse = pulse - 1;
        if (pulse == 0)
        {
            if (totalPulses > 0)
                previousPulse = totalPulses - 1;
        }

        const Music::NoteEvent &currentEvent = events.GetEventForPulse(pulse);
        const Music::NoteEvent &previousEvent = events.GetEventForPulse(previousPulse);

        // Always release when the sequence transitions from note to rest.
        if (previousEvent.note != Music::REST && currentEvent.note == Music::REST)
            return true;

        // Legato keeps the gate high between adjacent note events.
        if (articulation == Music::Articulation::Legato || currentEvent.note == Music::REST)
            return false;

        const int eventIdx = events.GetEventIndexForPulse(pulse);
        if (eventIdx < 0 || eventIdx >= static_cast<int>(events.Count()))
            return false;

        const int eventPulseOffset = GetEventPulseOffset(pulse);
        if (eventPulseOffset < 0)
            return false;

        const int span = static_cast<int>(events[eventIdx].value);
        if (span <= 1)
            return true;

        float gateFraction = 0.90f; // Normal articulation.
        if (articulation == Music::Articulation::Staccato)
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
        if (events.Count() == 0 || pulse < 0)
            return -1;

        const int totalPulses = events.GetTotalEventPulses();
        if (totalPulses <= 0)
            return -1;

        const int eventIdx = events.GetEventIndexForPulse(pulse);
        if (eventIdx < 0 || eventIdx >= static_cast<int>(events.Count()))
            return -1;

        const int eventStartPulse = events.GetEventStartPulse(static_cast<size_t>(eventIdx));
        return ((pulse % totalPulses) - eventStartPulse + totalPulses) % totalPulses;
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
        if (std::isfinite(value) && value > 0.0f)
        {
            baseFrequency_ = value;
            osc.SetFreq(baseFrequency_);
        }
    }
};
