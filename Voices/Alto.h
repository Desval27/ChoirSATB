/* SPDX-License-Identifier: CC0-1.0 */
/**
 * @file Alto.h
 * @brief Alto Voice.
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

class TheAlto : public TheVoice
{
  public:
    TheAlto(const TimeSignature&   ts,
            const TuningReference& tr,
            const Temperament&     t,
            const ScaleMap&        s)
            // -1 Relative to C4 = C3
    : TheVoice(ts, tr, t, s, -1), _freq(0.0f)
    {
        setWeights(SCALE_WEIGHTS_7_CHORD_TONE_HEAVY, ArrayLen(SCALE_WEIGHTS_7_CHORD_TONE_HEAVY));
    }

    virtual void Init(float sample_rate) override
    {
        osc.Init(sample_rate);
        osc.SetWaveform(osc.WAVE_SAW);

        env.Init(sample_rate);
        env.SetTime(ADENV_SEG_ATTACK, 0.05f);
        env.SetTime(ADENV_SEG_DECAY, 1.0f);
        env.SetMin(0.0);
        env.SetMax(1.0f);
        env.SetCurve(1); // linear

        flt.Init(sample_rate);
        flt.SetRes(0.5f);
    }

    virtual float Process() override
    {
        float env_out = env.Process();
        flt.SetFreq(
            fclamp(_freq + ((env_out * 0.75) * 5000.0f), 20.0f, 20000.0f));
        osc.SetAmp(env_out);
        return flt.Process(osc.Process());
    }

    virtual size_t makeEvents(Music::TimeSignature& ts,
                              int                   bars,
                              Music::ChordEvent*    chordEvents,
                              size_t                chordEventsLen) override
    {
        // First start with our "hit" pattern
        bool      pattern[128];
        float     density = randomRange(0.3, 0.5);
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
    {
        if(ne.note != REST && isEventRisingEdge(pulse))
        {
            env.Trigger();
        }

        _freq = getFreqForNote(ne.note, ne.period);
        osc.SetFreq(_freq);
    }

  private:
    Oscillator osc;
    MoogLadder flt;
    AdEnv      env;
    float      _freq;
};
