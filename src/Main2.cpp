// Not a real main....just something to test ideas out with.

#include <daisy_seed.h>
#include <daisysp.h>
#include <dev/oled_ssd130x.h>

#include <UIOverlord.h>

#include <Music/MusicHelpers.h>

using namespace daisysp;
using namespace daisy;
using TheOverlord = UIOverlord<SSD130xI2c128x64Driver, 1, 0, 0>;

DaisySeed   hw;
TheOverlord uiOverlord;

const TheOverlord::EncoderConfig encoderConfig[] = {
    {seed::D20, seed::D16, seed::D19}, // ENCODER_1
    //     // {seed::D0, seed::D1, seed::D2},   // ENCODER_1
    //     // {seed::D3, seed::D4, seed::D5},   // ENCODER_2
    //     // {seed::D6, seed::D7, seed::D8},   // ENCODER_3
    //     // {seed::D9, seed::D10, seed::D15}, // ENCODER_4
};
const TheOverlord::ButtonConfig buttonConfig[] = {
    {seed::D19},
};
const TheOverlord::PotConfig potConfig[] = {
    {seed::A7},
    {seed::A8},
    {seed::A9},
    {seed::A10},
};


Metro   clock;
HiHat<> hat;


////////////////////////////////////////////////////////////////////////////////
// Main Audio Loop
////////////////////////////////////////////////////////////////////////////////
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    //Prepare the audio block
    for(size_t i = 0; i < size; i += 2)
    {
        bool trig = clock.Process();

        float sig = hat.Process(trig);

        // Null output for now.
        out[i]     = sig;
        out[i + 1] = sig;
    }
}

////////////////////////////////////////////////////////////////////////////////
int main(void)
{
    hw.Configure();
    hw.Init();

    hw.StartLog(false);

    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    //
    // Initialize our Audio Components
    //
    float sample_rate = hw.AudioSampleRate();

    uiOverlord.Init(sample_rate, &hw.adc, encoderConfig, buttonConfig, potConfig);

    clock.Init(Music::BpmToFreq(60.0f), sample_rate);

    hat.Init(sample_rate);

    hw.PrintLine("Starting Audio...");
    hw.StartAudio(AudioCallback);

    while(1)
    {
        uint32_t nowMS = System::GetNow();

        uiOverlord.Process();
    }
}
