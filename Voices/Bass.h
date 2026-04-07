/* SPDX-License-Identifier: CC0-1.0 */
/**
 * @file Bass.h
 * @brief Bass Voice.
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

class TheBass : public TheVoice
{
  public:
    TheBass(const TimeSignature&   ts,
            const TuningReference& tr,
            const Temperament&     t,
            const ScaleMap&        s)
            // -3 Relative to C4 = C1
    : TheVoice(ts, tr, t, s, -3)
    {
        setWeights(SCALE_WEIGHTS_7_TONIC_HEAVY, ArrayLen(SCALE_WEIGHTS_7_TONIC_HEAVY));
    }

    virtual void Init(float sample_rate) override
    {
        osc.Init(sample_rate);
        osc.SetWaveform(osc.WAVE_TRI);

        //Set envelope parameters
        env.Init(sample_rate);
        env.SetTime(ADSR_SEG_ATTACK, .6);
        env.SetTime(ADSR_SEG_DECAY, .4);
        env.SetTime(ADSR_SEG_RELEASE, .1);
        env.SetSustainLevel(.40);
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
        // A direct rhythmic copy test for now.
        eventsLen = 0;
        for(size_t i = 0; i < chordEventsLen && i < ArrayLen(events); i++)
        {
            events[i].note   = chordEvents[i].root;
            events[i].period = 0;
            events[i].value  = chordEvents[i].value;
            eventsLen++;
        }
        return eventsLen;
    }

  protected:
    virtual void handleNoteEvent(int pulse, NoteEvent ne) override
    { osc.SetFreq(getFreqForNote(ne.note, ne.period)); }

  private:
    Oscillator osc;
    Adsr       env;
};
