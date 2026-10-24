#include "OpenFmod/Audio/Dynamics/SignalTap.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
void SignalTap::SetGain(float value) {
    if (!std::isfinite(value) || value < 0) throw FormatError("Invalid sidechain gain");
    gain = value;
}
void SignalTap::Process(float* stereo, std::size_t frames) {
    if ((frames && !stereo) || frames > std::numeric_limits<std::size_t>::max() / 2) throw FormatError("Invalid sidechain buffer");
    samples.resize(frames * 2);
    for (std::size_t index = 0; index < samples.size(); ++index) {
        samples[index] = stereo[index] * gain;
        if (!std::isfinite(samples[index])) throw FormatError("Nonfinite sidechain signal");
    }
    initialized = true;
}
const std::vector<float>& SignalTap::GetSamples(std::size_t frames) const {
    if (!initialized || frames > std::numeric_limits<std::size_t>::max() / 2 || samples.size() != frames * 2) {
        throw FormatError("Sidechain signal has not been prepared for this block");
    }
    return samples;
}
}
