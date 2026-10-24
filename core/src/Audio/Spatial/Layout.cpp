#include "OpenFmod/Audio/Spatial/Layout.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace OpenFmod::Audio {
PlanarPanner::PlanarPanner(PlanarLayout layout) : layout(std::move(layout)) {
    if (!this->layout.Channels || this->layout.Channels > 32 || this->layout.Speakers.empty()) throw FormatError("Invalid planar layout");
    std::set<std::size_t> channels;
    if (this->layout.LowFrequency) {
        if (*this->layout.LowFrequency >= this->layout.Channels) throw FormatError("Invalid low frequency channel");
        channels.insert(*this->layout.LowFrequency);
    }
    for (const auto& speaker : this->layout.Speakers) {
        if (speaker.Channel >= this->layout.Channels || !channels.insert(speaker.Channel).second
            || !std::isfinite(speaker.Azimuth) || speaker.Azimuth < -180 || speaker.Azimuth >= 180) {
            throw FormatError("Invalid planar speaker");
        }
    }
    std::sort(this->layout.Speakers.begin(), this->layout.Speakers.end(),
        [](const auto& first, const auto& second) { return first.Azimuth < second.Azimuth; });
    if (this->layout.Speakers.size() > 1) {
        for (std::size_t index = 0; index < this->layout.Speakers.size(); ++index) {
            const auto next = (index + 1) % this->layout.Speakers.size();
            const auto span = this->layout.Speakers[next].Azimuth + (next ? 0 : 360) - this->layout.Speakers[index].Azimuth;
            if (span <= 0 || span >= 180) throw FormatError("Planar speaker gap requires a non-pair panning sector");
        }
    }
}
void PlanarPanner::addPair(std::vector<float>& output, std::size_t index, float direction, float weight) const {
    const auto next = (index + 1) % layout.Speakers.size();
    const auto first = layout.Speakers[index].Azimuth;
    const auto last = layout.Speakers[next].Azimuth + (next ? 0 : 360);
    const auto left = std::sin((last - direction) * 0.0174532925f);
    const auto right = std::sin((direction - first) * 0.0174532925f);
    const auto scale = weight / std::sqrt(left * left + right * right);
    output[layout.Speakers[index].Channel] += left * scale;
    output[layout.Speakers[next].Channel] += right * scale;
}
void PlanarPanner::normalize(std::vector<float>& output) const {
    float power = 0;
    for (const auto value : output) power += value * value;
    if (!(power > 0) || !std::isfinite(power)) throw FormatError("Planar projection has no finite power");
    const auto gain = 1 / std::sqrt(power);
    for (auto& value : output) value *= gain;
}
std::vector<float> PlanarPanner::Project(float directionDegrees, float extentDegrees) const {
    if (!std::isfinite(directionDegrees) || !std::isfinite(extentDegrees) || extentDegrees < 0 || extentDegrees > 360) {
        throw FormatError("Invalid planar projection");
    }
    std::vector<float> result(layout.Channels);
    if (layout.Speakers.size() == 1 || extentDegrees == 360) {
        const auto value = 1 / std::sqrt(static_cast<float>(layout.Speakers.size()));
        for (const auto& speaker : layout.Speakers) result[speaker.Channel] = value;
        return result;
    }
    const auto direction = std::remainder(directionDegrees, 360.0f);
    const auto begin = direction - extentDegrees * 0.5f;
    const auto end = direction + extentDegrees * 0.5f;
    for (std::size_t index = 0; index < layout.Speakers.size(); ++index) {
        const auto next = (index + 1) % layout.Speakers.size();
        const auto first = layout.Speakers[index].Azimuth;
        const auto last = layout.Speakers[next].Azimuth + (next ? 0 : 360);
        for (int turn = -1; turn <= 1; ++turn) {
            const auto shift = turn * 360.0f;
            if (extentDegrees == 0) {
                if (direction >= first + shift && direction < last + shift) {
                    addPair(result, index, direction - shift, 1);
                    return result;
                }
            } else {
                const auto from = std::max(begin, first + shift);
                const auto to = std::min(end, last + shift);
                if (to > from) addPair(result, index, (from + to) * 0.5f - shift, (to - from) / (last - first));
            }
        }
    }
    normalize(result);
    return result;
}
std::vector<float> PlanarPanner::StereoDownmix() const {
    auto result = Project(-90, 180);
    const auto right = Project(90, 180);
    result.insert(result.end(), right.begin(), right.end());
    if (layout.LowFrequency) {
        result[*layout.LowFrequency] = 0.70710677f;
        result[layout.Channels + *layout.LowFrequency] = 0.70710677f;
    }
    return result;
}
}
