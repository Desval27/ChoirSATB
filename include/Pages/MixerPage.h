#pragma once

#include <daisy_seed.h>
#include <daisysp.h>

#include <Pages/BasePage.h>

class MixerPage : public BasePage<false>
{
public:
  MixerPage() {}

  virtual bool OnPotMoved(uint16_t potID, float newPosition) override
  {
    // TheApp &theApp = TheApp::instance();
    // TheVoice *v = theApp.GetVoice(potID);
    // v->SetVolumeAs0to1(newPosition);
    return true;
  }

  virtual bool OnCancelButton(uint8_t numberOfPresses,
                              bool isRetriggering) override
  {
    if (numberOfPresses > 0 && !isRetriggering)
      Close();
    return true;
  }

protected:
  void InternalDraw(daisy::OneBitGraphicsDisplay& display,
                    uint32_t nowMS) override
  {

    // TheApp &theApp = TheApp::instance();

    const FontDef& TitleFont = Font_11x18;
    const FontDef& VoiceFont = Font_6x8;

    Rectangle titleRect(0, 0, display.Width(), TitleFont.FontHeight + 4);
    display.WriteStringAligned(
      "MIXER", TitleFont, titleRect, Alignment::centered, true);

    // const int voiceWidth = display.Width() / theApp.NUM_VOICES;
    // const int voiceHeight = VoiceFont.FontHeight + 4;
    // for (int i = 0; i < theApp.NUM_VOICES; i++) {
    //   FixedCapStr<16> txtBuf;

    //   Rectangle voiceRect((i * voiceWidth), titleRect.GetBottom(),
    //   voiceWidth,
    //                       voiceHeight);
    //   TheVoice *v = theApp.GetVoice(i);
    //   display.WriteStringAligned(v->GetName(), VoiceFont, voiceRect,
    //                              Alignment::centered, true);

    //   const int fullBarHeight = display.Height() - voiceRect.GetBottom();
    //   const int barHeight = (int)((float)fullBarHeight *
    //   v->GetVolumeAs0to1());

    //   Rectangle volumeRect((i * voiceWidth),
    //                        voiceRect.GetBottom() + (fullBarHeight -
    //                        barHeight), voiceWidth, barHeight);

    //   v->AppendVolumeToString(txtBuf);
    //   display.WriteStringAligned(txtBuf.Cstr(), Font_6x8, volumeRect,
    //                              Alignment::centered, true);

    //   display.DrawRect(volumeRect.GetX(), volumeRect.GetY(),
    //                    volumeRect.GetRight() - 1, volumeRect.GetBottom() - 1,
    //                    true, false);
    // }
  }
};
