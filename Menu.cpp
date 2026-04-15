#include <daisy_seed.h>
#include <daisysp.h>

#include <Monkey.h>

#include "Voices/Bass.h"

using namespace daisysp;
using namespace daisy;

bool uiOpen = false;

class MainMenuPage : public FullScreenItemMenu
{
  public:
    void OnShow() override
    {
        FullScreenItemMenu::OnShow();
        uiOpen = true;
    }

    void OnHide() override
    {
        FullScreenItemMenu::OnHide();
        uiOpen = false;
    }
};

MainMenuPage menu;
//FullScreenItemMenu scaleMenu;
FullScreenItemMenu bassMenu;
FullScreenItemMenu bassWaveformMenu;
FullScreenItemMenu tenorMenu;
FullScreenItemMenu altoMenu;
FullScreenItemMenu sopranoMenu;
UI                 ui;

extern MappedIntValue   bpm;
extern MappedFloatValue voiceAttack[];
extern MappedFloatValue voiceDecay[];
extern MappedFloatValue voiceSustain[];
extern MappedFloatValue voiceRelease[];
extern bool             running;
extern void             SetScaleIndex(int scaleIndex);
extern void             ResetState();

extern TheBass bass;

////////////////////////////////////////////////////////////////////////////////
void OpenMainMenu()
{
    if(!uiOpen)
    {
        ui.OpenPage(menu);
    }
}

////////////////////////////////////////////////////////////////////////////////
void CloseMainMenu()
{
    if(uiOpen)
    {
        ui.ClosePage(menu);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Menu Callbacks
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void EmptyMenuCallback(void* context)
{ (void)context; }


////////////////////////////////////////////////////////////////////////////////
void ResetValuesCallback(void* context)
{
    (void)context;
    ResetState();
}

////////////////////////////////////////////////////////////////////////////////
void CloseMenuCallback(void* context)
{
    (void)context;
    CloseMainMenu();
}

////////////////////////////////////////////////////////////////////////////////
void SetBassWaveformCallback(void* context)
{ bass.SetWaveform((int)context); }

////////////////////////////////////////////////////////////////////////////////
// UI & Menu Structure
////////////////////////////////////////////////////////////////////////////////
AbstractMenu::ItemConfig topItems[] = {
    {.type = AbstractMenu::ItemType::checkboxItem,
     .text = "RUNNING",
     .asCheckboxItem{&running}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "TEMPO",
     .asMappedValueItem{&bpm}},
    {.type = AbstractMenu::ItemType::openUiPageItem,
     .text = "BASS",
     .asOpenUiPageItem{&bassMenu}},
    {.type                   = AbstractMenu::ItemType::openUiPageItem,
     .text                   = "TENOR",
     .asOpenUiPageItem{&tenorMenu}},
    {.type                   = AbstractMenu::ItemType::openUiPageItem,
     .text                   = "ALTO",
     .asOpenUiPageItem{&altoMenu}},
    {.type                   = AbstractMenu::ItemType::openUiPageItem,
     .text                   = "SOPRANO",
     .asOpenUiPageItem{&sopranoMenu}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "RESET",
     .asCallbackFunctionItem = {ResetValuesCallback, nullptr}},
    {.type                   = AbstractMenu::ItemType::callbackFunctionItem,
     .text                   = "CLOSE",
     .asCallbackFunctionItem = {CloseMenuCallback, nullptr}},
};

AbstractMenu::ItemConfig bassItems[] = {
    {.type             = AbstractMenu::ItemType::openUiPageItem,
     .text             = "WAVEFORM",
     .asOpenUiPageItem = {&bassWaveformMenu}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "ATTACK",
     .asMappedValueItem{&voiceAttack[0]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "DECAY",
     .asMappedValueItem{&voiceDecay[0]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "SUSTAIN",
     .asMappedValueItem{&voiceSustain[0]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "RELEASE",
     .asMappedValueItem{&voiceRelease[0]}},
};
AbstractMenu::ItemConfig tenorItems[] = {
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "ATTACK",
     .asMappedValueItem{&voiceAttack[1]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "DECAY",
     .asMappedValueItem{&voiceDecay[1]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "SUSTAIN",
     .asMappedValueItem{&voiceSustain[1]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "RELEASE",
     .asMappedValueItem{&voiceRelease[1]}},
};
AbstractMenu::ItemConfig altoItems[] = {
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "ATTACK",
     .asMappedValueItem{&voiceAttack[2]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "DECAY",
     .asMappedValueItem{&voiceDecay[2]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "SUSTAIN",
     .asMappedValueItem{&voiceSustain[2]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "RELEASE",
     .asMappedValueItem{&voiceRelease[2]}},
};
AbstractMenu::ItemConfig sopranoItems[] = {
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "ATTACK",
     .asMappedValueItem{&voiceAttack[3]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "DECAY",
     .asMappedValueItem{&voiceDecay[3]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "SUSTAIN",
     .asMappedValueItem{&voiceSustain[3]}},
    {.type = AbstractMenu::ItemType::valueItem,
     .text = "RELEASE",
     .asMappedValueItem{&voiceRelease[3]}},
};

AbstractMenu::ItemConfig bassWaveformItems[] = {
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "SIN",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_SIN}},
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "TRI",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_TRI}},
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "SAW",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_SAW}},
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "RAMP",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_RAMP}},
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "SQUARE",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_SQUARE}},
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "POLY TRI",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_POLYBLEP_TRI}},
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "POLY SAW",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_POLYBLEP_SAW}},
    {.type = AbstractMenu::ItemType::callbackFunctionItem,
     .text = "POLY SQUARE",
     .asCallbackFunctionItem
     = {SetBassWaveformCallback, (void*)Oscillator::WAVE_POLYBLEP_SQUARE}},
};

void InitMenus()
{
    menu.Init(topItems,
              ArrayLen(topItems),
              AbstractMenu::Orientation::upDownSelectLeftRightModify,
              true);

    bassMenu.Init(bassItems,
                  ArrayLen(bassItems),
                  AbstractMenu::Orientation::upDownSelectLeftRightModify,
                  true);
    tenorMenu.Init(tenorItems,
                  ArrayLen(tenorItems),
                  AbstractMenu::Orientation::leftRightSelectUpDownModify,
                  true);
    altoMenu.Init(altoItems,
                  ArrayLen(altoItems),
                  AbstractMenu::Orientation::upDownSelectLeftRightModify,
                  true);
    sopranoMenu.Init(sopranoItems,
                  ArrayLen(sopranoItems),
                  AbstractMenu::Orientation::upDownSelectLeftRightModify,
                  true);

    bassWaveformMenu.Init(
        bassWaveformItems,
        ArrayLen(bassWaveformItems),
        AbstractMenu::Orientation::upDownSelectLeftRightModify,
        true);

    // scaleMenu.Init(scaleItems,
    //                ArrayLen(scaleItems),
    //                AbstractMenu::Orientation::upDownSelectLeftRightModify,
    //                true);
}
