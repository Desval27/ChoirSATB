/* SPDX-License-Identifier: CC0-1.0 */
/**
 * @file ChoirSATB.h
 * @brief
 * @author pfburdette <paul.f.burdette@gmail.com>
 *
 * @copyright This work is dedicated to the public domain under CC0 1.0.
 * To the extent possible under law, the author(s) have waived all copyright
 * and related or neighboring rights to this software.
 * See <http://creativecommons.org>
 */
#pragma once

#include <BasicApp.h>
#include <SynthVoice.h>

#include <music/event_set_manager.hpp>
#include <music/music_config.hpp>
#include <singleton.hpp>

////////////////////////////////////////////////////////////////////////////////
/// @brief
template<std::size_t VOICE_COUNT = 4,
         std::size_t MAX_DEGREES = music::DEF_MAX_DEGREES,
         std::size_t SCALE_DEGREES = music::DEF_SCALE_DEGREES,
         std::size_t MAX_EVENTS = music::DEF_MAX_EVENTS>
class ChoirSATB
  : public BasicApp<MAX_DEGREES, SCALE_DEGREES>
  , public Singleton<
      ChoirSATB<VOICE_COUNT, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>>
{
  using BaseApp = BasicApp<MAX_DEGREES, SCALE_DEGREES>;
  using SingletonApp =
    Singleton<ChoirSATB<VOICE_COUNT, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>>;
  using EventSetManager =
    music::EventSetManager<MAX_DEGREES,
                           SCALE_DEGREES,
                           MAX_EVENTS,
                           music::NoteEventSet<MAX_EVENTS>>;

  struct SystemConfig
  {
    daisy::MappedFloatValue bpm;
    daisy::MappedIntValue transpose;

    SystemConfig()
      : bpm(1.0F,
            300.0F,
            60.0F,
            daisy::MappedFloatValue::Mapping::lin,
            "BPM",
            1,
            false)
      , transpose(-12, 12, 0, 1, 1, "T", true)
    {
    }
  };

  struct VoiceRole
  {
    const MString<6> Name;
    const music::Period period;
    const Range<float> density;
    const music::NoteValue granularity;

    VoiceRole(const char* name,
              music::Period period,
              Range<float> density,
              music::NoteValue granularity)
      : Name(name)
      , period(period)
      , density(density)
      , granularity(granularity)
    {
    }
  };

  using Persona =
    music::Persona<VoiceRole, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>;

  /*
    VoiceRole tenorRoleA("Tenor A",
                         3,
                         Range<float>(0.3F, 0.5F),
                         music::NoteValue::Quarter);
    VoiceRole altoRoleA("Alto A",
                        4,
                        Range<float>(0.5F, 0.7F),
                        music::NoteValue::Eighth);
    VoiceRole sopranoRoleA("Soprano A",
                           5,
                           Range<float>(0.6F, 0.9F),
                           music::NoteValue::Sixteenth);
                           */

private:
  ChoirSATB()
    : BaseApp()
    , roles(make_roles())
    , personas(make_personas())
  /*
  , bassRoleA("Bass A", 2, Range<float>(0.3F, 0.7F), music::NoteValue::Half)
  , tenorRoleA(
      "Tenor A", 3, Range<float>(0.3F, 0.5F), music::NoteValue::Quarter)
  , altoRoleA(
      "Alto A", 4, Range<float>(0.5F, 0.7F), music::NoteValue::Eighth)
  , sopranoRoleA("Soprano A",
                 5,
                 Range<float>(0.6F, 0.9F),
                 music::NoteValue::Sixteenth)
                 */
  {
  }

  friend SingletonApp;

public:
  std::array<SynthVoice, VOICE_COUNT> voices;

  void init(float sample_rate) override
  {
    BaseApp::init(sample_rate);

    // Voices
    for (auto& v : voices) {
      v.init(sample_rate);
      v.update(0UL); // Initial state
    }

    voices[0].set_freq(55.0F);
    voices[1].set_freq(110.0F);
    voices[2].set_freq(220.0F);
    voices[3].set_freq(440.0F);

    // Managers
    for (std::size_t i = 0; i < managers.size(); i++) {
      managers[i].set_persona(personas[i]);
    }
  }

  std::tuple<float, float> process(bool trigger = false) override
  {
    const float evenMix = 1.0 / VOICE_COUNT;
    float mixL = 0.0f;
    float mixR = 0.0f;
    for (SynthVoice& v : voices) {
      auto [sigL, sigR] = v.process();
      mixL = mixL + (sigL * v.config_.volume * evenMix);
      mixR = mixR + (sigR * v.config_.volume * evenMix);
    }
    return { mixL, mixR };
  }

  int handle_pulse()
  {
    int pulse = BaseApp::gnome.do_pulse();
    // if (pulse == 0) {
    //   // Just playing with it for now
    //   if (iterations_ == 4) {
    //     iterations_ = 0;
    //     Randomize();
    //   } else {
    //     iterations_++;
    //   }
    //   MakeEvents();
    // }

    for (auto& m : managers) {
      m.handle_pulse(pulse);
    }

    // // This won't always work and needs to be improved.
    // if (gnome_.RisingBeatEdge()) {
    //   MyChordEvent chord = chords_.GetEventForPulse(pulse);
    //   // Later we need to let the chord produce the text to account for tones
    //   chord.GetChordName(
    //     setup_.scaleMap, chordText_, setup_.temperament.DegreesPerPeriod());
    // }

    return pulse;
  }

  float get_bpm() const { return config_.bpm; }
  void set_bpm(float value) { config_.bpm = value; }
  template<std::size_t N>
  void bpm_append(daisy::FixedCapStr<N>& str) const
  {
    config_.bpm.AppentToString(str);
  }

  bool get_running() const { return running_; }
  void set_running(bool value) { running_ = value; }

protected:
  void internal_update(uint32_t nowMS) override
  {
    for (SynthVoice& v : voices) {
      v.update(nowMS);
    }
  }

private:
  SystemConfig config_;
  bool running_;

  std::array<VoiceRole, VOICE_COUNT> roles;
  std::array<Persona, VOICE_COUNT> personas;
  std::array<EventSetManager, VOICE_COUNT> managers;

  static std::array<VoiceRole, VOICE_COUNT> make_roles()
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        VoiceRole{
          "Bass A", 2, Range<float>(0.3F, 0.7F), music::NoteValue::Half },
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        VoiceRole{
          "Bass A", 2, Range<float>(0.3F, 0.7F), music::NoteValue::Half },
        VoiceRole{ "Soprano A",
                   5,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Sixteenth },
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        VoiceRole{
          "Bass A", 2, Range<float>(0.3F, 0.7F), music::NoteValue::Half },
        VoiceRole{
          "Tenor A", 3, Range<float>(0.3F, 0.5F), music::NoteValue::Quarter },
        VoiceRole{
          "Alto A", 4, Range<float>(0.5F, 0.7F), music::NoteValue::Eighth },
        VoiceRole{ "Soprano A",
                   5,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Sixteenth },
      } };
    } else {
      static_assert(VOICE_COUNT == 1 || VOICE_COUNT == 2 || VOICE_COUNT == 4,
                    "Unsupported ChoirSATB voice count");
    }
  }

  std::array<Persona, VOICE_COUNT> make_personas()
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        Persona{ "BASS VOICE", BaseApp::setup, roles[0] },
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        Persona{ "BASS VOICE", BaseApp::setup, roles[0] },
        Persona{ "SOPRANO VOICE", BaseApp::setup, roles[1] },
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        Persona{ "BASS VOICE", BaseApp::setup, roles[0] },
        Persona{ "TENOR VOICE", BaseApp::setup, roles[1] },
        Persona{ "ALTO VOICE", BaseApp::setup, roles[2] },
        Persona{ "SOPRANO VOICE", BaseApp::setup, roles[3] },
      } };
    } else {
      static_assert(VOICE_COUNT == 1 || VOICE_COUNT == 2 || VOICE_COUNT == 4,
                    "Unsupported ChoirSATB voice count");
    }
  }

  /*

      size_t MakeChordEvents(MyChordEventSet &chords);
      void   MakeEvents();

      int  DoPulse();
      int  GetBeat() const { return gnome_.GetBeat(); }
      int  GetBar() const { return gnome_.GetBar(); }
      int  GetBPM() const { return config_.bpm.Get(); }
      void AdjustBPM(int16_t delta);
      void AppendVolumeToString(daisy::FixedCapStrBase<char>& string) const {
    config_.bpm.AppentToString(string); }

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
      music::Gnome          gnome_;
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
  */
};
