#pragma once

#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <Monkey.h>
#include <Music/Music.h>
#include <Music/MusicTemplates.h>

#include <ChoirSATB.h>

///////////////////////////////////////////////////////////////////////////////
// Aliases
///////////////////////////////////////////////////////////////////////////////
using MyApp = ChoirSATB<4>;
using MyDisplay = daisy::OledDisplay<daisy::SSD130xI2c128x64Driver>;
using TSetup = music::Setup<>;
using MyPitchEngine = music::PitchEngine<>;

using MyTuningReference = music::TuningReference;
using MyWeightMap = music::WeightMap<music::HEPATONIC>;
using TChordEvent = music::ChordEvent<>;
using TChordEventSet = music::ChordEventSet<>;
using TNoteEventSet = music::NoteEventSet<>;
using TPatternEventSet = music::PatternEventSet<>;
using MyEuclidianPatternGenerator = music::EuclidianPatternGenerator<>;
using MySimpleRandomPatternGenerator = music::SimpleRandomPatternGenerator<>;
using MyStyleANoteGenerator = music::StyleANoteGenerator<>;
