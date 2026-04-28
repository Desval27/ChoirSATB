#include "Voice.h"

#include <cmath>

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param ts
/// @param tr
/// @param t
/// @param s
/// @param periodOffset
TheVoice::TheVoice(const Music::TimeSignature&   ts,
                   const Music::TuningReference& tr,
                   const Music::Temperament&     t,
                   const Music::ScaleMap<>&      s,
                   int                           periodOffset,
                   float                         attack,
                   float                         decay,
                   float                         sustain,
                   float                         release)
: config(periodOffset,
         daisysp::Oscillator::WAVE_TRI,
         attack,
         decay,
         sustain,
         release),
  _ts(&ts),
  _tr(&tr),
  _t(&t),
  _s(&s),
  _gate(false),
  _currentNoteIndex(-1)
{
    events.Clear();

    const float rootC4Hz
        = _t->FrequencyFromReference(Music::TemperedPitch(0, 0), *_tr);
    const float voiceRootHz
        = rootC4Hz * _t->PeriodMultiplier(config.periodOffset);
    _pe.SetTemperament(_t);
    _pe.SetScaleMap(_s);
    _pe.SetRootHz(voiceRootHz);

    _noteBuf[0] = '\0';
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param sample_rate
void TheVoice::Init(float sample_rate)
{
    //Set envelope parameters
    osc.Init(sample_rate);
    flt.Init(sample_rate);
    ampEnv.Init(sample_rate);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 
/// @return 
float TheVoice::Process() 
{
    osc.SetWaveform(config.waveform.Get());
    float env_out = ampEnv.Process(_gate);
    if(!std::isfinite(env_out))
        env_out = 0.0f;
    osc.SetAmp(env_out);
    const float sig = osc.Process();
    return std::isfinite(sig) ? sig : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
void TheVoice::DoPulse(int pulse)
{
    _currentNoteIndex = events.GetEventIndexForPulse(pulse);
    if(_currentNoteIndex < 0 || _currentNoteIndex >= static_cast<int>(events.Count()))
    {
        _gate = false;
        return;
    }

    const Music::NoteEvent& currentEvent = events[_currentNoteIndex];
    if(IsEventRisingEdge(pulse))
        _gate = (currentEvent.note != Music::REST);
    else if(IsEventFallingEdge(pulse))
        _gate = false;

    ampEnv.SetAttackTime(config.ampAdsr.attack);
    ampEnv.SetDecayTime(config.ampAdsr.decay);
    ampEnv.SetSustainLevel(config.ampAdsr.sustain);
    ampEnv.SetReleaseTime(config.ampAdsr.release);

    if(currentEvent.note != Music::REST)
    {
        HandleNoteEvent(pulse, currentEvent);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 
/// @param pulse 
/// @param ne 
void TheVoice::HandleNoteEvent(int pulse, Music::NoteEvent ne)
{
    SetBaseFrequency(GetFreqForNote(ne.note, ne.period));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 
/// @param pulse 
/// @param ne 
void TheVoice::SetBaseFrequency(float value)
{
    if(std::isfinite(value) && value > 0.0f)
    {
        _baseFrequency = value;
        osc.SetFreq(_baseFrequency);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
void TheVoice::Update()
{
    if(_currentNoteIndex < 0 || _currentNoteIndex >= static_cast<int>(events.Count()))
    {
        _noteBuf[0] = '\0';
        return;
    }

    // Update our text here outside of the main audio event handler
    if(events[_currentNoteIndex].note == Music::REST)
    {
        _noteBuf[0] = '\0';
    }
    else
    {
        char noteName[6];
        _t->GetNoteLabel(events[_currentNoteIndex].note, noteName, sizeof(noteName));
        snprintf(_noteBuf,
                 sizeof(_noteBuf),
                 "%s-%d",
                 noteName,
                 4 + config.periodOffset + events[_currentNoteIndex].period);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param n
/// @param p
/// @param fc
/// @return
float TheVoice::GetFreqForNote(Music::Note n, Music::Period p, float fc) const
{
    return _pe.Frequency(Music::TemperedPitch(n, p, fc));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param unitRandom
/// @param outPeriodOffset
/// @return
Music::Note TheVoice::GetWeightedNote(float unitRandom, int& outPeriodOffset)
{
    return _s->GetWeightedNote(unitRandom, outPeriodOffset, _weights);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param root
/// @param index
/// @param outPeriodOffset
/// @return
Music::Degree TheVoice::GetMappedDegreeFromRoot(Music::Degree root,
                                                int           index,
                                                int& outPeriodOffset) const
{
    int rootIdx = _s->GetIndexOfDegree(root);
    return _s->GetMappedDegree(rootIdx + index, outPeriodOffset);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
/// @return
bool TheVoice::IsEventRisingEdge(int pulse) const
{
    return GetEventPulseOffset(pulse) == 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
/// @param articulation
/// @return
int TheVoice::GetEventPulseOffset(int pulse) const
{
    if(events.Count() == 0 || pulse < 0)
        return -1;

    const int totalPulses = events.GetTotalEventPulses();
    if(totalPulses <= 0)
        return -1;

    const int eventIdx = events.GetEventIndexForPulse(pulse);
    if(eventIdx < 0 || eventIdx >= static_cast<int>(events.Count()))
        return -1;

    const int eventStartPulse
        = events.GetEventStartPulse(static_cast<size_t>(eventIdx));
    return ((pulse % totalPulses) - eventStartPulse + totalPulses) % totalPulses;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
/// @param articulation
/// @return
bool TheVoice::IsEventFallingEdge(int                 pulse,
                                  Music::Articulation articulation) const
{
    if(events.Count() == 0 || pulse < 0)
        return false;

    const int totalPulses = events.GetTotalEventPulses();
    if(totalPulses <= 0)
        return false;

    int previousPulse = pulse - 1;
    if(pulse == 0)
    {
        if(totalPulses > 0)
            previousPulse = totalPulses - 1;
    }

    const Music::NoteEvent& currentEvent  = events.GetEventForPulse(pulse);
    const Music::NoteEvent& previousEvent = events.GetEventForPulse(previousPulse);

    // Always release when the sequence transitions from note to rest.
    if(previousEvent.note != Music::REST && currentEvent.note == Music::REST)
        return true;

    // Legato keeps the gate high between adjacent note events.
    if(articulation == Music::Articulation::Legato
       || currentEvent.note == Music::REST)
        return false;

    const int eventIdx = events.GetEventIndexForPulse(pulse);
    if(eventIdx < 0 || eventIdx >= static_cast<int>(events.Count()))
        return false;

    const int eventPulseOffset = GetEventPulseOffset(pulse);
    if(eventPulseOffset < 0)
        return false;

    const int span = static_cast<int>(events[eventIdx].value);
    if(span <= 1)
        return true;

    float gateFraction = 0.90f; // Normal articulation.
    if(articulation == Music::Articulation::Staccato)
        gateFraction = 0.55f;

    int releasePulseOffset = static_cast<int>(span * gateFraction);
    if(releasePulseOffset < 0)
        releasePulseOffset = 0;
    if(releasePulseOffset > (span - 1))
        releasePulseOffset = span - 1;

    return eventPulseOffset == releasePulseOffset;
}
