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
    { SetWeights(SCALE_WEIGHTS_7_UNIFORM, ArrayLen(SCALE_WEIGHTS_7_UNIFORM)); }

    virtual const char* GetName() const override { return s_SOPRANO; }

    virtual void Init(float sample_rate) override
    {
        TheVoice::Init(sample_rate);
        vib.Init(sample_rate);
        vib.SetWaveform(Oscillator::WAVE_SIN);
        vib.SetFreq(5.5f); // Typical?
    }

    virtual float Process()
    {
        // Add vibrato to longer notes
        if(GetCurrentNote().value > NoteValue::Eighth)
        {
            const float vib_depth = 0.0293f; // ~50 cents pitch multipler

            float vib_val = vib.Process();

            // Multipilcation scales the frequency exponentially (musically)
            float freq = GetBaseFrequency() * (1.0f + (vib_val * vib_depth));
            osc.SetFreq(freq);
        }
        return TheVoice::Process();
    }

    virtual size_t MakeEvents(const Music::TimeSignature& ts,
                              int                         bars,
                              Music::ChordEventSet<>&     chords) override
    {
        PatternEventSet<>      pattern;
        const Music::NoteValue g       = Music::NoteValue::Eighth;
        const float            density = randomRange(0.4, 0.9);
        EuclidianPatternGenerator<>::GeneratePattern(ts, bars, density, g, pattern);
        return StyleANoteGenerator<>::GenerateEvents(pattern, chords, ts, GetTemperament(), GetScaleMap(), bars, g, events);

        // // First start with our "hit" pattern
        // PatternEventSet<>      pattern;
        // const Music::NoteValue g = Music::NoteValue::Eighth;

        // GeneratePattern(ts, bars, randomRange(0.6f, 0.9f), g, pattern);


        // events.Clear();
        // for(size_t i = 0; i < pattern.Count() && !events.AtCapacity(); i++)
        // {
        //     if(pattern[i]) // Hit
        //     {
        //         int               periodOffset = 0;
        //         const Music::Note n            = GetWeightedNote(
        //             randomRange(0.0f, 0.999999f), periodOffset);
        //         events.Emplace(n, periodOffset, g);
        //     }
        //     else
        //     {
        //         events.Emplace(REST, 0, g);
        //     }
        // }
        // return events.Count();
    }

  private:
    Oscillator vib;
};
