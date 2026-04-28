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

////////////////////////////////////////////////////////////////////////////////
/// @brief
class TheBass : public TheVoice
{
  public:
    TheBass(const TimeSignature&   ts,
            const TuningReference& tr,
            const Temperament&     t,
            const ScaleMap<>&      s)
    // -3 Relative to C4 = C1
    : TheVoice(ts, tr, t, s, -3, 0.3, 0.1, 0.7, 0.2)
    {
        SetWeights(SCALE_WEIGHTS_7_TONIC_HEAVY);
    }

    virtual const char* GetName() const override { return s_BASS; }

    virtual size_t MakeEvents(const TimeSignature&    ts,
                              int                     bars,
                              Music::ChordEventSet<>& chords) override
    {
        // A direct rhythmic copy test for now.
        size_t    maxSize = min(chords.Count(), events.Capacity());
        NoteValue len1;
        NoteValue len2;
        Note      n;
        int       periodOffset = 0;

        events.Clear();
        for(size_t i = 0; i < maxSize; i++)
        {
            // Each chord event will generate a constrained set events randomly
            switch(random() % 6)
            {
                default:
                case 0:
                case 1:
                    n    = chords[i].root;
                    len1 = chords[i].value;
                    events.Emplace(n, 0, len1);
                    break;


                // Split into two equal events
                case 2:
                    n    = chords[i].root;
                    len1 = static_cast<NoteValue>(chords[i].value / 2);
                    events.Emplace(n, 0, len1);

                    // Randomly choose between the 4th and 5th for the second note.
                    if(random() % 2 == 0)
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 3, periodOffset);
                    else
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 4, periodOffset);
                    events.Emplace(n, periodOffset, len1);
                    break;

                // Split into four equal events.
                case 3:
                case 4:
                    n    = chords[i].root;
                    len1 = static_cast<NoteValue>(chords[i].value / 4);

                    events.Emplace(n, 0, len1);

                    // Randomly choose between the 4th and 5th for the second note.
                    if(random() % 2 == 0)
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 3, periodOffset);
                    else
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 4, periodOffset);
                    events.Emplace(n, periodOffset, len1);

                    if(random() % 2 == 0)
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 2, periodOffset);
                    else
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 4, periodOffset);
                    events.Emplace(n, periodOffset, len1);

                    if(random() % 2 == 0)
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 3, periodOffset);
                    else
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 4, periodOffset);
                    events.Emplace(n, periodOffset, len1);
                    break;

                // Split into two events.  In 4/4 this will result in a dotted half note followed by a quarter.
                case 5:
                    n    = chords[i].root;
                    len2 = min(ts.beatValue, chords[i].value);
                    len1 = max(ts.beatValue, static_cast<NoteValue>(chords[i].value - len2));
                    events.Emplace(n, 0, len1);

                    // Randomly choose between the 4th and 5th for the second note.
                    if(random() % 2 == 0)
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 3, periodOffset);
                    else
                        n = GetMappedDegreeFromRoot(
                            chords[i].root, 4, periodOffset);
                    events.Emplace(n, periodOffset, len2);
                    break;
            }
        }
        return events.Count();
    }
};
