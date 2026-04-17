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

#include "daisy_seed.h"
#include "daisysp.h"

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/Temperament.h>

#include "Types.h"

static const char* s_BASS    = "BASS";
static const char* s_TENOR   = "TENOR";
static const char* s_ALTO    = "ALTO";
static const char* s_SOPRANO = "SOPRANO";

static const char* s_SIN           = "SIN";
static const char* s_TRIANGLE      = "TRIANGLE";
static const char* s_SAW           = "SAW";
static const char* s_RAMP          = "RAMP";
static const char* s_SQUARE        = "SQUARE";
static const char* s_POLY_TRIANGLE = "POLY TRIANGLE";
static const char* s_POLY_SAW      = "POLY SAW";
static const char* s_POLY_SQUARE   = "POLY SQUARE";
static const char* s_UNKNOWN       = "????";

class TheVoice
{
  public:
    TheVoice(const Music::TimeSignature&   ts,
             const Music::TuningReference& tr,
             const Music::Temperament&     t,
             const Music::ScaleMap&        s,
             int                           periodOffset = 0,
             float                         attack       = 0.1,
             float                         decay        = 0.2,
             float                         sustain      = 0.5,
             float                         release      = 0.2);

    virtual void  Init(float sample_rate);
    virtual float Process();
    virtual void  Update();

    const char*         GetNoteText() const { return _noteBuf; }
    virtual const char* GetName() const = 0;
    const char*         GetWaveformName() const
    {
        switch(GetWaveform())
        {
            case daisysp::Oscillator::WAVE_SIN: return s_SIN;
            case daisysp::Oscillator::WAVE_TRI: return s_TRIANGLE;
            case daisysp::Oscillator::WAVE_SAW: return s_SAW;
            case daisysp::Oscillator::WAVE_RAMP: return s_RAMP;
            case daisysp::Oscillator::WAVE_SQUARE: return s_SQUARE;
            case daisysp::Oscillator::WAVE_POLYBLEP_TRI: return s_POLY_TRIANGLE;
            case daisysp::Oscillator::WAVE_POLYBLEP_SAW: return s_POLY_SAW;
            case daisysp::Oscillator::WAVE_POLYBLEP_SQUARE:
                return s_POLY_SQUARE;
            default: return s_UNKNOWN;
        }
    }

    void DoPulse(int pulse);
    bool GetGate() const { return _gate; }

    int8_t       GetWaveform() const { return config.waveform.Get(); }
    virtual void SetWaveform(const int8_t value) 
    { 
        config.waveform.Set(value);
        osc.SetWaveform(config.waveform.Get()); 
    }

    float GetVolume() const { return config.volume.Get(); }
    float GetVolumeAs0to1() const { return config.volume.GetAs0to1(); }
    void SetVolume(float value) { config.volume.Set(value); }
    void SetVolumeAs0to1(float value) { config.volume.SetFrom0to1(value); }
    void AppendVolumeToString(daisy::FixedCapStrBase<char>& string) const { config.volume.AppentToString(string); }

    void SetWeights(const float weights[], size_t weightCount);

    float GetAttack() const { return config.ampAdsr.attack.Get(); }
    float GetAttackAs0to1() const { return config.ampAdsr.attack.GetAs0to1(); }
    void  SetAttack(float value) { config.ampAdsr.attack.Set(value); }
    void  SetAttackAs0to1(float value)
    {
        config.ampAdsr.attack.SetFrom0to1(value);
    }
    float GetDecay() const { return config.ampAdsr.decay.Get(); }
    float GetDecayAs0to1() const { return config.ampAdsr.decay.GetAs0to1(); }
    void  SetDecay(float value) { config.ampAdsr.decay.Set(value); }
    void  SetDecayAs0to1(float value)
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

    virtual size_t MakeEvents(const Music::TimeSignature& ts,
                              int                         bars,
                              Music::ChordEvent*          chordEvents,
                              size_t                      chordEventsLen)
    {
        return eventsLen = 0;
    }

  protected:
    VoiceConfig         config;
    Music::NoteEvent    emptyNote;
    Music::NoteEvent    events[128];
    size_t              eventsLen;

    daisysp::Oscillator osc;
    daisysp::MoogLadder flt;
    daisysp::Adsr       ampEnv;

    virtual void HandleNoteEvent(int pulse, Music::NoteEvent ne);

    Music::Note GetWeightedNote(float unitRandom, int& outPeriodOffset);
    int         GetTotalEventPulses() const;
    float GetFreqForNote(Music::Note n, Music::Period p, float fc = 0.0f) const;

    Music::Degree GetMappedDegreeFromRoot(Music::Degree root,
                                          int           index,
                                          int&          outPeriodOffset) const;

    int  GetEventIndexForPulse(int pulse) const;
    bool IsEventRisingEdge(int pulse) const;
    bool IsEventFallingEdge(int                 pulse,
                            Music::Articulation articulation
                            = Music::Articulation::Normal) const;

    const Music::NoteEvent& GetEventForPulse(int pulse) const;

  private:
    const Music::TimeSignature*   _ts;
    const Music::TuningReference* _tr;
    const Music::Temperament*     _t;
    const Music::ScaleMap*        _s;
    const float*                  _weights;
    size_t                        _weightCount;
    Music::PitchEngine            _pe;
    bool                          _gate;
    Music::NoteEvent              _currentNote;
    char                          _noteBuf[16];

    int GetEventStartPulse(size_t eventIndex) const;
    int FindAssociatedEventIndex(int pulse) const;
};
