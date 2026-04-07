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
    // const TimeSignature*   _ts;
    // const TuningReference* _tr;
    // const Temperament*     _t;
    // const ScaleMap*        _s;
    // const float*           _weights;
    // size_t                 _weightCount;
    // PitchEngine            _pe;
    // bool                   _gate;

    TheVoice(const TimeSignature&   ts,
             const TuningReference& tr,
             const Temperament&     t,
             const ScaleMap&        s,
             int                    periodOffset = 0)
    : eventsLen(0), _ts(&ts), _tr(&tr), _t(&t), _weightCount(0), _gate(false)
    {
        const float rootC4Hz
            = _t->frequencyFromReference(TemperedPitch(0, 0), *_tr);
        const float voiceRootHz = rootC4Hz * _t->periodMultiplier(periodOffset);
        _pe.setTemperament(_t);
        _pe.setScaleMap(_s);
        _pe.setRootHz(voiceRootHz);
    }

    virtual void  Init(float sample_rate) = 0;
    virtual float Process()               = 0;

    void doPulse(int pulse)
    {
        NoteEvent ne = getEventForPulse(pulse);
        if(ne.note != REST && isEventRisingEdge(pulse))
            _gate = true;
        if(isEventFallingEdge(pulse))
            _gate = false;

        if(ne.note != REST)
        {
            handleNoteEvent(pulse, ne);
        }
    }

    // void setDegrees(const Degree degrees[], uint16_t degreeCount)
    // { scale.setDegrees(degrees, degreeCount); }
    void setWeights(const float weights[], size_t weightCount)
    {
        _weights = weights;
        _weightCount = weightCount;
    }

    virtual size_t makeEvents(TimeSignature& ts,
                              int            bars,
                              ChordEvent*    chordEvents,
                              size_t         chordEventsLen)
    { return eventsLen = 0; }

  protected:
    NoteEvent emptyNote;
    NoteEvent events[128];
    size_t    eventsLen;

    // -------------------------------------------------------------------------
    bool getGate() const { return _gate; }

    // -------------------------------------------------------------------------
    virtual void handleNoteEvent(int pulse, NoteEvent n) = 0;

    // -------------------------------------------------------------------------
    float getFreqForNote(Note n, Period p, float fc = 0.0f) const
    { return _pe.frequency(TemperedPitch(n, p, fc)); }

    // -------------------------------------------------------------------------
    Note getWeightedNote(float unitRandom, int& outPeriodOffset)
    {
        return _s->getWeightedNote(
            unitRandom, outPeriodOffset, _weights, _weightCount);
    }

    // -------------------------------------------------------------------------
    int getTotalEventPulses() const
    {
        int totalPulses = 0;
        for(size_t i = 0; i < eventsLen; i++)
            totalPulses += static_cast<int>(events[i].value);
        return totalPulses;
    }

    // -------------------------------------------------------------------------
    int getEventIndexForPulse(int pulse) const
    { return findAssociatedEventIndex(pulse); }

    // -------------------------------------------------------------------------
    bool isEventRisingEdge(int pulse) const
    {
        if(eventsLen == 0 || pulse < 0)
            return false;

        const int currentEventIdx = getEventIndexForPulse(pulse);
        if(currentEventIdx < 0)
            return false;

        int previousPulse = pulse - 1;
        if(pulse == 0)
        {
            const int totalPulses = getTotalEventPulses();
            if(totalPulses > 0)
                previousPulse = totalPulses - 1;
        }

        const int previousEventIdx = getEventIndexForPulse(previousPulse);
        return currentEventIdx != previousEventIdx;
    }

    // -------------------------------------------------------------------------
    bool isEventFallingEdge(int          pulse,
                            Articulation articulation
                            = Articulation::Normal) const
    {
        if(eventsLen == 0 || pulse < 0)
            return false;

        const int totalPulses = getTotalEventPulses();
        if(totalPulses <= 0)
            return false;

        int previousPulse = pulse - 1;
        if(pulse == 0)
        {
            if(totalPulses > 0)
                previousPulse = totalPulses - 1;
        }

        const NoteEvent& currentEvent  = getEventForPulse(pulse);
        const NoteEvent& previousEvent = getEventForPulse(previousPulse);

        // Always release when the sequence transitions from note to rest.
        if(previousEvent.note != REST && currentEvent.note == REST)
            return true;

        // Legato keeps the gate high between adjacent note events.
        if(articulation == Articulation::Legato || currentEvent.note == REST)
            return false;

        const int eventIdx = getEventIndexForPulse(pulse);
        if(eventIdx < 0 || eventIdx >= static_cast<int>(eventsLen))
            return false;

        const int eventStartPulse
            = getEventStartPulse(static_cast<size_t>(eventIdx));
        const int eventPulseOffset
            = ((pulse % totalPulses) - eventStartPulse + totalPulses)
              % totalPulses;
        const int span = static_cast<int>(events[eventIdx].value);
        if(span <= 1)
            return true;

        float gateFraction = 0.90f; // Normal articulation.
        if(articulation == Articulation::Staccato)
            gateFraction = 0.55f;

        int releasePulseOffset = static_cast<int>(span * gateFraction);
        if(releasePulseOffset < 0)
            releasePulseOffset = 0;
        if(releasePulseOffset > (span - 1))
            releasePulseOffset = span - 1;

        return eventPulseOffset == releasePulseOffset;
    }

    // -------------------------------------------------------------------------
    bool IsEventFallingEdge(int          pulse,
                            Articulation articulation
                            = Articulation::Normal) const
    { return isEventFallingEdge(pulse, articulation); }

    // -------------------------------------------------------------------------
    const Music::NoteEvent& getEventForPulse(int pulse) const
    {
        int eventIdx = findAssociatedEventIndex(pulse);
        if(eventIdx >= 0 && eventIdx < static_cast<int>(eventsLen))
            return events[eventIdx];
        return emptyNote;
    }

  private:
    const TimeSignature*   _ts;
    const TuningReference* _tr;
    const Temperament*     _t;
    const ScaleMap*        _s;
    const float*           _weights;
    size_t                 _weightCount;
    PitchEngine            _pe;
    bool                   _gate;

    // -------------------------------------------------------------------------
    int getEventStartPulse(size_t eventIndex) const
    {
        int pulseCursor = 0;
        for(size_t i = 0; i < eventIndex && i < eventsLen; i++)
            pulseCursor += static_cast<int>(events[i].value);
        return pulseCursor;
    }

    // -------------------------------------------------------------------------
    int findAssociatedEventIndex(int pulse) const
    {
        if(eventsLen == 0 || pulse < 0)
            return -1;

        int totalPulses = getTotalEventPulses();

        if(totalPulses <= 0)
            return -1;

        const int normalizedPulse = pulse % totalPulses;

        int pulseCursor = 0;
        for(size_t i = 0; i < eventsLen; i++)
        {
            const int span = static_cast<int>(events[i].value);
            if(normalizedPulse < (pulseCursor + span))
                return static_cast<int>(i);

            pulseCursor += span;
        }

        return static_cast<int>(eventsLen - 1);
    }
};
