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

class TheAlto : public TheVoice
{
  public:
    TheAlto(const Music::TimeSignature&   ts,
            const Music::TuningReference& tr,
            const Music::Temperament&     t,
            const Music::ScaleMap&        s)
    // -1 Relative to C4 = C3
    : TheVoice(ts, tr, t, s, -1, 0.1, 0.2, 0.4, 0.1)
    {
        SetWeights(SCALE_WEIGHTS_7_CHORD_TONE_HEAVY,
                   ArrayLen(SCALE_WEIGHTS_7_CHORD_TONE_HEAVY));
    }

    virtual const char* GetName() const override { return s_ALTO; }

    virtual size_t MakeEvents(const Music::TimeSignature& ts,
                              int                         bars,
                              Music::ChordEventSet<> &chords)
    {
        // First start with our "hit" pattern
        PatternEventSet<> pattern;
        float            density = randomRange(0.3, 0.5);
        Music::NoteValue g       = Music::NoteValue::Quarter;
        size_t maxSize = min(pattern.Count(), events.Capacity());

        GeneratePattern(ts, bars, density, g, pattern);

        events.Clear();
        for(size_t i = 0; i < maxSize; i++)
        {
            const float r            = randomRange(0.0f, 0.999999f);
            int         periodOffset = 0;

            if(pattern[i]) // Hit
            {
                events.Emplace(GetWeightedNote(r, periodOffset), periodOffset, g);
            }
            else
            {
                events.Emplace(REST, periodOffset, g);
            }
        }
        return events.Count();
    }
};
