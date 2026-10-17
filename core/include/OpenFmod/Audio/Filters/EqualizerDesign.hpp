#ifndef OPENFMOD_AUDIO_FILTERS_EQUALIZERDESIGN_HPP
#define OPENFMOD_AUDIO_FILTERS_EQUALIZERDESIGN_HPP
#include "OpenFmod/Audio/Filters/Biquad.hpp"
namespace OpenFmod::Audio {
enum class FilterType {
    Disabled, LowPass12, LowPass24, LowPass48, HighPass12, HighPass24, HighPass48,
    LowShelf, HighShelf, Peaking, BandPass, Notch, AllPass, LowPass6, HighPass6
};
struct EqualizerBand {
    FilterType Type{FilterType::Disabled};
    float Frequency{600};
    float Quality{0.7071067811865475f};
    float GainDecibels{};
};
BiquadCoefficients DesignEqualizerFilter(std::uint32_t sampleRate, EqualizerBand band);
std::size_t FilterStages(FilterType type);
}
#endif
