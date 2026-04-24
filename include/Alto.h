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

    virtual void Init(float sample_rate) override 
    {
        TheVoice::Init(sample_rate);
        vib.Init(sample_rate);
        vib.SetWaveform(Oscillator::WAVE_SIN);
        vib.SetFreq(5.5f);   // Typical?
    }

    virtual float Process() 
    {
        const float vib_depth = 0.0293f;  // ~50 cents pitch multipler

        float vib_val = vib.Process();

        // Multipilcation scales the frequency exponentially (musically)
        float freq = GetBaseFrequency() * (1.0f + (vib_val * vib_depth));
        osc.SetFreq(freq);

        return TheVoice::Process();
    }

    virtual size_t MakeEvents(const Music::TimeSignature& ts,
                              int                         bars,
                              Music::ChordEventSet<>&     chords)
    {
        // First start with our "hit" pattern
        PatternEventSet<> pattern;
        const Music::NoteValue  g = Music::NoteValue::Quarter;
        const float density = randomRange(0.5, 0.8);
        EuclidianPatternGenerator<>::GeneratePattern(ts, bars, density, g, pattern);
        return GenerateEventsFromPattern(pattern, chords, ts, GetTemperament(), GetScaleMap(), bars, g, events);
        // events.Clear();
        // for(size_t i = 0; i < pattern.Count() && !events.AtCapacity(); i++)
        // {
        //     if(pattern[i]) // Hit
        //     {
        //         int         periodOffset = 0;
        //         Music::Note n = GetWeightedNote(randomRange(0.0f, 0.999999f),
        //                                         periodOffset);
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
