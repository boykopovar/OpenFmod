#include "OpenFmod/Audio/Reverb/Settings.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <array>
#include <cmath>
namespace OpenFmod::Audio {
void ReverbSettings::Validate() const {
    const std::array<float, 13> values{DecayMilliseconds, EarlyDelayMilliseconds, LateDelayMilliseconds,
        HighFrequencyReference, HighFrequencyDecayPercent, DiffusionPercent, DensityPercent,
        LowShelfFrequency, LowShelfDecibels, HighCutFrequency, LateMixPercent, WetDecibels, DryDecibels};
    const std::array<float, 13> minimum{100, 0, 0, 20, 10, 0, 0, 20, -36, 20, 0, -80, -80};
    const std::array<float, 13> maximum{20000, 300, 100, 20000, 100, 100, 100, 1000, 12, 20000, 100, 20, 20};
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (!std::isfinite(values[index]) || values[index] < minimum[index] || values[index] > maximum[index]) {
            throw FormatError("Invalid reverb parameter");
        }
    }
}
}
