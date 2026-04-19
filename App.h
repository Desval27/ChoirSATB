#pragma once

#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <Music/Music.h>
#include <Music/Gnome.h>

#include <Types.h>
#include <Bass.h>
#include <Tenor.h>
#include <Alto.h>
#include <Soprano.h>

////////////////////////////////////////////////////////////////////////////////
/// @brief
class TheApp
{
  public:
    static TheApp &instance(int bars = 8);

    static constexpr int MAX_EVENTS = 128;

    enum
    {
        THE_BASS = 0,
        THE_TENOR,
        THE_ALTO,
        THE_SOPRANO,
        NUM_VOICES
    };


    void Init(float sample_rate);

    size_t MakeChordEvents(ChordEventSet<> &chords);
    void   MakeEvents();

    int  DoPulse();
    int  GetBeat() const { return _gnome.GetBeat(); }
    int  GetBar() const { return _gnome.GetBar(); }
    int  GetBPM() const { return _config.bpm.Get(); }
    void AdjustBPM(int delta);
    void AppendVolumeToString(daisy::FixedCapStrBase<char>& string) const { _config.bpm.AppentToString(string); }

    bool GetRunning() const { return _running; }
    void SetRunning(bool value) { _running = value; }
    void ToggleRunning() { _running = !_running; }

    TheVoice *           GetVoice(int index) { return _voices[index]; }
    const TimeSignature *GetTS() const { return &_ts; }
    int                  GetScaleIndex() const { return _scaleIndex; }
    void                 SetScaleIndex(int value);

  private:
    SystemConfig          _config;
    Gnome                 _gnome;
    const TuningReference _refA4;
    const TimeSignature   _ts;
    Temperament           _t;
    ScaleMap              _s;

    volatile bool _running;
    int           _scaleIndex;
    int           _bars;
    TheVoice *    _voices[NUM_VOICES];

    TheBass    _bass;
    TheTenor   _tenor;
    TheAlto    _alto;
    TheSoprano _soprano;

    TheApp(int bars = 8);
};