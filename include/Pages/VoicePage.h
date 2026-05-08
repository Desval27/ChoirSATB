#pragma once

#include <cstdint>
#include <daisy_seed.h>
#include <daisysp.h>

#include <Voice.h>

class VoicePage : public daisy::UiPage
{
public:
  VoicePage(int voiceIndex)
    : _voiceIndex(voiceIndex)
  {
  }

  virtual bool OnArrowButton(daisy::ArrowButtonType arrowType,
                             uint8_t numberOfPresses,
                             bool isRetriggering) override;

  virtual bool OnMenuEncoderTurned(int16_t turns,
                                   uint16_t stepsPerRevolution) override;

  virtual bool OnPotMoved(uint16_t potID, float newPosition) override;

  virtual bool OnCancelButton(uint8_t numberOfPresses,
                              bool isRetriggering) override;

  void Draw(const daisy::UiCanvasDescriptor& canvas) override;

private:
  int _voiceIndex;

  void DrawADSR(MyDisplay* d, const daisy::Rectangle& rect, TheVoice* v);
};