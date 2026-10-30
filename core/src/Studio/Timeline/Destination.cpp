#include "OpenFmod/Studio/Timeline/Bounds.hpp"
#include "OpenFmod/Studio/Timeline/TempoGrid.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
std::uint32_t TransitionDestination(const Timeline& timeline, const Transition& transition, std::uint32_t position) {
    const auto& destination = RequireMarker(timeline, transition.Destination);
    if ((transition.Flags & 4) == 0) return destination.Position;
    auto origin = transition.Start;
    if ((transition.Flags & 16) != 0) {
        const auto found = std::find_if(timeline.Markers.begin(), timeline.Markers.end(), [&](const auto& marker) {
            return marker.Position <= position && position < TimelineEnd(marker.Position, marker.Duration);
        });
        if (found == timeline.Markers.end()) return destination.Position;
        origin = found->Position;
    }
    if (position < origin) throw FormatError("Relative transition precedes its source region");
    auto offset = position - origin;
    const bool reverse = (transition.Flags & 8) != 0;
    if (timeline.Tempo.empty()) {
        if (destination.Duration) {
            const auto remainder = offset % destination.Duration;
            if (remainder) offset = reverse ? destination.Duration - remainder : remainder;
            else offset = (offset != 0) != reverse ? destination.Duration - 1 : 0;
        }
    } else {
        TempoGrid grid(timeline.Tempo);
        auto bars = grid.ToBars(origin, offset);
        if (destination.Duration) {
            const auto length = grid.ToBars(destination.Position, destination.Duration);
            if (length <= 0) throw FormatError("Relative transition destination has zero musical duration");
            const auto loops = std::floor(bars / length);
            bars -= loops * length;
            constexpr double epsilon = 1.1920928955078125e-7;
            if (bars < epsilon && loops > epsilon) bars = length;
            if (reverse) bars = length - bars;
        }
        offset = grid.ToTicks(destination.Position, bars);
        if (offset && offset == destination.Duration) --offset;
    }
    return TimelineEnd(destination.Position, offset);
}
}
