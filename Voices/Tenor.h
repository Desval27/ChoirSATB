/* SPDX-License-Identifier: CC0-1.0 */
/**
 * @file Tenor.h
 * @brief Tenor Voice.
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
#include <Music/ScaleMaps.h>

#include "Voice.h"

using namespace daisysp;
using namespace daisy;
using namespace Music;

class TheTenor : public TheVoice
{
  public:
    TheTenor(const TimeSignature&   ts,
            const TuningReference& tr,
            const Temperament&     t,
            const ScaleMap&        s)
            // -2 Relative to C4 = C2
    : TheVoice(ts, tr, t, s, -2)
    {
        setWeights(SCALE_WEIGHTS_7_CHORD_TONE_HEAVY, ArrayLen(SCALE_WEIGHTS_7_CHORD_TONE_HEAVY));
    }

    virtual void Init(float sample_rate) override
    {
        osc.Init(sample_rate);
        osc.SetWaveform(osc.WAVE_TRI);

        //Set envelope parameters
        env.Init(sample_rate);
        env.SetTime(ADSR_SEG_ATTACK, .1);
        env.SetTime(ADSR_SEG_DECAY, .1);
        env.SetTime(ADSR_SEG_RELEASE, .1);

        env.SetSustainLevel(.70);
    }

    virtual float Process() override
    {
        float env_out = env.Process(getGate());
        osc.SetAmp(env_out);
        return osc.Process();
    }

    virtual size_t makeEvents(Music::TimeSignature& ts,
                              int                   bars,
                              Music::ChordEvent*    chordEvents,
                              size_t                chordEventsLen) override
    {
        // First start with our "hit" pattern
        bool      pattern[128];
        float     density = randomRange(0.6, 0.9);
        NoteValue g       = NoteValue::Quarter;
        size_t    patternLen
            = generatePattern(ts, bars, density, g, pattern, ArrayLen(pattern));

        eventsLen = 0;
        for(size_t i = 0; i < patternLen && eventsLen < ArrayLen(events); i++)
        {
            const float r            = randomRange(0.0f, 0.999999f);
            int         periodOffset = 0;

            if(pattern[i]) // Hit
            {
                events[eventsLen].note = getWeightedNote(r, periodOffset);
                events[eventsLen].period = periodOffset;
                events[eventsLen].value  = g;
            }
            else
            {
                events[eventsLen].note   = REST;
                events[eventsLen].period = periodOffset;
                events[eventsLen].value  = g;
            }
            eventsLen++;
        }
        // debugNoteEvents(ts, events, eventsLen);
        return eventsLen;
    }

  protected:
    virtual void handleNoteEvent(int pulse, NoteEvent ne) override
    { osc.SetFreq(getFreqForNote(ne.note, ne.period)); }

  private:
    Oscillator osc;
    Adsr       env;
};
