#include "OpenFmod/Studio/Modulation/Runtime/FollowerState.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
FollowerState::FollowerState(EnvelopeFollower parameters) : parameters(parameters) {
    if (!std::isfinite(parameters.MinimumDecibels) || !std::isfinite(parameters.MaximumDecibels)
        || parameters.MinimumDecibels >= parameters.MaximumDecibels) throw FormatError("Invalid follower decibel range");
    if (!std::isfinite(parameters.AttackMilliseconds) || parameters.AttackMilliseconds < 0
        || !std::isfinite(parameters.ReleaseMilliseconds) || parameters.ReleaseMilliseconds < 0) {
        throw FormatError("Invalid follower duration");
    }
    if (!std::isfinite(parameters.Amount)) throw FormatError("Nonfinite follower amount");
}
float FollowerState::Process(const float* samples, std::size_t count, double milliseconds) {
    if ((!samples && count) || !count) throw FormatError("Empty follower input");
    if (!std::isfinite(milliseconds) || milliseconds < 0) throw FormatError("Invalid follower elapsed time");
    double amplitude = 0;
    for (std::size_t index = 0; index < count; ++index) {
        const double value = samples[index];
        if (!std::isfinite(value)) throw FormatError("Nonfinite follower input");
        if (parameters.RootMeanSquare) amplitude += value * value / count;
        else amplitude = std::max(amplitude, std::abs(value));
    }
    if (parameters.RootMeanSquare) amplitude = std::sqrt(amplitude);
    return ProcessLevel(amplitude, milliseconds);
}
float FollowerState::ProcessLevel(double amplitude, double milliseconds) {
    if (!std::isfinite(amplitude) || amplitude < 0 || !std::isfinite(milliseconds) || milliseconds < 0) throw FormatError("Invalid follower meter input");
    const auto decibels = amplitude > 0 ? 20 * std::log10(amplitude) : -80;
    const auto target = static_cast<float>((std::clamp(decibels, static_cast<double>(parameters.MinimumDecibels),
        static_cast<double>(parameters.MaximumDecibels)) - parameters.MinimumDecibels)
        / (parameters.MaximumDecibels - parameters.MinimumDecibels));
    if (milliseconds == 0) return level;
    if (target > level) {
        level = parameters.AttackMilliseconds == 0 ? target
            : static_cast<float>(std::min<double>(target, level + target * milliseconds / parameters.AttackMilliseconds));
        releaseStart = level;
    } else if (target < level) {
        releaseStart = std::max(releaseStart, level);
        level = parameters.ReleaseMilliseconds == 0 ? target
            : static_cast<float>(std::max<double>(target, level - (releaseStart - target) * milliseconds / parameters.ReleaseMilliseconds));
    }
    return level;
}
float FollowerState::GetLevel() const { return level; }
}
