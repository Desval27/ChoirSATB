#include <Types.h>
#include <Pages/MainPage.h>

bool MainPage::OnMenuEncoderTurned(int16_t turns, uint16_t stepsPerRevolution)
{
    // bpm = bpm.Get() + turns;
    return true;
}

bool MainPage::OnPotMoved(uint16_t potID, float newPosition)
{
    // voiceVolumes[potID].Set(newPosition * 100.0f);
    return true;
}

bool MainPage::OnOkayButton(uint8_t numberOfPresses, bool isRetriggering)
{
    // if(numberOfPresses > 0)
    // {
    //     ui.OpenPage(mainMenu);
    // }
    return true;
}

bool MainPage::OnCancelButton(uint8_t numberOfPresses, bool isRetriggering)
{
    // if(numberOfPresses > 0)
    // {
    //     theApp.SetScaleIndex(
    //         randomRange(D12StartIndex, D12StartIndex + D12Count));
    //     theApp.gnome.Reset();
    // }
    return true;
}

bool MainPage::OnButton(uint16_t buttonID,
                        uint8_t  numberOfPresses,
                        bool     isRetriggering)
{
    // if(numberOfPresses > 0)
    // {
    //     switch(buttonID)
    //     {
    //         case BTN_RUN_STOP:
    //             theApp.SetRunning(!theApp.GetRunning());
    //             break;
    //     }
    // }
    return true;
}

bool MainPage::OnArrowButton(daisy::ArrowButtonType arrowType,
                             uint8_t                numberOfPresses,
                             bool                   isRetriggering)
{
    // if(numberOfPresses > 0)
    // {
    //     switch(arrowType)
    //     {
    //         case daisy::ArrowButtonType::left:
    //             theApp.SetScaleIndex(theApp.GetScaleIndex() - 1);
    //             break;
    //         case daisy::ArrowButtonType::right:
    //             theApp.SetScaleIndex(theApp.GetScaleIndex() + 1);
    //             break;
    //         case daisy::ArrowButtonType::up: break;
    //         case daisy::ArrowButtonType::down: break;
    //     }
    // }
    return true;
}

void MainPage::Draw(const daisy::UiCanvasDescriptor& canvas)
{
    char  txtBuf[64];
    auto* d = static_cast<MyDisplay*>(canvas.handle_);

    d->Fill(false);

    // if(beatFlash)
    // {
    //     d->DrawRect(0, 0, d->Width() - 1, d->Height() - 1, true);
    // }
    // const int line1X = 2;
    // const int line1Y = 1;
    // const int line2X = 2;
    // const int line2Y = line1Y + Font_11x18.FontHeight;
    // const int line3Y = line2Y + Font_7x10.FontHeight;

    // const int voiceWidth = d->Width() / theApp.NUM_VOICES;
    // const int voiceLineY = d->Height() - Font_6x8.FontHeight - 4;
    // const int voiceStartX[theApp.NUM_VOICES] = {
    //     (voiceWidth * 0) + 6,
    //     (voiceWidth * 1) + 6,
    //     (voiceWidth * 2) + 6,
    //     (voiceWidth * 3) + 6,
    // };

    // const int barWidth                     = voiceWidth / 2;
    // const int barEndY                      = voiceLineY - 2;
    // const int barFullHeight                = barEndY - line3Y;
    // const int barStartX[theApp.NUM_VOICES] = {
    //     (voiceWidth * 0) + (barWidth / 2),
    //     (voiceWidth * 1) + (barWidth / 2),
    //     (voiceWidth * 2) + (barWidth / 2),
    //     (voiceWidth * 3) + (barWidth / 2),
    // };

    // // Line 1: Time Signature, BPM, Bar & Beat
    // d->SetCursor(line1X, line1Y);
    // snprintf(txtBuf,
    //          sizeof(txtBuf),
    //          "%2d/%-2d %2d:%-2d",
    //          theApp.GetTS()->beats,
    //          theApp.GetTS()->GetDenominator(),
    //          theApp.gnome.GetBar() + 1,
    //          theApp.gnome.GetBeat() + 1);
    // d->WriteString(txtBuf, Font_11x18, true);

    // d->SetCursor(line2X, line2Y);
    // snprintf(txtBuf,
    //          sizeof(txtBuf),
    //          "%-10s BPM:%3d",
    //          SCALE_TABLES[theApp.GetScaleIndex()].name,
    //          bpm.Get());
    // d->WriteString(txtBuf, Font_7x10, true);

    // // Now a line with all the voices current status
    // for(int i = 0; i < theApp.NUM_VOICES; i++)
    // {
    //     int barHeight
    //         = (int)(voiceVolumes[i].Get() / 100.0f * barFullHeight);
    //     int barStartY = barEndY - barHeight;
    //     d->DrawRect(barStartX[i],
    //                 barStartY,
    //                 barStartX[i] + barWidth,
    //                 barEndY,
    //                 true,
    //                 true);
    //     d->SetCursor(voiceStartX[i], voiceLineY);
    //     d->WriteString(theApp.GetVoice(i)->GetNoteText(), Font_6x8, true);
    // }

    d->Update();
}
