/* SPDX-License-Identifier: CC0-1.0 */
/**
 * @file Soprano.h
 * @brief Soprano Voice.
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

class TheSoprano : public TheVoice
{
  public:
    TheSoprano(const DaisySeed&       hw,
               const TimeSignature&   ts,
               const TuningReference& tr,
               const Temperament&     t,
               const ScaleMap&        s)
    // 0 Relative to C4 = C4
    : TheVoice(hw, ts, tr, t, s, 0)
    { SetWeights(SCALE_WEIGHTS_7_UNIFORM, ArrayLen(SCALE_WEIGHTS_7_UNIFORM)); }

    virtual void Init(float sample_rate) override
    {
        TheVoice::Init(sample_rate);

        osc.Init(sample_rate);
        osc.SetWaveform(osc.WAVE_TRI);
    }

    virtual float Process() override
    {
        float env_out = env.Process(GetGate());
        osc.SetAmp(env_out);
        return osc.Process();
    }

    virtual size_t MakeEvents(Music::TimeSignature& ts,
                              int                   bars,
                              Music::ChordEvent*    chordEvents,
                              size_t                chordEventsLen) override
    {
        // First start with our "hit" pattern
        bool            pattern[128];
        const float     density    = randomRange(0.4f, 0.8f); //0.50f;
        const NoteValue g          = NoteValue::Eighth;
        size_t          patternLen = Music::GeneratePattern(
            ts, bars, density, g, pattern, ArrayLen(pattern));

        eventsLen = 0;
        for(size_t i = 0; i < patternLen && eventsLen < ArrayLen(events); i++)
        {
            const float r            = randomRange(0.0f, 0.999999f);
            int         periodOffset = 0;

            if(pattern[i]) // Hit
            {
                events[eventsLen].note   = GetWeightedNote(r, periodOffset);
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
        return eventsLen;
    }

  protected:
    virtual void HandleNoteEvent(int pulse, NoteEvent ne) override
    { osc.SetFreq(GetFreqForNote(ne.note, ne.period)); }


  private:
    Oscillator osc;
};
