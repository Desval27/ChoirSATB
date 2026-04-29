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

#include "Voice.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief
class TheSoprano : public TheVoice
{
public:
    TheSoprano(const MyTimeSignature &ts,
               const MyTuningReference &tr,
               const MyTemperament &t,
               const MyScaleMap &s)
        // 0 Relative to C4 = C4
        : TheVoice(ts, tr, t, s, 0, 0.1, 0.3, 0.6, 0.3, Music::SCALE_WEIGHTS_7_UNIFORM)
    {
    }

    virtual const char *GetName() const override { return s_SOPRANO; }

    virtual void Init(float sample_rate) override
    {
        TheVoice::Init(sample_rate);
        vib.Init(sample_rate);
        vib.SetWaveform(daisysp::Oscillator::WAVE_SIN);
        vib.SetFreq(5.5f); // Typical?
    }

    virtual float Process()
    {
        // Add vibrato to longer notes
        if (GetCurrentNote().value > Music::NoteValue::Eighth)
        {
            const float vib_depth = 0.0293f; // ~50 cents pitch multipler

            float vib_val = vib.Process();

            // Multipilcation scales the frequency exponentially (musically)
            float freq = GetBaseFrequency() * (1.0f + (vib_val * vib_depth));
            osc.SetFreq(freq);
        }
        return TheVoice::Process();
    }

    virtual size_t MakeEvents(const MyTimeSignature &ts,
                              int bars,
                              MyChordEventSet &chords) override
    {
        MyPatternEventSet pattern;
        const Music::NoteValue g = Music::NoteValue::Eighth;
        const float density = randomRange(0.4f, 0.9f);
        MyEuclidianPatternGenerator::GeneratePattern(ts, bars, density, g, pattern);
        return MyStyleANoteGenerator::GenerateEvents(pattern, chords, ts, temperament, scaleMap, bars, g, weights, events);
    }

private:
    daisysp::Oscillator vib;
};
