#ifndef OPENFMOD_AUDIO_REVERB_SETTINGS_HPP
#define OPENFMOD_AUDIO_REVERB_SETTINGS_HPP
namespace OpenFmod::Audio {
struct ReverbSettings {
    float DecayMilliseconds{1500};
    float EarlyDelayMilliseconds{20};
    float LateDelayMilliseconds{40};
    float HighFrequencyReference{5000};
    float HighFrequencyDecayPercent{50};
    float DiffusionPercent{100};
    float DensityPercent{100};
    float LowShelfFrequency{250};
    float LowShelfDecibels{0};
    float HighCutFrequency{20000};
    float LateMixPercent{50};
    float WetDecibels{-6};
    float DryDecibels{0};
    void Validate() const;
};
}
#endif
