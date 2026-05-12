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

#include <basic_app.hpp>
#include <synth_voice.hpp>

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
  requires(SCALE_DEGREES == music::HEPATONIC) // For now
class ChoirSATB
  : public BasicApp<MAX_DEGREES, SCALE_DEGREES>
  , public Singleton<
      ChoirSATB<VOICE_COUNT, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>>
{

  using TBasicApp = BasicApp<MAX_DEGREES, SCALE_DEGREES>;
  using TSetup = music::Setup<MAX_DEGREES, SCALE_DEGREES>;
  using TSingletonApp =
    Singleton<ChoirSATB<VOICE_COUNT, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>>;
  using TNoteEventSetManager =
    music::EventSetManager<MAX_DEGREES,
                           SCALE_DEGREES,
                           MAX_EVENTS,
                           music::NoteEventSet<MAX_EVENTS>>;
  using TChordEventSetManager = music::EventSetManager<
    MAX_DEGREES,
    SCALE_DEGREES,
    MAX_EVENTS,
    music::ChordEventSet<MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>>;

  using TSynthVoice = SynthVoice<MAX_DEGREES, SCALE_DEGREES>;
  using TPersonaRole = music::StockPersonaRole<SCALE_DEGREES>;
  using TPersona =
    music::Persona<TPersonaRole, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>;
  using TChordPersona =
    music::Persona<TPersonaRole, MAX_DEGREES, SCALE_DEGREES, MAX_EVENTS>;

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

private:
  ChoirSATB()
    : TBasicApp()
    , chord_role_(music::NoteValue::Whole)
    , chord_persona_("CHORDS", TBasicApp::setup, chord_role_)
    , chord_manager_(TBasicApp::setup)
    , roles_(make_roles())
    , personas_(make_personas())
    , managers_(make_managers(TBasicApp::setup))
  {
  }

  friend TSingletonApp;

public:
  std::array<TSynthVoice, VOICE_COUNT> voices;

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  static std::size_t VoiceCount() { return VOICE_COUNT; }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param sample_rate
  void init(float sample_rate) override
  {
    TBasicApp::init(sample_rate);

    // Voices
    for (std::size_t i = 0; i < voices.size(); i++) {
      voices[i].init(TBasicApp::setup, roles_[i].period_offset, sample_rate);
      voices[i].update(0UL); // Initial state
    }

    // Managers
    chord_manager_.set_persona(chord_persona_);
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
    int pulse = TBasicApp::gnome.do_pulse();
    if (pulse == 0) {
      make_events();
    }
    chord_manager_.handle_pulse(pulse);

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
    return chord_manager_.get_text();
  }

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @return
  const char* get_scale_name() const { return scale_name_.c_str(); }

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
    str.AppendInt(TBasicApp::gnome.get_bar() + 1);
    str.Append(':');
    str.AppendInt(TBasicApp::gnome.get_beat() + 1);
  }

  void load_voice_config() {}

  void save_voice_config() const {}

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
    const std::size_t scale_idx = randomRange(
      static_cast<std::size_t>(0), ArrayLen(music::HEPATONIC_D12_SCALES) - 1);
    auto& scale = music::HEPATONIC_D12_SCALES[scale_idx];
    TBasicApp::setup.scale_map.set_scale(scale);
    scale_name_.set(scale.name);

    // Recreate our chord events
    chord_manager_.make_chord_events();

    // And now use those chords to create events for our voices
    for (auto& m : managers_) {
      m.make_note_events_from_chords(chord_manager_.get_events());
    }
  }

private:
  SystemConfig config_;
  bool running_;
  MString<20> scale_name_;

  TPersonaRole chord_role_;
  TChordPersona chord_persona_;
  TChordEventSetManager chord_manager_;

  std::array<TPersonaRole, VOICE_COUNT> roles_;
  std::array<TPersona, VOICE_COUNT> personas_;
  std::array<TNoteEventSetManager, VOICE_COUNT> managers_;

  /////////////////////////////////////////////////////////////////////////////
  /// @brief
  /// @param setup
  /// @return
  static std::array<TNoteEventSetManager, VOICE_COUNT> make_managers(
    const TSetup& setup)
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        TNoteEventSetManager{ setup },
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        TNoteEventSetManager{ setup },
        TNoteEventSetManager{ setup },
      } };
    } else if constexpr (VOICE_COUNT == 3) {
      return { {
        TNoteEventSetManager{ setup },
        TNoteEventSetManager{ setup },
        TNoteEventSetManager{ setup },
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        TNoteEventSetManager{ setup },
        TNoteEventSetManager{ setup },
        TNoteEventSetManager{ setup },
        TNoteEventSetManager{ setup },
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
  static std::array<TPersonaRole, VOICE_COUNT> make_roles()
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        // Single voice?
        TPersonaRole(music::NoteValue::Quarter,
                     0,
                     music::SCALE_WEIGHTS_7_UNIFORM,
                     0.6F,
                     0.9F,
                     0.0F,
                     0.5F),
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        // Bass
        TPersonaRole(music::NoteValue::Half,
                     -2,
                     music::SCALE_WEIGHTS_7_CHORD_TONE_HEAVY,
                     0.6F,
                     0.9F,
                     0.0F,
                     0.3F),
        // Treble
        TPersonaRole(music::NoteValue::Eighth,
                     2,
                     music::SCALE_WEIGHTS_7_UNIFORM,
                     0.6F,
                     0.9F,
                     0.2F,
                     0.6F),
      } };
    } else if constexpr (VOICE_COUNT == 3) {
      return { {
        // Bass
        TPersonaRole(music::NoteValue::Half,
                     -2,
                     music::SCALE_WEIGHTS_7_TONIC_HEAVY,
                     0.6F,
                     0.9F,
                     0.0F,
                     0.3F),
        // Mids
        TPersonaRole(music::NoteValue::Quarter,
                     0,
                     music::SCALE_WEIGHTS_7_CHORD_TONE_HEAVY,
                     0.2F,
                     0.7F,
                     0.2F,
                     0.6F),
        // Treble
        TPersonaRole(music::NoteValue::Eighth,
                     2,
                     music::SCALE_WEIGHTS_7_UNIFORM,
                     0.6F,
                     0.9F,
                     0.3F,
                     0.6F),
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        // Bass
        TPersonaRole(music::NoteValue::Half,
                     -2,
                     music::SCALE_WEIGHTS_7_TONIC_HEAVY,
                     0.7F,
                     0.9F,
                     0.0F,
                     0.0F),
        // Tenor
        TPersonaRole(music::NoteValue::Quarter,
                     -1,
                     music::SCALE_WEIGHTS_7_CHORD_TONE_HEAVY,
                     0.3F,
                     0.7F,
                     0.4F,
                     0.4F),
        // Alto
        TPersonaRole(music::NoteValue::Eighth,
                     0,
                     music::SCALE_WEIGHTS_7_CHORD_TONE_HEAVY,
                     0.3F,
                     0.8F,
                     0.0F,
                     0.0F),
        // Soprano
        TPersonaRole(music::NoteValue::Eighth,
                     1,
                     music::SCALE_WEIGHTS_7_UNIFORM,
                     0.2F,
                     0.98F,
                     0.1F,
                     0.5F),
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
  std::array<TPersona, VOICE_COUNT> make_personas()
  {
    if constexpr (VOICE_COUNT == 1) {
      return { {
        TPersona{ "BASS VOICE", TBasicApp::setup, roles_[0] },
      } };
    } else if constexpr (VOICE_COUNT == 2) {
      return { {
        TPersona{ "BASS VOICE", TBasicApp::setup, roles_[0] },
        TPersona{ "SOPRANO VOICE", TBasicApp::setup, roles_[1] },
      } };
    } else if constexpr (VOICE_COUNT == 3) {
      return { {
        TPersona{ "BASS VOICE", TBasicApp::setup, roles_[0] },
        TPersona{ "ALTO VOICE", TBasicApp::setup, roles_[1] },
        TPersona{ "SOPRANO VOICE", TBasicApp::setup, roles_[2] },
      } };
    } else if constexpr (VOICE_COUNT == 4) {
      return { {
        TPersona{ "BASS VOICE", TBasicApp::setup, roles_[0] },
        TPersona{ "TENOR VOICE", TBasicApp::setup, roles_[1] },
        TPersona{ "ALTO VOICE", TBasicApp::setup, roles_[2] },
        TPersona{ "SOPRANO VOICE", TBasicApp::setup, roles_[3] },
      } };
    } else {
      static_assert(VOICE_COUNT == 1 || VOICE_COUNT == 2 || VOICE_COUNT == 3 ||
                      VOICE_COUNT == 4,
                    "Unsupported ChoirSATB voice count");
    }
  }
};
