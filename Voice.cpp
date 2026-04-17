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
  eventsLen(0),
  _ts(&ts),
  _tr(&tr),
  _t(&t),
  _s(&s),
  _weights(nullptr),
  _weightCount(0),
  _gate(false)
{
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
    _currentNote = GetEventForPulse(pulse);
    if(_currentNote.note != Music::REST && IsEventRisingEdge(pulse))
        _gate = true;
    if(IsEventFallingEdge(pulse))
        _gate = false;

    ampEnv.SetAttackTime(config.ampAdsr.attack);
    ampEnv.SetDecayTime(config.ampAdsr.decay);
    ampEnv.SetSustainLevel(config.ampAdsr.sustain);
    ampEnv.SetReleaseTime(config.ampAdsr.release);

    if(_currentNote.note != Music::REST)
    {
        HandleNoteEvent(pulse, _currentNote);
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
    if(_currentNote.note == Music::REST)
    {
        _noteBuf[0] = '\0';
    }
    else
    {
        char noteName[6];
        _t->GetIntervalLabel(_currentNote.note, noteName, sizeof(noteName));
        snprintf(_noteBuf,
                 sizeof(_noteBuf),
                 "%s-%d",
                 noteName,
                 4 + config.periodOffset + _currentNote.period);
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
/// @return
int TheVoice::GetTotalEventPulses() const
{
    int totalPulses = 0;
    for(size_t i = 0; i < eventsLen; i++)
        totalPulses += static_cast<int>(events[i].value);
    return totalPulses;
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
int TheVoice::GetEventIndexForPulse(int pulse) const
{
    return FindAssociatedEventIndex(pulse);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
/// @return
bool TheVoice::IsEventRisingEdge(int pulse) const
{
    if(eventsLen == 0 || pulse < 0)
        return false;

    const int currentEventIdx = GetEventIndexForPulse(pulse);
    if(currentEventIdx < 0)
        return false;

    int previousPulse = pulse - 1;
    if(pulse == 0)
    {
        const int totalPulses = GetTotalEventPulses();
        if(totalPulses > 0)
            previousPulse = totalPulses - 1;
    }

    const int previousEventIdx = GetEventIndexForPulse(previousPulse);
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
    if(eventsLen == 0 || pulse < 0)
        return false;

    const int totalPulses = GetTotalEventPulses();
    if(totalPulses <= 0)
        return false;

    int previousPulse = pulse - 1;
    if(pulse == 0)
    {
        if(totalPulses > 0)
            previousPulse = totalPulses - 1;
    }

    const Music::NoteEvent& currentEvent  = GetEventForPulse(pulse);
    const Music::NoteEvent& previousEvent = GetEventForPulse(previousPulse);

    // Always release when the sequence transitions from note to rest.
    if(previousEvent.note != Music::REST && currentEvent.note == Music::REST)
        return true;

    // Legato keeps the gate high between adjacent note events.
    if(articulation == Music::Articulation::Legato
       || currentEvent.note == Music::REST)
        return false;

    const int eventIdx = GetEventIndexForPulse(pulse);
    if(eventIdx < 0 || eventIdx >= static_cast<int>(eventsLen))
        return false;

    const int eventStartPulse
        = GetEventStartPulse(static_cast<size_t>(eventIdx));
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

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
/// @return
const Music::NoteEvent& TheVoice::GetEventForPulse(int pulse) const
{
    int eventIdx = FindAssociatedEventIndex(pulse);
    if(eventIdx >= 0 && eventIdx < static_cast<int>(eventsLen))
        return events[eventIdx];
    return emptyNote;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param eventIndex
/// @return
int TheVoice::GetEventStartPulse(size_t eventIndex) const
{
    int pulseCursor = 0;
    for(size_t i = 0; i < eventIndex && i < eventsLen; i++)
        pulseCursor += static_cast<int>(events[i].value);
    return pulseCursor;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief
/// @param pulse
/// @return
int TheVoice::FindAssociatedEventIndex(int pulse) const
{
    if(eventsLen == 0 || pulse < 0)
        return -1;

    int totalPulses = GetTotalEventPulses();

    if(totalPulses <= 0)
        return -1;

    const int normalizedPulse = pulse % totalPulses;

    int pulseCursor = 0;
    for(size_t i = 0; i < eventsLen; i++)
    {
        const int span = static_cast<int>(events[i].value);
        if(normalizedPulse < (pulseCursor + span))
            return static_cast<int>(i);

        pulseCursor += span;
    }

    return static_cast<int>(eventsLen - 1);
}
