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

#include "Voice.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief
class TheBass : public TheVoice
{
public:
  TheBass(const MySetup& setup, const MyTuningReference& tr)
    // -3 Relative to C4 = C1
    : TheVoice(setup,
               tr,
               BASS_REGISTER,
               0.3,
               0.1,
               0.7,
               0.2,
               Music::SCALE_WEIGHTS_7_TONIC_HEAVY)
  {
  }

  virtual const char* GetName() const override { return s_BASS; }

  virtual size_t MakeEvents(MyChordEventSet& chords) override
  {
    // A direct rhythmic copy test for now.
    size_t maxSize = min(chords.Count(), events_.Capacity());
    Music::NoteValue len1;
    Music::NoteValue len2;
    Music::Note n;
    int periodOffset = 0;

    events_.Clear();
    for (size_t i = 0; i < maxSize; i++) {
      // Each chord event will generate a constrained set events_ randomly
      switch (random() % 6) {
        default:
        case 0:
        case 1:
          n = chords[i].root;
          len1 = chords[i].value;
          events_.Emplace(n, 0, len1);
          break;

        // Split into two equal events_
        case 2:
          n = chords[i].root;
          len1 = static_cast<Music::NoteValue>(chords[i].value / 2);
          events_.Emplace(n, 0, len1);

          // Randomly choose between the 4th and 5th for the second note.
          if (random() % 2 == 0)
            n = GetMappedDegreeFromRoot(chords[i].root, 3, periodOffset);
          else
            n = GetMappedDegreeFromRoot(chords[i].root, 4, periodOffset);
          events_.Emplace(n, periodOffset, len1);
          break;

        // Split into four equal events_.
        case 3:
        case 4:
          n = chords[i].root;
          len1 = static_cast<Music::NoteValue>(chords[i].value / 4);

          events_.Emplace(n, 0, len1);

          // Randomly choose between the 4th and 5th for the second note.
          if (random() % 2 == 0)
            n = GetMappedDegreeFromRoot(chords[i].root, 3, periodOffset);
          else
            n = GetMappedDegreeFromRoot(chords[i].root, 4, periodOffset);
          events_.Emplace(n, periodOffset, len1);

          if (random() % 2 == 0)
            n = GetMappedDegreeFromRoot(chords[i].root, 2, periodOffset);
          else
            n = GetMappedDegreeFromRoot(chords[i].root, 4, periodOffset);
          events_.Emplace(n, periodOffset, len1);

          if (random() % 2 == 0)
            n = GetMappedDegreeFromRoot(chords[i].root, 3, periodOffset);
          else
            n = GetMappedDegreeFromRoot(chords[i].root, 4, periodOffset);
          events_.Emplace(n, periodOffset, len1);
          break;

        // Split into two events_.  In 4/4 this will result in a dotted half
        // note followed by a quarter.
        case 5:
          n = chords[i].root;
          len2 = min(setup_.timeSignature.beatValue, chords[i].value);
          len1 = max(setup_.timeSignature.beatValue,
                     static_cast<Music::NoteValue>(chords[i].value - len2));
          events_.Emplace(n, 0, len1);

          // Randomly choose between the 4th and 5th for the second note.
          if (random() % 2 == 0)
            n = GetMappedDegreeFromRoot(chords[i].root, 3, periodOffset);
          else
            n = GetMappedDegreeFromRoot(chords[i].root, 4, periodOffset);
          events_.Emplace(n, periodOffset, len2);
          break;
      }
    }
    return events_.Count();
  }
};
