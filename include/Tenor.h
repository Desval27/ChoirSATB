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

#include "Voice.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief
class TheTenor : public TheVoice
{
public:
    TheTenor(const MyTimeSignature &ts,
             const MyTuningReference &tr,
             const MyTemperament &t,
             const MyScaleMap &s)
        // -2 Relative to C4 = C2
        : TheVoice(ts, tr, t, s, -2, 0.2, 0.2, 0.4, 0.2, Music::SCALE_WEIGHTS_7_CHORD_TONE_HEAVY)
    {
    }

    virtual const char *GetName() const override { return s_TENOR; }

    virtual size_t MakeEvents(const MyTimeSignature &ts,
                              int bars,
                              MyChordEventSet &chords)
    {
        // First start with our "hit" pattern
        MyPatternEventSet pattern;
        const Music::NoteValue g = Music::NoteValue::Quarter;
        Music::EuclidianPatternGenerator<>::GeneratePattern(ts,
                                                     bars,
                                                     randomRange(0.6, 0.9), // density
                                                     g,                     // granularity
                                                     pattern);

        events.Clear();
        for (size_t i = 0; i < pattern.Count() && !events.AtCapacity(); i++)
        {
            if (pattern[i]) // Hit
            {
                int periodOffset = 0;
                Music::Note n = GetWeightedNote(randomRange(0.0f, 0.999999f),
                                                periodOffset);
                events.Emplace(n, periodOffset, g);
            }
            else
            {
                events.Emplace(Music::REST, 0, g);
            }
        }
        return events.Count();
    }
};
