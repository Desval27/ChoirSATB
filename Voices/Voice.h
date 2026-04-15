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

using namespace daisysp;
using namespace daisy;
using namespace Music;

class TheVoice
{
  public:
    enum class Articulation : uint8_t
    {
        Staccato,
        Normal,
        Legato,
    };

    TheVoice(const DaisySeed&       hw,
             const TimeSignature&   ts,
             const TuningReference& tr,
             const Temperament&     t,
             const ScaleMap&        s,
             int                    periodOffset = 0);

    virtual void  Init(float sample_rate);
    virtual float Process()               = 0;
    virtual void Update(); 

    const char* GetNoteText() const { return _noteBuf; }
    void        DoPulse(int pulse);

    void SetWeights(const float weights[], size_t weightCount);
    void SetAttack(float time) { env.SetTime(ADSR_SEG_ATTACK, time); }
    void SetDecay(float time) { env.SetTime(ADSR_SEG_DECAY, time); }
    void SetRelease(float time) { env.SetTime(ADSR_SEG_RELEASE, time); }
    void SetSustainLevel(float level) { env.SetSustainLevel(level); }

    virtual size_t MakeEvents(TimeSignature& ts,
                              int            bars,
                              ChordEvent*    chordEvents,
                              size_t         chordEventsLen)
    { return eventsLen = 0; }

  protected:
    const DaisySeed* hw;
    NoteEvent        emptyNote;
    NoteEvent        events[128];
    size_t           eventsLen;
    Adsr             env;

    // -------------------------------------------------------------------------
    bool GetGate() const { return _gate; }

    // -------------------------------------------------------------------------
    virtual void HandleNoteEvent(int pulse, NoteEvent n) = 0;

    // -------------------------------------------------------------------------
    float GetFreqForNote(Note n, Period p, float fc = 0.0f) const
    { return _pe.Frequency(TemperedPitch(n, p, fc)); }

    // -------------------------------------------------------------------------
    Note GetWeightedNote(float unitRandom, int& outPeriodOffset)
    {
        return _s->GetWeightedNote(
            unitRandom, outPeriodOffset, _weights, _weightCount);
    }

    // -------------------------------------------------------------------------
    int GetTotalEventPulses() const
    {
        int totalPulses = 0;
        for(size_t i = 0; i < eventsLen; i++)
            totalPulses += static_cast<int>(events[i].value);
        return totalPulses;
    }

    /**
     * @brief Gets the mapped degree for a given scale index, along with the period offset.
     * 
     * @param index 
     * @param outPeriodOffset 
     * @return Note 
     */
    Degree
    GetMappedDegreeFromRoot(Degree root, int index, int& outPeriodOffset) const
    {
        int rootIdx = _s->GetIndexOfDegree(root);
        return _s->GetMappedDegree(rootIdx + index, outPeriodOffset);
    }

    // -------------------------------------------------------------------------
    int GetEventIndexForPulse(int pulse) const
    { return FindAssociatedEventIndex(pulse); }

    /**
     * @brief Determines if the event at the given pulse is a rising edge, meaning the gate should be activated.
     * 
     * @param pulse 
     * @return true 
     * @return false 
     */
    bool IsEventRisingEdge(int pulse) const;

    /**
     * @brief Determines if the event at the given pulse is a falling edge, meaning the gate should be released.
     * 
     * @param pulse 
     * @param articulation 
     * @return true 
     * @return false 
     */
    bool IsEventFallingEdge(int          pulse,
                            Articulation articulation
                            = Articulation::Normal) const;

    const Music::NoteEvent& GetEventForPulse(int pulse) const;

  private:
    const TimeSignature*   _ts;
    const TuningReference* _tr;
    const Temperament*     _t;
    const ScaleMap*        _s;
    int                    _periodOffset;
    const float*           _weights;
    size_t                 _weightCount;
    PitchEngine            _pe;
    bool                   _gate;
    NoteEvent              _currentNote;
    char                   _noteBuf[16];

    int GetEventStartPulse(size_t eventIndex) const;
    int FindAssociatedEventIndex(int pulse) const;
};
