#include "OpenFmod/Studio/Effect/Processor.hpp"
#include <type_traits>
#include <algorithm>
#include <limits>
namespace OpenFmod::Studio {
EffectProcessor::Unit EffectProcessor::create(const Effect& effect, std::uint32_t sampleRate) {
    switch (effect.Type) {
        case 18: return Audio::Compressor(sampleRate, CompressorConfiguration(effect.Parameters));
        case 19: return Audio::Reverb(sampleRate, ReverbConfiguration(effect.Parameters));
        case 28: return Audio::ThreeBandEq(sampleRate, ThreeBandConfiguration(effect.Parameters));
        case 36: {
            Audio::MultibandEq result(sampleRate);
            for (std::size_t band = 0; band < 5; ++band) result.Configure(band, EqualizerConfiguration(effect.Parameters, band));
            return result;
        }
        default: throw FormatError("Unsupported effect processor: " + std::to_string(effect.Type));
    }
}
EffectProcessor::EffectProcessor(const Effect& effect, std::uint32_t sampleRate)
    : parameters(effect.Parameters), unit(create(effect, sampleRate)) {
    if (effect.Type == 18) sidechainEnabled = CompressorSidechainEnabled(parameters);
}
void EffectProcessor::SetParameter(std::size_t index, EffectParameter value, std::uint32_t rampFrames) {
    if (index >= parameters.size()) throw FormatError("Unknown effect parameter");
    if (value.index() != parameters[index].index()) throw FormatError("Incorrect effect parameter type");
    if (value == parameters[index]) return;
    auto updated = parameters;
    updated[index] = std::move(value);
    std::visit([&](auto& processor) {
        using TProcessor = std::decay_t<decltype(processor)>;
        if constexpr (std::is_same_v<TProcessor, Audio::Compressor>) processor.Configure(CompressorConfiguration(updated));
        else if constexpr (std::is_same_v<TProcessor, Audio::Reverb>) processor.Configure(ReverbConfiguration(updated));
        else if constexpr (std::is_same_v<TProcessor, Audio::ThreeBandEq>) processor.Configure(ThreeBandConfiguration(updated));
        else processor.Configure(index / 4, EqualizerConfiguration(updated, index / 4), rampFrames);
    }, unit);
    if (std::holds_alternative<Audio::Compressor>(unit)) sidechainEnabled = CompressorSidechainEnabled(updated);
    parameters = std::move(updated);
}
void EffectProcessor::SetSidechains(std::vector<std::shared_ptr<Audio::SignalTap>> sources) {
    if (!std::holds_alternative<Audio::Compressor>(unit)) throw FormatError("Effect does not accept a sidechain");
    for (auto current = sources.begin(); current != sources.end(); ++current) {
        if (!*current || std::find(sources.begin(), current, *current) != current) throw FormatError("Invalid sidechain source");
    }
    sidechains = std::move(sources);
}
void EffectProcessor::SetBypassed(bool value) { bypassed = value; }
void EffectProcessor::Process(float* stereo, std::size_t frames) {
    if ((frames && !stereo) || frames > std::numeric_limits<std::size_t>::max() / 2) throw FormatError("Invalid effect output buffer");
    if (!frames || bypassed) return;
    if (sidechainEnabled) {
        if (sidechains.empty()) throw FormatError("Compressor requires a sidechain signal");
        mixedSidechain.assign(frames * 2, 0);
        for (const auto& source : sidechains) {
            const auto& signal = source->GetSamples(frames);
            for (std::size_t index = 0; index < signal.size(); ++index) mixedSidechain[index] += signal[index];
        }
        ProcessWithSidechain(stereo, frames, mixedSidechain.data(), 2);
        return;
    }
    if (!bypassed) std::visit([&](auto& processor) { processor.Process(stereo, frames); }, unit);
}
void EffectProcessor::ProcessWithSidechain(float* stereo, std::size_t frames, const float* sidechain, std::size_t channels) {
    const auto processor = std::get_if<Audio::Compressor>(&unit);
    if (!sidechainEnabled) throw FormatError("Compressor sidechain is disabled");
    if (!processor) throw FormatError("Effect does not accept an external sidechain");
    if (frames && (!stereo || !sidechain)) throw FormatError("Missing sidechain audio buffer");
    if (!channels || channels > 32) throw FormatError("Invalid sidechain channel count");
    if (!bypassed) processor->ProcessWithSidechain(stereo, frames, sidechain, channels);
}
}
