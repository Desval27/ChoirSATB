#pragma once

#include <daisy_seed.h>
#include <daisysp.h>

using namespace daisysp;
using namespace daisy;

#include "App.h"
#include "Voice.h"

class VoiceMenuPage : public UiPage
{
  public:
    VoiceMenuPage(int voiceIndex) : _voiceIndex(voiceIndex) {}

    // inherited from UiPage
    void Draw(const UiCanvasDescriptor& canvas) override
    {
        const FontDef& TitleFont = Font_11x18;
        const FontDef& WaveformFont = Font_7x10;

        TheApp&   theApp = TheApp::instance();
        TheVoice* v      = theApp.GetVoice(_voiceIndex);
        auto*     d      = static_cast<MyDisplay*>(canvas.handle_);

        d->Fill(false);
        Rectangle titleRect(0, 0, d->Width(), TitleFont.FontHeight + 4);
        d->WriteStringAligned(
            v->GetName(), TitleFont, titleRect, Alignment::centered, true);

        Rectangle waveformRect(0, titleRect.GetBottom(), d->Width(), WaveformFont.FontHeight + 2);
        d->WriteStringAligned(
            v->GetWaveformName(), WaveformFont, waveformRect, Alignment::centered, true);

        Rectangle adsrRect(0,
                           waveformRect.GetBottom(),
                           d->Width(),
                           d->Height() - waveformRect.GetBottom() - 1);
        drawADSR(d, adsrRect, v);
        d->Update();
    }

    virtual bool OnArrowButton(ArrowButtonType arrowType,
                               uint8_t         numberOfPresses,
                               bool            isRetriggering)
    {
        return UiPage::OnArrowButton(
            arrowType, numberOfPresses, isRetriggering);
    }

    virtual bool OnMenuEncoderTurned(int16_t  turns,
                                     uint16_t stepsPerRevolution) override
    {
        TheApp&   theApp = TheApp::instance();
        TheVoice* v      = theApp.GetVoice(_voiceIndex);

        if(turns > 0)
        {
            v->SetWaveform(wrap(v->GetWaveform() + 1,
                                static_cast<int>(Oscillator::WAVE_LAST) - 1));
        }
        else if(turns < 0)
        {
            v->SetWaveform(wrap(v->GetWaveform() - 1,
                                static_cast<int>(Oscillator::WAVE_LAST) - 1));
        }
        return UiPage::OnMenuEncoderTurned(turns, stepsPerRevolution);
    }

    virtual bool OnPotMoved(uint16_t potID, float newPosition) override
    {
        TheApp&   theApp = TheApp::instance();
        TheVoice* v      = theApp.GetVoice(_voiceIndex);
        
        switch (potID)
        {
            case POT_1:
                v->SetAttackAs0to1(newPosition);
                break;
            case POT_2:
                v->SetDecayAs0to1(newPosition);
                break;
            case POT_3:
                v->SetSustainAs0to1(newPosition);
                break;
            case POT_4:
                v->SetReleaseAs0to1(newPosition);
                break;
        }
        return true;
    }

    virtual bool OnCancelButton(uint8_t numberOfPresses,
                                bool    isRetriggering) override
    {
        Close();
        return true;
    }

  private:
    int _voiceIndex;

    void drawADSR(MyDisplay* d, const Rectangle& rect, TheVoice* v)
    {
        // 1. Calculate X-positions (splitting width into 4 segments)
        const int segW = rect.GetWidth() / 4;
        const int x0   = rect.GetX();                        // Start
        const int x1   = x0 + (v->GetAttackAs0to1() * segW); // End of Attack
        const int x2   = x1 + (v->GetDecayAs0to1() * segW);  // End of Decay
        const int x3   = x2 + segW; // End of Sustain (held)
        const int x4   = x3 + (v->GetReleaseAs0to1() * segW); // End of Release

        // 2. Calculate Y-positions (Inverted)
        const int yBottom = rect.GetY() + rect.GetHeight();
        const int yTop    = rect.GetY();
        const int ySus = yBottom - (v->GetSustainAs0to1() * rect.GetHeight());

        // 3. Draw the lines
        d->DrawLine(x0, yBottom, x1, yTop, true); // Attack: Bottom to Peak
        d->DrawLine(x1, yTop, x2, ySus, true); // Decay: Peak to Sustain Level
        d->DrawLine(x2, ySus, x3, ySus, true); // Sustain: Constant Level
        d->DrawLine(x3, ySus, x4, yBottom, true); // Release: Sustain to Bottom
    }
};