#pragma once

#include <Music/Music.h>

#include "Voices/Bass.h"
#include "Voices/Tenor.h"
#include "Voices/Alto.h"
#include "Voices/Soprano.h"

using namespace Music;

/**
 * @brief 
 * 
 */
class TheApp
{
  public:
    static constexpr int NUM_VOICES = 4;
    static constexpr int MAX_EVENTS = 128;

    TimeSignature   ts;
    TuningReference refA4;
    Temperament     t;
    ScaleMap        scale;
    Gnome           gnome;
    TheVoice*       voices[NUM_VOICES];

    TheApp(DaisySeed hw, int bars = 8)
    : refA4(440.0f, 9, 0),
      gnome(ts, bars),
      _running(false),
      _scaleIndex(0),
      _bars(bars),
      _bass(hw, ts, refA4, t, scale),
      _tenor(hw, ts, refA4, t, scale),
      _alto(hw, ts, refA4, t, scale),
      _soprano(hw, ts, refA4, t, scale)
    {
        t.MakeEqualDivision(12, 2.0f);
        t.AttachNoteLabels(Music::NOTE_NAMES_12,
                           ArrayLen(Music::NOTE_NAMES_12));
        t.AttachIntervalLabels(Music::INTERVAL_NAMES_12,
                               ArrayLen(Music::INTERVAL_NAMES_12));

        SetScaleIndex(GetScaleIndex());

        voices[0] = &_bass;
        voices[1] = &_tenor;
        voices[2] = &_alto;
        voices[3] = &_soprano;
    }

    bool GetRunning() const { return _running; }
    void SetRunning(bool value) { _running = value; }

    /**
     * @brief 
     * 
     * @param eventsOut 
     * @param eventsOutLen 
     * @return size_t 
     */
    size_t MakeChordEvents(ChordEvent* eventsOut, size_t eventsOutLen)
    {
        return GenerateStandardChordEvents(
            ts,
            scale,
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

    /**
     * @brief 
     * 
     */
    void MakeEvents()
    {
        ChordEvent chordEvents[MAX_EVENTS];
        size_t     chordEventsLen = 0;

        // First establish our harmonic rhythm
        chordEventsLen = MakeChordEvents(chordEvents, ArrayLen(chordEvents));

        // Pass that onto our voices
        for(int i = 0; i < NUM_VOICES; i++)
            voices[i]->MakeEvents(ts, _bars, chordEvents, chordEventsLen);
    }

    int  GetScaleIndex() const { return _scaleIndex; }
    void SetScaleIndex(int value)
    {
        _scaleIndex = wrap(value, D12StartIndex, D12StartIndex + D12Count - 1);
        scale.SetDegrees(SCALE_TABLES[_scaleIndex].degrees,
                         SCALE_TABLES[_scaleIndex].degreeCount);
        MakeEvents();
        gnome.Reset();
    }

    void DoPulse(int pulse)
    {
        for(int i = 0; i < NUM_VOICES; i++)
        {
            voices[i]->DoPulse(pulse);
        }
    }


  private:
    volatile bool _running;
    int           _scaleIndex;
    int           _bars;

    TheBass    _bass;
    TheTenor   _tenor;
    TheAlto    _alto;
    TheSoprano _soprano;
};