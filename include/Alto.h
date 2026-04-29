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

#include "Voice.h"

class TheAlto : public TheVoice
{
public:
    TheAlto(const MyTimeSignature &ts,
            const MyTuningReference &tr,
            const MyTemperament &t,
            const MyScaleMap &s)
        // -1 Relative to C4 = C3
        : TheVoice(ts, tr, t, s, -1, 0.1, 0.2, 0.4, 0.1, Music::SCALE_WEIGHTS_7_CHORD_TONE_HEAVY)
    {
    }

    virtual const char *GetName() const override { return s_ALTO; }

    virtual void Init(float sample_rate) override
    {
        TheVoice::Init(sample_rate);
        vib.Init(sample_rate);
        vib.SetWaveform(daisysp::Oscillator::WAVE_SIN);
        vib.SetFreq(5.5f); // Typical?
    }

    virtual float Process()
    {
        const float vib_depth = 0.0293f; // ~50 cents pitch multipler

        float vib_val = vib.Process();

        // Multipilcation scales the frequency exponentially (musically)
        float freq = GetBaseFrequency() * (1.0f + (vib_val * vib_depth));
        osc.SetFreq(freq);

        return TheVoice::Process();
    }

    virtual size_t MakeEvents(const MyTimeSignature &ts,
                              int bars,
                              MyChordEventSet &chords)
    {
        // First start with our "hit" pattern
        MyPatternEventSet pattern;
        const Music::NoteValue g = Music::NoteValue::Quarter;
        const float density = randomRange(0.5, 0.8);
        MyEuclidianPatternGenerator::GeneratePattern(ts, bars, density, g, pattern);
        return MyStyleANoteGenerator::GenerateEvents(pattern, chords, ts, temperament, scaleMap, bars, g, weights, events);
    }
        // static std::size_t GeneratePattern(const PatternEventSet<MAX_EVENTS> &pattern,
        //                               const ChordEventSet<MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS> &chords,
        //                               const TimeSignature &ts,
        //                               const Temperament<MAX_DEGREES> &temperament,
        //                               const ScaleMap<SCALE_DEGREES> &scale,
        //                               int bars,
        //                               NoteValue granularity,
        //                               NoteEventSet<MAX_EVENTS> &events)

private:
    daisysp::Oscillator vib;
};
