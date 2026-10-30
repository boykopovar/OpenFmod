#include "OpenFmod/Studio/Timeline/Timing/TempoGrid.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
TempoGrid::TempoGrid(std::vector<TempoMarker> markers) : markers(std::move(markers)) {
    std::uint32_t previous = 0;
    for (const auto& marker : this->markers) {
        marker.Validate();
        if (marker.Position < previous) throw FormatError("Unordered tempo markers");
        previous = marker.Position;
    }
}
std::uint64_t TempoGrid::Quantize(std::uint64_t position, QuantizationMode mode, std::uint32_t interval) const {
    if (mode > QuantizationMode::EighthNotes) throw FormatError("Invalid quantization mode");
    if (mode == QuantizationMode::None) return position;
    if (!interval) throw FormatError("Zero quantization interval");
    const TempoMarker standard{{}, 4, 4, 0, 120};
    const auto next = std::upper_bound(markers.begin(), markers.end(), position,
        [](std::uint64_t time, const TempoMarker& marker) { return time < marker.Position; });
    const auto& current = markers.empty() ? standard : (next == markers.begin() ? *next : *(next - 1));
    if (current.BeatsPerMinute == 0) return next == markers.end() || position == current.Position ? position : next->Position;
    const auto units = mode == QuantizationMode::Bars
        ? static_cast<std::uint64_t>(interval) * (8ull * current.Numerator / current.Denominator) : interval;
    if (!units) throw FormatError("Quantization interval is below the musical grid resolution");
    const long double step = 1440000.0L * units / current.BeatsPerMinute;
    long double result;
    if (position >= current.Position) {
        const long double distance = position - current.Position;
        const auto index = std::floor(distance / step);
        auto offset = std::floor(index * step + 0.5L);
        if (offset < distance) offset = std::floor((index + 1) * step + 0.5L);
        result = current.Position + offset;
        if (next != markers.end()) result = std::min(result, static_cast<long double>(next->Position));
    } else {
        const long double distance = current.Position - position;
        const auto index = std::floor(distance / step);
        auto offset = std::floor(index * step + 0.5L);
        const auto adjacent = std::floor((index + 1) * step + 0.5L);
        if (adjacent <= distance) offset = adjacent;
        result = current.Position - offset;
    }
    if (result < position || result >= std::ldexp(1.0L, 64)) throw FormatError("Quantized timeline position overflow");
    return static_cast<std::uint64_t>(result);
}
}
