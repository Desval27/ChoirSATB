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
#include <music/music_tables.hpp>
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
  using MySetup = music::Setup<MAX_DEGREES, SCALE_DEGREES>;
  using SingletonApp =
    Singleton<ChoirSATB<VOICE_COUNT, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>>;
  using NoteEventSetManager =
    music::EventSetManager<MAX_DEGREES,
                           SCALE_DEGREES,
                           MAX_EVENTS,
                           music::NoteEventSet<MAX_EVENTS>>;
  using ChordEventSetManager = music::EventSetManager<
    MAX_DEGREES,
    SCALE_DEGREES,
    MAX_EVENTS,
    music::ChordEventSet<MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>>;

  using MySynthVoice = SynthVoice<MAX_DEGREES, SCALE_DEGREES>;

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
    const music::WeightMap<SCALE_DEGREES>& weight_map;

    VoiceRole(const char* name,
              music::Period period,
              Range<float> density,
              music::NoteValue granularity,
              const music::WeightMap<SCALE_DEGREES>& weight_map)
      : Name(name)
      , period(period)
      , density(density)
      , granularity(granularity)
      , weight_map(weight_map)
    {
    }
  };

  struct ChordRole
  {
    const MString<6> Name;
    const music::Period period;
    const Range<float> density;
    const music::NoteValue granularity;
    ChordRole()
      : Name("CHORDS")
      , period(0)
      , density(0.0F, 0.0F)
      , granularity(music::NoteValue::Whole)
    {
    }
  };

  using Persona =
    music::Persona<VoiceRole, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>;
  using ChordPersona =
    music::Persona<ChordRole, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>;

private:
  ChoirSATB()
    : BaseApp()
    , chordRole_()
    , chordPersona_("CHORDS", BaseApp::setup, chordRole_)
    , chordManager_(BaseApp::setup)
    , roles_(make_roles())
    , personas_(make_personas())
    , managers_(make_managers(BaseApp::setup))
  {
  }

  friend SingletonApp;

public:
  std::array<MySynthVoice, VOICE_COUNT> voices;

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  static std::size_t VoiceCount() { return VOICE_COUNT; }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param sample_rate
  void init(float sample_rate) override
  {
    BaseApp::init(sample_rate);

    // Voices
    for (std::size_t i = 0; i < voices.size(); i++) {
      voices[i].init(BaseApp::setup, roles_[i].period, sample_rate);
      voices[i].update(0UL); // Initial state
    }

    // Managers
    chordManager_.set_persona(chordPersona_);
    for (std::size_t i = 0; i < managers_.size(); i++) {
      managers_[i].set_persona(personas_[i]);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param trigger
  /// @return
  std::tuple<float, float> process(bool trigger = false) override
  {
    const float evenMix = 1.0 / VOICE_COUNT;
    float mixL = 0.0f;
    float mixR = 0.0f;
    for (auto& v : voices) {
      auto [sigL, sigR] = v.process();
      mixL = mixL + (sigL * v.config_.volume * evenMix);
      mixR = mixR + (sigR * v.config_.volume * evenMix);
    }
    return { mixL, mixR };
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  int handle_pulse()
  {
    int pulse = BaseApp::gnome.do_pulse();
    if (pulse == 0) {
      make_events();
    }
    chordManager_.handle_pulse(pulse);

    int i = 0;
    for (auto& m : managers_) {
      m.handle_pulse(pulse);
      voices[i++].handle_pulse(
        pulse, m.get_current_note(), m.get_gate(), m.get_trigger());
    }
    return pulse;
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  float get_bpm() const { return config_.bpm; }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param value
  void set_bpm(float value) { config_.bpm = value; }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @tparam N
  /// @param str
  template<std::size_t N>
  void bpm_append(daisy::FixedCapStr<N>& str) const
  {
    config_.bpm.AppentToString(str);
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  bool get_running() const { return running_; }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param value
  void set_running(bool value) { running_ = value; }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  const char* get_current_chord_text() const
  {
    return chordManager_.get_text();
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  const char* get_scale_name() const { return scaleName_.c_str(); }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @tparam N
  /// @param str
  /// @param index
  template<std::size_t N>
  void voice_volume_append(daisy::FixedCapStr<N>& str, std::size_t index) const
  {
    voices[index].config_.volume.AppentToString(str);
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param index
  /// @return
  const char* get_current_voice_text(std::size_t index) const
  {
    return managers_[index].get_text();
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @tparam N
  /// @param str
  template<std::size_t N>
  void timing_append(daisy::FixedCapStr<N>& str)
  {
    // str.AppendInt(BaseApp::setup.timeSignature.beats);
    // str.Append('/');
    // str.AppendInt(BaseApp::setup.timeSignature.get_denominator());
    // str.Append(" ");
    str.AppendInt(BaseApp::gnome.get_bar() + 1);
    str.Append(':');
    str.AppendInt(BaseApp::gnome.get_beat() + 1);
  }

protected:
  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param nowMS
  void internal_update(uint32_t nowMS) override
  {
    for (auto& v : voices) {
      v.update(nowMS);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  virtual void make_events()
  {
    const std::size_t scaleIdx = randomRange(
      static_cast<std::size_t>(0), ArrayLen(music::HEPATONIC_D12_SCALES) - 1);
    auto& scale = music::HEPATONIC_D12_SCALES[scaleIdx];
    BaseApp::setup.scaleMap.set_scale(scale);
    scaleName_.set(scale.name);

    // Recreate our chord events
    chordManager_.make_chord_events();

    // And now use those chords to create events for our voices
    for (auto& m : managers_) {
      m.make_note_events_from_chords(chordManager_.get_events());
    }
  }

private:
  SystemConfig config_;
  bool running_;
  MString<20> scaleName_;

  ChordRole chordRole_;
  ChordPersona chordPersona_;
  ChordEventSetManager chordManager_;

  std::array<VoiceRole, VOICE_COUNT> roles_;
  std::array<Persona, VOICE_COUNT> personas_;
  std::array<NoteEventSetManager, VOICE_COUNT> managers_;

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param setup
  /// @return
  static std::array<NoteEventSetManager, VOICE_COUNT> make_managers(
    const MySetup& setup)
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        NoteEventSetManager{ setup },
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        NoteEventSetManager{ setup },
        NoteEventSetManager{ setup },
      } };
    } else if constexpr (VOICE_COUNT == 3) {
      return { {
        NoteEventSetManager{ setup },
        NoteEventSetManager{ setup },
        NoteEventSetManager{ setup },
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        NoteEventSetManager{ setup },
        NoteEventSetManager{ setup },
        NoteEventSetManager{ setup },
        NoteEventSetManager{ setup },
      } };
    } else {
      static_assert(VOICE_COUNT == 1 || VOICE_COUNT == 2 || VOICE_COUNT == 3 ||
                      VOICE_COUNT == 4,
                    "Unsupported ChoirSATB voice count");
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  static std::array<VoiceRole, VOICE_COUNT> make_roles()
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        VoiceRole{ "Bass A",
                   -2,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Quarter,
                   music::SCALE_WEIGHTS_7_UNIFORM },
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        VoiceRole{ "Bass A",
                   -2,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Quarter,
                   music::SCALE_WEIGHTS_7_UNIFORM },
        VoiceRole{ "Soprano A",
                   1,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Sixteenth,
                   music::SCALE_WEIGHTS_7_UNIFORM },
      } };
    } else if constexpr (VOICE_COUNT == 3) {
      return { {
        VoiceRole{ "Bass A",
                   -2,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Half,
                   music::SCALE_WEIGHTS_7_UNIFORM },
        VoiceRole{ "Alto A",
                   0,
                   Range<float>(0.5F, 0.7F),
                   music::NoteValue::Eighth,
                   music::SCALE_WEIGHTS_7_UNIFORM },
        VoiceRole{ "Soprano A",
                   1,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Sixteenth,
                   music::SCALE_WEIGHTS_7_UNIFORM },
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        VoiceRole{ "Bass A",
                   -2,
                   Range<float>(0.6F, 0.9F),
                   music::NoteValue::Half,
                   music::SCALE_WEIGHTS_7_TONIC_HEAVY },
        VoiceRole{ "Tenor A",
                   -1,
                   Range<float>(0.3F, 0.7F),
                   music::NoteValue::Quarter,
                   music::SCALE_WEIGHTS_7_TONIC_HEAVY },
        VoiceRole{ "Alto A",
                   0,
                   Range<float>(0.3F, 0.8F),
                   music::NoteValue::Eighth,
                   music::SCALE_WEIGHTS_7_CHORD_TONE_HEAVY },
        VoiceRole{ "Soprano A",
                   1,
                   Range<float>(0.3F, 0.9F),
                   music::NoteValue::Sixteenth,
                   music::SCALE_WEIGHTS_7_UNIFORM },
      } };
    } else {
      static_assert(VOICE_COUNT == 1 || VOICE_COUNT == 2 || VOICE_COUNT == 3 ||
                      VOICE_COUNT == 4,
                    "Unsupported ChoirSATB voice count");
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  std::array<Persona, VOICE_COUNT> make_personas()
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        Persona{ "BASS VOICE", BaseApp::setup, roles_[0] },
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        Persona{ "BASS VOICE", BaseApp::setup, roles_[0] },
        Persona{ "SOPRANO VOICE", BaseApp::setup, roles_[1] },
      } };
    } else if constexpr (VOICE_COUNT == 3) {
      return { {
        Persona{ "BASS VOICE", BaseApp::setup, roles_[0] },
        Persona{ "ALTO VOICE", BaseApp::setup, roles_[1] },
        Persona{ "SOPRANO VOICE", BaseApp::setup, roles_[2] },
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        Persona{ "BASS VOICE", BaseApp::setup, roles_[0] },
        Persona{ "TENOR VOICE", BaseApp::setup, roles_[1] },
        Persona{ "ALTO VOICE", BaseApp::setup, roles_[2] },
        Persona{ "SOPRANO VOICE", BaseApp::setup, roles_[3] },
      } };
    } else {
      static_assert(VOICE_COUNT == 1 || VOICE_COUNT == 2 || VOICE_COUNT == 3 ||
                      VOICE_COUNT == 4,
                    "Unsupported ChoirSATB voice count");
    }
  }
};
