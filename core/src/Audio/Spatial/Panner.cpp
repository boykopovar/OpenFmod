#include "OpenFmod/Audio/Spatial/Panner.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <limits>
namespace OpenFmod::Audio {
Panner::Panner(const PannerSettings& settings) : matrix(PannerMatrix(settings)), target(matrix) {}
void Panner::Configure(const PannerSettings& settings, std::uint32_t rampFrames) {
    target = PannerMatrix(settings);
    remaining = rampFrames;
    if (!remaining) matrix = target;
    else for (std::size_t index = 0; index < 4; ++index) increment[index] = (target[index] - matrix[index]) / remaining;
}
void Panner::Process(float* stereo, std::size_t frames) {
    if ((frames && !stereo) || frames > std::numeric_limits<std::size_t>::max() / 2) throw FormatError("Invalid panner buffer");
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const auto left = stereo[frame * 2];
        const auto right = stereo[frame * 2 + 1];
        stereo[frame * 2] = left * matrix[0] + right * matrix[2];
        stereo[frame * 2 + 1] = left * matrix[1] + right * matrix[3];
        if (remaining) {
            if (--remaining == 0) matrix = target;
            else for (std::size_t index = 0; index < 4; ++index) matrix[index] += increment[index];
        }
    }
}
}
