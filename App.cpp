#include "App.h"


////////////////////////////////////////////////////////////////////////////////
/// @brief 
/// @param hw 
/// @param bars 
/// @return 
TheApp &TheApp::instance(int bars)
{
    static TheApp inst(bars);
    return inst;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param hw
/// @param bars
TheApp::TheApp(int bars)
: gnome(_ts, bars),
  _refA4(440.0f, 9, 0),
  _running(false),
  _scaleIndex(0),
  _bars(bars),
  _bass(_ts, _refA4, _t, _s),
  _tenor(_ts, _refA4, _t, _s),
  _alto(_ts, _refA4, _t, _s),
  _soprano(_ts, _refA4, _t, _s)
{
    _voices[0] = &_bass;
    _voices[1] = &_tenor;
    _voices[2] = &_alto;
    _voices[3] = &_soprano;

    _t.MakeEqualDivision(12, 2.0f);
    _t.AttachNoteLabels(Music::NOTE_NAMES_12, ArrayLen(Music::NOTE_NAMES_12));
    _t.AttachIntervalLabels(Music::INTERVAL_NAMES_12,
                            ArrayLen(Music::INTERVAL_NAMES_12));

    // Trigger the side-effects...bad code monkey, bad code monkey.
    SetScaleIndex(GetScaleIndex());
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param sample_rate
void TheApp::Init(float sample_rate)
{
    // Voices
    for(int i = 0; i < NUM_VOICES; i++)
    {
        _voices[i]->Init(sample_rate);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param eventsOut
/// @param eventsOutLen
/// @return
size_t TheApp::MakeChordEvents(ChordEvent *eventsOut, size_t eventsOutLen)
{
    return GenerateStandardChordEvents(
        _ts,
        _s,
        _bars,
        SCALE_TABLES[GetScaleIndex()].harmonicMode,
        NoteValue::Whole,
        eventsOut,
        eventsOutLen);
    // First start with our "hit" pattern
    // bool   pattern[MAX_EVENTS];
    // size_t patternLen = Music::GeneratePattern(
    //     ts, BARS, 0.50f, Music::NoteValue::Quarter, pattern, ArrayLen(pattern));
    // return Music::GenerateChordEventsFromPattern(pattern,
    //                                              patternLen,
    //                                              Music::NoteValue::Quarter,
    //                                              eventsOut,
    //                                              eventsOutLen);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
void TheApp::MakeEvents()
{
    ChordEvent chordEvents[MAX_EVENTS];
    size_t     chordEventsLen = 0;

    // First establish our harmonic rhythm
    chordEventsLen = MakeChordEvents(chordEvents, ArrayLen(chordEvents));

    // Pass that onto our voices
    for(int i = 0; i < NUM_VOICES; i++)
        _voices[i]->MakeEvents(_ts, _bars, chordEvents, chordEventsLen);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param value
void TheApp::SetScaleIndex(int value)
{
    _scaleIndex = wrap(value, D12StartIndex, D12StartIndex + D12Count - 1);
    _s.SetDegrees(SCALE_TABLES[_scaleIndex].degrees,
                  SCALE_TABLES[_scaleIndex].degreeCount);
    MakeEvents();
    gnome.Reset();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
void TheApp::DoPulse(int pulse)
{
    for(int i = 0; i < NUM_VOICES; i++)
    {
        _voices[i]->DoPulse(pulse);
    }
}
