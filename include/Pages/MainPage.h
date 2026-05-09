#pragma once

#include <daisy_seed.h>
#include <daisysp.h>

#include <monkey.hpp>
#include <music/music.hpp>
#include <music/music_tables.hpp>

#include <Pages/BasePage.h>

extern FullScreenItemMenu mainMenu;

template<typename TApp>
class MainPage : public BasePage<true>
{
public:
  MainPage()
    : BasePage()
  {
  }

  bool OnEncoderTurned(uint16_t encoderID,
                       int16_t turns,
                       uint16_t stepsPerRevolution) override
  {
    if (turns != 0) {
      auto& theApp = TApp::get_instance();
      switch (encoderID) {
        case ENCODER_1:
          theApp.set_bpm(theApp.get_bpm() + turns);
          return true;
      }
    }
    return false;
  }

  bool OnButton(uint16_t buttonID,
                uint8_t numberOfPresses,
                bool isRetriggering) override
  {
    if (numberOfPresses > 0 && !isRetriggering) {
      auto& theApp = TApp::get_instance();
      switch (buttonID) {
        case BUTTON_1:
          theApp.set_running(!theApp.get_running());
          return true;
        case BUTTON_2:
          if (auto* ui = GetParentUI())
            ui->OpenPage(mainMenu);
          return true;
      }
    }
    return false;
  }

  bool OnPotMoved(uint16_t potID, float newPosition) override
  {
    // Pot id corresponds to voice id
    auto& theApp = TApp::get_instance();
    for (std::size_t i = 0; i < theApp.voices.size(); i++) {
      if (i == potID) {
        theApp.voices[i].config_.volume.SetFrom0to1(newPosition);
        return true;
      }
    }
    return false;
  }

  // if (numberOfPresses > 0 && !isRetriggering) {
  //   TheApp& theApp = TheApp::instance();
  //   switch (buttonID) {
  //     case BUTTON_1:
  //       theApp.ToggleRunning();
  //       return true;
  //     case BUTTON_2:
  //       if (auto* ui = GetParentUI())
  //         ui->OpenPage(mainMenu);
  //       return true;
  //     case BUTTON_3:
  //       theApp.Randomize();
  //       theApp.Reset();
  //       return true;
  //     case BUTTON_4:
  //       theApp.SetScaleIndex(theApp.GetScaleIndex() - 1);
  //       return true;
  //     case BUTTON_5:
  //       theApp.SetScaleIndex(theApp.GetScaleIndex() + 1);
  //       return true;
  //   }
  // }

protected:
  void InternalDraw(daisy::OneBitGraphicsDisplay& display,
                    uint32_t nowMS) override
  {
    auto& theApp = TApp::get_instance();

    // if(beatFlash)
    // {
    //     display.DrawRect(0, 0, display.Width() - 1, display.Height() - 1,
    //     true);
    // }
    // const int line1X = 2;
    // const int line1Y = 1;
    // const int line2X = 2;
    // const int line2Y = line1Y + Font_11x18.FontHeight;
    // const int line3X = 2;
    // const int line3Y = line2Y + Font_7x10.FontHeight;
    FixedCapStr<32> txt;

    display.SetCursor(4, 0);
    display.WriteString("CHOIR SATB", Font_11x18, true);
    display.SetCursor(0, 20);
    theApp.bpm_append(txt);
    txt.Append(theApp.get_running() ? " RUNNING" : " STOPPED");
    display.WriteString(txt.Cstr(), Font_7x10, true);

    // const int voiceWidth = display.Width() / theApp.NUM_VOICES;
    // const int voiceLineY = line3Y + Font_7x10.FontHeight + 2;

    // const int voiceStartX[theApp.NUM_VOICES] = {
    //   (voiceWidth * 0) + 6,
    //   (voiceWidth * 1) + 6,
    //   (voiceWidth * 2) + 6,
    //   (voiceWidth * 3) + 6,
    // };

    // const int barWidth = voiceWidth / 4;
    // const int barHeight = barWidth;
    // const int barStartY = voiceLineY + Font_6x8.FontHeight + 4;
    // const int barStartX[theApp.NUM_VOICES] = {
    //   (voiceWidth * 0) + (voiceWidth / 2) - (barWidth / 2),
    //   (voiceWidth * 1) + (voiceWidth / 2) - (barWidth / 2),
    //   (voiceWidth * 2) + (voiceWidth / 2) - (barWidth / 2),
    //   (voiceWidth * 3) + (voiceWidth / 2) - (barWidth / 2),
    // };

    // // Line 1: Time Signature, BPM, Bar & Beat
    // display.SetCursor(line1X, line1Y);
    // daisy::FixedCapStr<20> text;
    // theApp.GetTimingText(text);
    // display.WriteString(text.Cstr(), Font_11x18, true);

    // display.SetCursor(line2X, line2Y);
    // display.WriteString(
    //   music::HEPATONIC_D12_SCALES[theApp.GetScaleIndex()].name,
    //   Font_7x10,
    //   true);
    // display.SetCursor(line3X, line3Y);
    // theApp.AppendVolumeToString(txt);
    // txt.Append(" ");
    // txt.Append(theApp.GetChordText());
    // // theApp.snprintf(txtBuf, sizeof(txtBuf), "BPM: %3d", theApp.GetBPM());
    // display.WriteString(txt.Cstr(), Font_7x10, true);

    // // Now a line with all the voices current status
    // for (int i = 0; i < theApp.NUM_VOICES; i++) {
    //   //   int barHeight
    //   //     = (int)(voiceVolumes[i].Get() / 100.0f * barFullHeight);
    //   TheVoice* v = theApp.GetVoice(i);
    //   display.SetCursor(voiceStartX[i], voiceLineY);
    //   display.WriteString(v->GetNoteText(), Font_6x8, true);
    //   if (v->GetGate()) {
    //     display.DrawRect(barStartX[i],
    //                      barStartY,
    //                      barStartX[i] + barWidth,
    //                      barStartY + barHeight,
    //                      true,
    //                      true);
    //   }
    // }
  }
};
