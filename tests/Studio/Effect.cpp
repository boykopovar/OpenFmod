#include "OpenFmod/Studio/Effect/Processor.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
void CheckEffect() {
    using namespace TestData;
    OpenFmod::Studio::Effect compressor;
    compressor.Type = 18;
    compressor.Parameters = {0.0f, 2.5f, 20.0f, 200.0f, 0.0f, std::string{}, true};
    OpenFmod::Studio::EffectProcessor processor(compressor, 48000);
    processor.SetParameter(4, 6.0f);
    std::array<float, 2> signal{0.01f, 0.01f};
    processor.Process(signal.data(), 1);
    if (signal[0] < 0.019f || signal[0] > 0.021f) throw std::runtime_error("Effect parameter was not applied");
    bool rejected = false;
    try { processor.SetParameter(4, 100.0f); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Invalid effect update accepted");
    processor.SetBypassed(true);
    signal = {0.01f, 0.01f};
    processor.Process(signal.data(), 1);
    if (signal[0] != 0.01f || signal[1] != 0.01f) throw std::runtime_error("Bypassed effect changed signal");
    processor.SetBypassed(false);
    processor.SetParameter(5, std::string("\1\0\0\0", 4));
    rejected = false;
    try { processor.Process(signal.data(), 1); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Sidechain compressor silently used its input");
    const std::array<float, 2> sidechain{};
    processor.ProcessWithSidechain(signal.data(), 1, sidechain.data(), 2);
    auto tap = std::make_shared<OpenFmod::Audio::SignalTap>();
    tap->SetGain(0.5f);
    signal = {0.1f, 0.2f};
    tap->Process(signal.data(), 1);
    if (signal[0] != 0.1f || signal[1] != 0.2f || tap->GetSamples(1)[1] != 0.1f) {
        throw std::runtime_error("Sidechain tap changed the audible signal");
    }
    processor.SetSidechains({tap});
    processor.Process(signal.data(), 1);
    rejected = false;
    try { tap->GetSamples(2); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Mismatched sidechain block accepted");
    OpenFmod::Studio::Effect panner;
    panner.Type = 27;
    panner.Parameters = {std::int32_t{1}, 0.0f, 0.0f, 360.0f, 0.0f, 0.0f, std::int32_t{1}, 60.0f, 0.0f, std::int32_t{4095}, std::string{}, std::int32_t{0}, 1.0f, 20.0f, std::int32_t{0}, 0.0f, 0.0f, 0.0f, std::int32_t{0}, std::string{}, std::int32_t{2}, 0.0f, std::string{}, false};
    OpenFmod::Studio::EffectProcessor cachedPanner(panner, 48000);
    const std::array<float, 8> surround{0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
    for (const auto layout : {OpenFmod::Audio::SpeakerMode::Surround51, OpenFmod::Audio::SpeakerMode::Surround51, OpenFmod::Audio::SpeakerMode::Mono, OpenFmod::Audio::SpeakerMode::Surround71, OpenFmod::Audio::SpeakerMode::Stereo}) {
        OpenFmod::Studio::EffectProcessor freshPanner(panner, 48000);
        std::array<float, 2> expected{};
        std::array<float, 2> actual{};
        freshPanner.Process(surround.data(), expected.data(), 1, layout);
        cachedPanner.Process(surround.data(), actual.data(), 1, layout);
        if (actual != expected) throw std::runtime_error("Cached panner changed channel conversion");
    }
    std::string bytes(16, '\0');
    Append(bytes, 18, 4);
    Append(bytes, 0, 1);
    const auto offset = bytes.size();
    Append(bytes, 4, 4);
    Append(bytes, 0, 4);
    AppendFloat(bytes, -12);
    Append(bytes, 1, 4);
    Append(bytes, 0xfffffffe, 4);
    Append(bytes, 2, 4);
    Append(bytes, 1, 1);
    Append(bytes, 3, 4);
    Append(bytes, 3, 2);
    bytes.append("abc", 3);
    Append(bytes, 1, 1);
    OpenFmod::Bank::Chunk chunk{"LIST", "BEFF", 0, bytes.size(), {
        {"BEFB", {}, 0, offset, {}}, {"PMEF", {}, offset, bytes.size() - offset, {}}, {"LIST", "PRPS", 0, 0, {}}}};
    const auto effect = OpenFmod::Studio::ReadEffect(bytes, chunk);
    if (effect.Type != 18 || std::get<float>(effect.Parameters[0]) != -12
        || std::get<std::int32_t>(effect.Parameters[1]) != -2 || !std::get<bool>(effect.Parameters[2])
        || std::get<std::string>(effect.Parameters[3]) != "abc" || !effect.ParameterFlag) {
        throw std::runtime_error("Incorrect effect parameter decoding");
    }
    bytes.back() = 2;
    try { OpenFmod::Studio::ReadEffect(bytes, chunk); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Invalid effect flag accepted");
}
