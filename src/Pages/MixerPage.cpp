#include <App.h>
#include <Pages/MixerPage.h>

using namespace daisy;
using namespace daisysp;

bool MixerPage::OnPotMoved(uint16_t potID, float newPosition)
{
    TheApp&   theApp = TheApp::instance();
    TheVoice* v      = theApp.GetVoice(potID);
    v->SetVolumeAs0to1(newPosition);
    return true;
}

bool MixerPage::OnCancelButton(uint8_t numberOfPresses, bool isRetriggering)
{
    Close();
    return true;
}

void MixerPage::Draw(const daisy::UiCanvasDescriptor& canvas)
{
    const FontDef& TitleFont = Font_11x18;
    const FontDef& VoiceFont = Font_6x8;
    TheApp&        theApp    = TheApp::instance();
    auto*          d         = static_cast<MyDisplay*>(canvas.handle_);

    d->Fill(false);

    Rectangle titleRect(0, 0, d->Width(), TitleFont.FontHeight + 4);
    d->WriteStringAligned("MIXER", TitleFont, titleRect, Alignment::centered, true);

    const int voiceWidth = d->Width() / theApp.NUM_VOICES;
    const int voiceHeight = VoiceFont.FontHeight + 4;
    for (int i = 0; i < theApp.NUM_VOICES; i++)
    {
        FixedCapStr<16> txtBuf;

        Rectangle voiceRect((i * voiceWidth), titleRect.GetBottom(), voiceWidth, voiceHeight);
        TheVoice *v = theApp.GetVoice(i);
        d->WriteStringAligned(v->GetName(), VoiceFont, voiceRect, Alignment::centered, true);

        
        const int fullBarHeight = d->Height() - voiceRect.GetBottom();
        const int barHeight = (int)((float)fullBarHeight * v->GetVolumeAs0to1());

        Rectangle volumeRect((i * voiceWidth), voiceRect.GetBottom() + (fullBarHeight - barHeight), voiceWidth, barHeight);

        v->AppendVolumeToString(txtBuf);
        d->WriteStringAligned(txtBuf.Cstr(), Font_6x8, volumeRect, Alignment::centered, true);

        d->DrawRect(volumeRect.GetX(), volumeRect.GetY(), volumeRect.GetRight()-1, volumeRect.GetBottom()-1, true, false);
    }

    d->Update();
}