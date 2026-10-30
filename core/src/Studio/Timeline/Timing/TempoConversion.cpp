#include "OpenFmod/Studio/Timeline/Timing/TempoGrid.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
std::size_t TempoGrid::nextMarker(std::uint32_t position) const {
    return static_cast<std::size_t>(std::upper_bound(markers.begin(), markers.end(), position,
        [](std::uint32_t tick, const TempoMarker& marker) { return tick < marker.Position; }) - markers.begin());
}
double TempoGrid::rate(std::size_t next) const {
    if (markers.empty()) return 120.0;
    const auto& marker = markers[next ? next - 1 : 0];
    return static_cast<double>(marker.BeatsPerMinute) * marker.Numerator / marker.Denominator;
}
double TempoGrid::ToBars(std::uint32_t position, std::uint32_t duration) const {
    auto next = nextMarker(position);
    double result = 0;
    while (duration) {
        const auto count = next < markers.size() ? std::min(duration, markers[next].Position - position) : duration;
        result += static_cast<double>(count) * rate(next) / 2880000.0;
        duration -= count;
        if (duration) position = markers[next++].Position;
    }
    return result;
}
std::uint32_t TempoGrid::ToTicks(std::uint32_t position, double bars) const {
    if (!std::isfinite(bars) || bars < 0) throw FormatError("Invalid musical duration");
    auto next = nextMarker(position);
    std::uint32_t result = 0;
    const auto limit = std::numeric_limits<std::uint32_t>::max();
    while (bars > 0) {
        const auto tempo = rate(next);
        if (tempo <= 0) throw FormatError("Cannot convert musical duration at zero tempo");
        const auto ticksPerBar = 2880000.0 / tempo;
        const auto duration = ticksPerBar * bars;
        if (!std::isfinite(duration) || duration >= static_cast<double>(limit) + 1) throw FormatError("Musical duration overflow");
        const auto ticks = static_cast<std::uint32_t>(duration);
        const auto count = next < markers.size() ? std::min(ticks, markers[next].Position - position) : ticks;
        if (count > limit - result) throw FormatError("Musical duration overflow");
        result += count;
        bars = static_cast<double>(ticks - count) / ticksPerBar;
        if (bars > 0) position = markers[next++].Position;
    }
    return result;
}
}
