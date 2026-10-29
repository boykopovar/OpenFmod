#include "OpenFmod/Studio/Levels.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
namespace OpenFmod::Studio {
float VolumeGain(float decibels) {
    if (!std::isfinite(decibels)) throw FormatError("Nonfinite volume");
    const auto gain = decibels <= -80 ? 0 : std::pow(10.0f, decibels * 0.05f);
    if (!std::isfinite(gain)) throw FormatError("Volume gain overflow");
    return gain;
}
float PitchRatio(float semitones) {
    if (!std::isfinite(semitones)) throw FormatError("Nonfinite pitch");
    const auto ratio = std::exp2(semitones / 12);
    if (!std::isfinite(ratio) || ratio <= 0) throw FormatError("Pitch ratio overflow");
    return ratio;
}
}
