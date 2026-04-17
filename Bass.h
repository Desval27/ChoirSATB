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
            const ScaleMap&        s)
    // -3 Relative to C4 = C1
    : TheVoice(ts, tr, t, s, -3, 0.3, 0.1, 0.7, 0.2)
    {
        SetWeights(SCALE_WEIGHTS_7_TONIC_HEAVY,
                   ArrayLen(SCALE_WEIGHTS_7_TONIC_HEAVY));
    }

    virtual const char* GetName() const override { return s_BASS; }

    virtual size_t MakeEvents(const TimeSignature& ts,
                              int                  bars,
                              ChordEvent*          chordEvents,
                              size_t               chordEventsLen) override
    {
        // A direct rhythmic copy test for now.
        eventsLen = 0;
        NoteValue len1;
        NoteValue len2;
        int       periodOffset;
        // Right now there is no bounds checking for max event length.  Will fix later.
        for(size_t i = 0; i < chordEventsLen && i < ArrayLen(events); i++)
        {
            // Each chord event will generate a constrained set events randomly
            switch(random() % 6)
            {
                default:
                case 0:
                case 1:
                    events[eventsLen].note   = chordEvents[i].root;
                    events[eventsLen].period = 0;
                    events[eventsLen].value  = chordEvents[i].value;
                    eventsLen++;
                    break;


                // Split into two equal events
                case 2:
                    len1                     = chordEvents[i].value / 2;
                    events[eventsLen].note   = chordEvents[i].root;
                    events[eventsLen].period = 0;
                    events[eventsLen].value  = len1;
                    eventsLen++;

                    // Randomly choose between the 4th and 5th for the second note.
                    if(random() % 2 == 0)
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 3, periodOffset);
                    else
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 4, periodOffset);
                    events[eventsLen].period = periodOffset;
                    events[eventsLen].value  = len1;
                    eventsLen++;
                    break;

                // Split into four equal events.
                case 3:
                case 4:
                    len1 = chordEvents[i].value / 4;

                    events[eventsLen].note   = chordEvents[i].root;
                    events[eventsLen].period = 0;
                    events[eventsLen].value  = len1;
                    eventsLen++;

                    // Randomly choose between the 4th and 5th for the second note.
                    if(random() % 2 == 0)
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 3, periodOffset);
                    else
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 4, periodOffset);
                    events[eventsLen].period = periodOffset;
                    events[eventsLen].value  = len1;
                    eventsLen++;

                    if(random() % 2 == 0)
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 2, periodOffset);
                    else
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 4, periodOffset);
                    events[eventsLen].period = periodOffset;
                    events[eventsLen].value  = len1;
                    eventsLen++;

                    if(random() % 2 == 0)
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 3, periodOffset);
                    else
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 4, periodOffset);
                    events[eventsLen].period = periodOffset;
                    events[eventsLen].value  = len1;
                    eventsLen++;
                    break;

                // Split into two events.  In 4/4 this will result in a dotted half note followed by a quarter.
                case 5:
                    len2 = min(ts.beatValue, chordEvents[i].value);
                    len1 = max(
                        ts.beatValue,
                        static_cast<NoteValue>(chordEvents[i].value - len2));
                    events[eventsLen].note   = chordEvents[i].root;
                    events[eventsLen].period = 0;
                    events[eventsLen].value  = len1;
                    eventsLen++;

                    // Randomly choose between the 4th and 5th for the second note.
                    if(random() % 2 == 0)
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 3, periodOffset);
                    else
                        events[eventsLen].note = GetMappedDegreeFromRoot(
                            chordEvents[i].root, 4, periodOffset);
                    events[eventsLen].period = periodOffset;
                    events[eventsLen].value  = len2;
                    eventsLen++;
                    break;
            }
        }
        return eventsLen;
    }

};
