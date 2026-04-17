#pragma once

#include <daisy_seed.h>
#include <daisysp.h>

class MixerPage: public daisy::UiPage
{
  public:
    MixerPage() {}

    virtual bool OnPotMoved(uint16_t potID, float newPosition) override;
    virtual bool OnCancelButton(uint8_t numberOfPresses,
                                bool    isRetriggering) override;

    void Draw(const daisy::UiCanvasDescriptor& canvas) override;
};