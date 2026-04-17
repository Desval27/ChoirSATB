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

////////////////////////////////////////////////////////////////////////////////
/// @brief
class TheSoprano : public TheVoice
{
  public:
    TheSoprano(const Music::TimeSignature&   ts,
               const Music::TuningReference& tr,
               const Music::Temperament&     t,
               const Music::ScaleMap&        s)
    // 0 Relative to C4 = C4
    : TheVoice(ts, tr, t, s, 0, 0.1, 0.3, 0.6, 0.3)
    {
        SetWeights(SCALE_WEIGHTS_7_UNIFORM, ArrayLen(SCALE_WEIGHTS_7_UNIFORM));
    }

    virtual const char* GetName() const override { return s_SOPRANO; }

    virtual size_t MakeEvents(const Music::TimeSignature& ts,
                              int                         bars,
                              Music::ChordEvent*          chordEvents,
                              size_t chordEventsLen) override
    {
        // First start with our "hit" pattern
        bool                   pattern[128];
        const float            density = randomRange(0.4f, 0.8f); //0.50f;
        const Music::NoteValue g       = Music::NoteValue::Eighth;
        size_t                 patternLen
            = GeneratePattern(ts, bars, density, g, pattern, ArrayLen(pattern));

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
};
