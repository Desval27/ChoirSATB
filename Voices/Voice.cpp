#include "Voice.h"

/**
 * @brief Constructs a new TheVoice object, which is an abstract class representing a voice in the choir. 
 * This constructor initializes the voice's pitch engine based on the provided temperament, scale map, 
 * and tuning reference, and sets up the root frequency for the voice based on the given period offset.
 * 
 * @param hw A reference to the DaisySeed hardware object, which can be used by derived classes to interact with the hardware.
 * @param ts A reference to the time signature object, which defines the rhythmic structure of the music.
 * @param tr A reference to the tuning reference object, which provides the reference pitch for the voice.
 * @param t A reference to the temperament object, which defines the tuning system for the voice.
 * @param s A reference to the scale map object, which maps scale degrees to pitches.
 * @param periodOffset The offset for the period, which determines the root frequency of the voice.
 */
TheVoice::TheVoice(const DaisySeed&       hw,
                   const TimeSignature&   ts,
                   const TuningReference& tr,
                   const Temperament&     t,
                   const ScaleMap&        s,
                   int                    periodOffset)
: hw(&hw),
  eventsLen(0),
  _ts(&ts),
  _tr(&tr),
  _t(&t),
  _s(&s),
  _periodOffset(periodOffset),
  _weights(nullptr),
  _weightCount(0),
  _gate(false)
{
    const float rootC4Hz
        = _t->FrequencyFromReference(TemperedPitch(0, 0), *_tr);
    const float voiceRootHz = rootC4Hz * _t->PeriodMultiplier(_periodOffset);
    _pe.SetTemperament(_t);
    _pe.SetScaleMap(_s);
    _pe.SetRootHz(voiceRootHz);

    _noteBuf[0] = '\0';
}

////////////////////////////////////////////////////////////////////////////////
void TheVoice::Init(float sample_rate)
{
    //Set envelope parameters
    env.Init(sample_rate);
    env.SetTime(ADSR_SEG_ATTACK, .1);
    env.SetTime(ADSR_SEG_DECAY, .1);
    env.SetSustainLevel(.70);
    env.SetTime(ADSR_SEG_RELEASE, .1);
}

////////////////////////////////////////////////////////////////////////////////
void TheVoice::DoPulse(int pulse)
{
    _currentNote = GetEventForPulse(pulse);
    if(_currentNote.note != REST && IsEventRisingEdge(pulse))
        _gate = true;
    if(IsEventFallingEdge(pulse))
        _gate = false;

    if(_currentNote.note != REST)
    {
        HandleNoteEvent(pulse, _currentNote);
    }
}

////////////////////////////////////////////////////////////////////////////////
void TheVoice::Update()
{
    // Update our text here outside of the main audio event handler
    if(_currentNote.note == REST)
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
                 4 + _periodOffset + _currentNote.period);
    }
}

////////////////////////////////////////////////////////////////////////////////
void TheVoice::SetWeights(const float weights[], size_t weightCount)
{
    _weights     = weights;
    _weightCount = weightCount;
}

/**
 * @brief Determines if the event at the given pulse is a rising edge, meaning the gate should be activated.
 * 
 * @param pulse 
 * @return true 
 * @return false 
 */
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

/**
 * @brief Determines if the event at the given pulse is a falling edge, meaning the gate should be released.
 * 
 * @param pulse 
 * @param articulation 
 * @return true 
 * @return false 
 */
bool TheVoice::IsEventFallingEdge(int pulse, Articulation articulation) const
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

    const NoteEvent& currentEvent  = GetEventForPulse(pulse);
    const NoteEvent& previousEvent = GetEventForPulse(previousPulse);

    // Always release when the sequence transitions from note to rest.
    if(previousEvent.note != REST && currentEvent.note == REST)
        return true;

    // Legato keeps the gate high between adjacent note events.
    if(articulation == Articulation::Legato || currentEvent.note == REST)
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
    if(articulation == Articulation::Staccato)
        gateFraction = 0.55f;

    int releasePulseOffset = static_cast<int>(span * gateFraction);
    if(releasePulseOffset < 0)
        releasePulseOffset = 0;
    if(releasePulseOffset > (span - 1))
        releasePulseOffset = span - 1;

    return eventPulseOffset == releasePulseOffset;
}

////////////////////////////////////////////////////////////////////////////////
const Music::NoteEvent& TheVoice::GetEventForPulse(int pulse) const
{
    int eventIdx = FindAssociatedEventIndex(pulse);
    if(eventIdx >= 0 && eventIdx < static_cast<int>(eventsLen))
        return events[eventIdx];
    return emptyNote;
}

////////////////////////////////////////////////////////////////////////////////
int TheVoice::GetEventStartPulse(size_t eventIndex) const
{
    int pulseCursor = 0;
    for(size_t i = 0; i < eventIndex && i < eventsLen; i++)
        pulseCursor += static_cast<int>(events[i].value);
    return pulseCursor;
}

////////////////////////////////////////////////////////////////////////////////
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
