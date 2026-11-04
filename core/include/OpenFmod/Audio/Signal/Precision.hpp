#ifndef OPENFMOD_AUDIO_SIGNAL_PRECISION_HPP
#define OPENFMOD_AUDIO_SIGNAL_PRECISION_HPP
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
inline float FlushSubnormal(float value) {
    return std::abs(value) < std::numeric_limits<float>::min() ? 0.0f : value;
}
}
#endif
