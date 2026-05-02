#include <App.h>

using namespace Music;

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param hw
/// @param bars
/// @return
TheApp &TheApp::instance(int bars) {
  static TheApp inst(bars);
  return inst;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param hw
/// @param bars
TheApp::TheApp(int bars)
    : config_(), setup_(4, NoteValue::Quarter, 12, 2.0f),
      tuningRef_(440.0f, 9, 0),
      gnome_(setup_.timeSignature, setup_.bars), 
      running_(false), scaleIndex_(0), iterations_(0),
      bass_(setup_, tuningRef_), tenor_(setup_, tuningRef_),
      alto_(setup_, tuningRef_), soprano_(setup_, tuningRef_) {

  voices_[0] = &bass_;
  voices_[1] = &tenor_;
  voices_[2] = &alto_;
  voices_[3] = &soprano_;

  setup_.temperament.AttachNoteLabels(Music::NOTE_NAMES_12);
  setup_.temperament.AttachIntervalLabels(Music::INTERVAL_NAMES_12);

  // Trigger the side-effects...bad code monkey, bad code monkey.
  SetScaleIndex(GetScaleIndex());
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
void TheApp::Update() {
  for (int i = 0; i < NUM_VOICES; i++) {
    voices_[i]->Update();
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param delta
void TheApp::AdjustBPM(int delta) { config_.bpm.Step(delta, false); }

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param chords
/// @return
size_t TheApp::MakeChordEvents(MyChordEventSet &chords) {
  return GenerateStandardChordEvents(setup_, NoteValue::Whole, chords);

  // First start with our "hit" pattern
  // bool   pattern[MAX_EVENTS];
  // size_t patternLen = Music::GeneratePattern(
  //     ts, BARS, 0.50f, Music::NoteValue::Quarter, pattern,
  //     ArrayLen(pattern));
  // return Music::GenerateChordEventsFromPattern(pattern,
  //                                              patternLen,
  //                                              Music::NoteValue::Quarter,
  //                                              eventsOut,
  //                                              eventsOutLen);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
void TheApp::MakeEvents() {
  // First establish our harmonic rhythm
  MakeChordEvents(chords_);

  // Pass that onto our voices
  for (int i = 0; i < NUM_VOICES; i++)
    voices_[i]->MakeEvents(chords_);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param value
void TheApp::SetScaleIndex(int value) {
  scaleIndex_ = wrap(value, static_cast<int>(ArrayLen(HEPATONIC_D12_SCALES) - 1));
  setup_.scaleMap.SetScale(HEPATONIC_D12_SCALES[scaleIndex_]);
}

////////////////////////////////////////////////////////////////////////////////
void TheApp::Randomize() {
  SetScaleIndex(randomRange(0, static_cast<int>(ArrayLen(HEPATONIC_D12_SCALES) - 1)));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
int TheApp::DoPulse() {
  int pulse = gnome_.DoPulse();
  if (pulse == 0) {
    // Just playing with it for now
    if (iterations_ == 4) {
      iterations_ = 0;
      Randomize();
    } else {
      iterations_++;
    }
    MakeEvents();
  }

  for (int i = 0; i < NUM_VOICES; i++) {
    voices_[i]->DoPulse(pulse);
  }

  // This won't always work and needs to be improved.
  if (gnome_.RisingBeatEdge()) {
    MyChordEvent chord = chords_.GetEventForPulse(pulse);
    // Later we need to let the chord produce the text to account for tones
    chord.GetChordName(setup_.scaleMap, chordText_, setup_.temperament.DegreesPerPeriod());
  }

  return pulse;
}
