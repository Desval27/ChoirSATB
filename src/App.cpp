#include <App.h>


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
: _gnome(_ts, bars),
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
void TheApp::Update()
{
    for(int i = 0; i < NUM_VOICES; i++)
    {
        _voices[i]->Update();
    }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param delta
void TheApp::AdjustBPM(int delta)
{ _config.bpm.Step(delta, false); }

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param chords
/// @return
size_t TheApp::MakeChordEvents(ChordEventSet<> &chords)
{
    return GenerateStandardChordEvents(
        _ts,
        _s,
        _bars,
        SCALE_TABLES[GetScaleIndex()].harmonicMode,
        NoteValue::Whole,
        chords);
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
    // First establish our harmonic rhythm
    MakeChordEvents(_chords);

    // Pass that onto our voices
    for(int i = 0; i < NUM_VOICES; i++)
        _voices[i]->MakeEvents(_ts, _bars, _chords);
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
    _gnome.Reset();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
int TheApp::DoPulse()
{
    int pulse = _gnome.DoPulse();
    for(int i = 0; i < NUM_VOICES; i++)
    {
        _voices[i]->DoPulse(pulse);
    }
    
    // This won't always work and needs to be improved.
    if (_gnome.RisingBeatEdge())
    {
        ChordEvent chord = _chords.GetEventForPulse(pulse);
        // Later we need to let the chord produce the text to account for tones
        _t.GetNoteLabel(chord.root, _chordText, sizeof(_chordText));

    }
    return pulse;
}
