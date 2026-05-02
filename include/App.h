#pragma once

#include <daisy_seed.h>
#include <daisysp.h>

#include <Music/Gnome.h>

#include "Types.h"

#include "Bass.h"
#include "Tenor.h"
#include "Alto.h"
#include "Soprano.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief
class TheApp
{
  public:
    static TheApp &instance(int bars = 8);

    enum
    {
        THE_BASS = 0,
        THE_TENOR,
        THE_ALTO,
        THE_SOPRANO,
        NUM_VOICES
    };

    ///////////////////////////////////////////////////////////////////////////
    /// @brief
    /// @param sample_rate
    void Init(float sample_rate)
    {
        // Voices
        for(int i = 0; i < NUM_VOICES; i++)
        {
            voices_[i]->Init(sample_rate);
        }

        // For Fun
        // bpmLfo_.Init(sample_rate);
        // bpmLfo_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
        // bpmLfo_.SetFreq(0.001f);
        // bpmLfo_.SetAmp(1.0f);
    }

    void Update();

    ///////////////////////////////////////////////////////////////////////////
    /// @brief 
    /// @return 
    float Process() 
    {
        const float voiceMix[NUM_VOICES] = {
          0.30f, 0.25f, 0.25f, 0.20f,
        };
        //const float equalMix = 1.0f / (float)NUM_VOICES;
        float sig = 0.0f;
        for(int i = 0; i < NUM_VOICES; i++)
        {
            sig += (voices_[i]->Process() * voiceMix[i]);
        }

      // config_.bpm.SetFrom0to1(bpmLfo_.Process());
      return sig;
    }

    size_t MakeChordEvents(MyChordEventSet &chords);
    void   MakeEvents();

    int  DoPulse();
    int  GetBeat() const { return gnome_.GetBeat(); }
    int  GetBar() const { return gnome_.GetBar(); }
    int  GetBPM() const { return config_.bpm.Get(); }
    void AdjustBPM(int delta);
    void AppendVolumeToString(daisy::FixedCapStrBase<char>& string) const { config_.bpm.AppentToString(string); }

    const char* GetChordText() const { return chordText_; }
    bool GetRunning() const { return running_; }
    void SetRunning(bool value) { running_ = value; }
    void ToggleRunning() { running_ = !running_; }
    void Randomize();
    void Reset() { gnome_.Reset(); }

    TheVoice *           GetVoice(int index) { return voices_[index]; }

    template <std::size_t N>
    void GetTimingText(daisy::FixedCapStr<N> &text)
    {
      text.Clear();
      text.AppendInt(setup_.timeSignature.beats);
      text.Append('/');
      text.AppendInt(setup_.timeSignature.GetDenominator());
      text.Append(" ");
      text.AppendInt(GetBar()+1);
      text.Append(':');
      text.AppendInt(GetBeat()+1);
    }
    
    int                  GetScaleIndex() const { return scaleIndex_; }
    void                 SetScaleIndex(int value);

  private:
    SystemConfig          config_;
    MySetup               setup_;
    const MyTuningReference tuningRef_;
    Music::Gnome          gnome_;
    MyChordEventSet       chords_;

    MString<20>           chordText_;

    volatile bool running_;
    int           scaleIndex_;
    int           iterations_;
    TheVoice *    voices_[NUM_VOICES];

    TheBass    bass_;
    TheTenor   tenor_;
    TheAlto    alto_;
    TheSoprano soprano_;

    // daisysp::Oscillator bpmLfo_;

    TheApp(int bars = 8);
};
