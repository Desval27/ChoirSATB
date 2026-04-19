#include "Voice.h"

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
                   const Music::ScaleMap&        s,
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
  _weights(nullptr),
  _weightCount(0),
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
    osc.SetAmp(env_out);
    return osc.Process();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
void TheVoice::DoPulse(int pulse)
{
    _currentNoteIndex = events.GetEventIndexForPulse(pulse);
    if(events[_currentNoteIndex].note != Music::REST && IsEventRisingEdge(pulse))
        _gate = true;
    if(IsEventFallingEdge(pulse))
        _gate = false;

    ampEnv.SetAttackTime(config.ampAdsr.attack);
    ampEnv.SetDecayTime(config.ampAdsr.decay);
    ampEnv.SetSustainLevel(config.ampAdsr.sustain);
    ampEnv.SetReleaseTime(config.ampAdsr.release);

    if(events[_currentNoteIndex].note != Music::REST)
    {
        HandleNoteEvent(pulse, events[_currentNoteIndex]);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief 
/// @param pulse 
/// @param ne 
void TheVoice::HandleNoteEvent(int pulse, Music::NoteEvent ne)
{
    osc.SetFreq(GetFreqForNote(ne.note, ne.period));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
void TheVoice::Update()
{
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
void TheVoice::SetWeights(const float weights[], size_t weightCount)
{
    _weights     = weights;
    _weightCount = weightCount;
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
    return _s->GetWeightedNote(
        unitRandom, outPeriodOffset, _weights, _weightCount);
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
    if(events.Count() == 0 || pulse < 0)
        return false;

    const int currentEventIdx = events.GetEventIndexForPulse(pulse);
    if(currentEventIdx < 0)
        return false;

    int previousPulse = pulse - 1;
    if(pulse == 0)
    {
        const int totalPulses = events.GetTotalEventPulses();
        if(totalPulses > 0)
            previousPulse = totalPulses - 1;
    }

    const int previousEventIdx = events.GetEventIndexForPulse(previousPulse);
    return currentEventIdx != previousEventIdx;
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

    const int eventStartPulse
        = events.GetEventStartPulse(static_cast<size_t>(eventIdx));
    const int eventPulseOffset
        = ((pulse % totalPulses) - eventStartPulse + totalPulses) % totalPulses;
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

