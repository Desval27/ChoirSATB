#pragma once

#include <daisy_seed.h>
#include <daisysp.h>

class MainPage : public daisy::UiPage
{
  public:
    MainPage() {}

    bool OnMenuEncoderTurned(int16_t  turns,
                             uint16_t stepsPerRevolution) override;

    bool OnPotMoved(uint16_t potID, float newPosition) override;

    bool OnOkayButton(uint8_t numberOfPresses, bool isRetriggering) override;

    bool OnCancelButton(uint8_t numberOfPresses, bool isRetriggering) override;

    bool OnButton(uint16_t buttonID,
                  uint8_t  numberOfPresses,
                  bool     isRetriggering) override;

    bool OnArrowButton(daisy::ArrowButtonType arrowType,
                       uint8_t                numberOfPresses,
                       bool                   isRetriggering) override;

    void Draw(const daisy::UiCanvasDescriptor& canvas) override;
};
