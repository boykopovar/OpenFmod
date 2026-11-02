#include "OpenFmod/Audio/Spatial/Speakers.hpp"
#include "OpenFmod/Audio/Spatial/Projection.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
#include <algorithm>
namespace OpenFmod::Audio {
std::size_t SpeakerChannels(SpeakerMode mode) {
    switch (mode) {
        case SpeakerMode::Mono: return 1;
        case SpeakerMode::Stereo: return 2;
        case SpeakerMode::Quad: return 4;
        case SpeakerMode::Surround: return 5;
        case SpeakerMode::Surround51: return 6;
        case SpeakerMode::Surround71: return 8;
        default: throw FormatError("Unsupported speaker layout");
    }
}
Speakers::Speakers(SpeakerMode mode) : mode(mode), channels(SpeakerChannels(mode)) {
    switch (mode) {
        case SpeakerMode::Mono: break;
        case SpeakerMode::Stereo: break;
        case SpeakerMode::Quad:
            planar.emplace(PlanarLayout{4, {{0, -45}, {1, 45}, {2, -135}, {3, 135}}, {}});
            break;
        case SpeakerMode::Surround:
            planar.emplace(PlanarLayout{5, {{0, -30}, {1, 30}, {2, 0}, {3, -110}, {4, 110}}, {}});
            break;
        case SpeakerMode::Surround51:
            planar.emplace(PlanarLayout{6, {{0, -30}, {1, 30}, {2, 0}, {4, -110}, {5, 110}}, 3});
            break;
        case SpeakerMode::Surround71:
            planar.emplace(PlanarLayout{8, {{0, -30}, {1, 30}, {2, 0}, {4, -90}, {5, 90}, {6, -150}, {7, 150}}, 3});
            break;
        default: throw FormatError("Unsupported speaker layout");
    }
}
std::size_t Speakers::GetChannels() const { return channels; }
std::vector<float> Speakers::Project(float direction, float extent, bool normalized) const {
    if (!std::isfinite(direction) || !std::isfinite(extent) || extent < 0 || extent > 360) throw FormatError("Invalid speaker projection");
    if (planar) return planar->Project(direction, extent, normalized);
    if (mode == SpeakerMode::Mono) return {1};
    const auto gains = StereoProjection(direction, extent);
    return {gains[0], gains[1]};
}
std::vector<float> Speakers::StereoDownmix() const {
    if (planar) return planar->StereoDownmix();
    if (mode == SpeakerMode::Mono) return {0.70710677f, 0.70710677f};
    return {1, 0, 0, 1};
}
std::vector<float> Speakers::projectArc(float begin, float end, float direction, float scale) const {
    std::vector<float> result(channels);
    for (int turn = -1; turn <= 1; ++turn) {
        const auto shift = turn * 360.0f;
        const auto from = std::max(begin, -180.0f + shift);
        const auto to = std::min(end, 180.0f + shift);
        if (to <= from) continue;
        const auto gains = Project(direction + ((from + to) * 0.5f - shift) * scale, (to - from) * scale, false);
        for (std::size_t index = 0; index < channels; ++index) result[index] += gains[index] * ((to - from) / 180.0f);
    }
    float power = 0;
    for (const auto gain : result) power += gain * gain;
    if (!(power > 0)) throw FormatError("Empty stereo source projection");
    const auto gain = 1 / std::sqrt(power);
    for (auto& value : result) value *= gain;
    return result;
}
std::vector<float> Speakers::ProjectStereo(float direction, float extent, float rotation) const {
    if (!std::isfinite(direction) || !std::isfinite(rotation) || !std::isfinite(extent) || extent < 0 || extent > 360) {
        throw FormatError("Invalid stereo source projection");
    }
    if (mode == SpeakerMode::Mono) return {0.70710677f, 0.70710677f};
    const auto angle = std::remainder(rotation - direction, 360.0f);
    auto result = projectArc(angle - 180, angle, direction, extent / 360);
    const auto right = projectArc(angle, angle + 180, direction, extent / 360);
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}
