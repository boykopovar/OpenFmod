#include "OpenFmod/Studio/Effect/Processor.hpp"
#include "OpenFmod/Audio/Spatial/MixMatrix.hpp"
#include <algorithm>
#include <limits>
#include <type_traits>
namespace OpenFmod::Studio {
Audio::SpeakerMode EffectProcessor::GetOutputLayout(Audio::SpeakerMode input) const {
    static_cast<void>(Audio::Speakers(input));
    return !bypassed && std::holds_alternative<Audio::Panner>(unit) ? Audio::SpeakerMode::Stereo : input;
}
void EffectProcessor::Process(const float* input, float* output, std::size_t frames, Audio::SpeakerMode inputLayout) {
    const auto channels = Audio::Speakers(inputLayout).GetChannels();
    if (frames > std::numeric_limits<std::size_t>::max() / 8 || (frames && (!input || !output))) throw FormatError("Invalid effect channel buffers");
    if (!frames) return;
    if (bypassed) {
        std::copy_n(input, frames * channels, output);
        return;
    }
    if (std::holds_alternative<Audio::Panner>(unit)) {
        if (inputLayout != Audio::SpeakerMode::Stereo && PannerConfiguration(parameters).Mode != 1) {
            throw FormatError("Spatial panning requires a stereo input layout");
        }
        std::fill_n(output, frames * 2, 0);
        Audio::MixMatrix(inputLayout, Audio::SpeakerMode::Stereo).Add(input, output, frames);
        Process(output, frames);
        return;
    }
    std::copy_n(input, frames * channels, output);
    if (inputLayout == Audio::SpeakerMode::Stereo) {
        Process(output, frames);
        return;
    }
    std::visit([&](auto& processor) {
        using TProcessor = std::decay_t<decltype(processor)>;
        if constexpr (std::is_same_v<TProcessor, Audio::ThreeBandEq> || std::is_same_v<TProcessor, Audio::MultibandEq> || std::is_same_v<TProcessor, Audio::Reverb>) {
            processor.Process(output, frames, channels);
        } else throw FormatError("Effect requires a stereo input layout");
    }, unit);
}
}

