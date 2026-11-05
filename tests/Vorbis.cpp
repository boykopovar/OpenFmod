#include "OpenFmod/Sound/Vorbis/Decoder.hpp"
#include "OpenFmod/Sound/Vorbis/State.hpp"
#include "OpenFmod/Binary/Checksum.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include "OpenFmod/Audio/PcmSource.hpp"
#include <vorbis/vorbisenc.h>
#include <cmath>
#include <future>
#include <stdexcept>
namespace {
void checkSignal(int channels, float quality) {
    using namespace OpenFmod;
    Sound::Vorbis::State encoder;
    if (vorbis_encode_init_vbr(&encoder.Info, channels, 48000, quality)) throw std::runtime_error("Cannot encode test signal");
    if (vorbis_analysis_init(&encoder.Dsp, &encoder.Info) || vorbis_block_init(&encoder.Dsp, &encoder.Block)) {
        throw std::runtime_error("Cannot initialize test encoder");
    }
    ogg_packet identification{}, comment{}, setup{};
    if (vorbis_analysis_headerout(&encoder.Dsp, &encoder.Comment, &identification, &comment, &setup)) {
        throw std::runtime_error("Cannot generate test headers");
    }
    Sound::Sample sample;
    sample.Rate = 48000;
    sample.Channels = static_cast<std::uint32_t>(channels);
    sample.Frames = 4800;
    sample.VorbisSetup = Binary::Crc32({reinterpret_cast<char*>(setup.packet), static_cast<std::size_t>(setup.bytes)});
    auto pcm = vorbis_analysis_buffer(&encoder.Dsp, static_cast<int>(sample.Frames));
    if (!pcm) throw std::runtime_error("Cannot allocate test signal");
    const auto signal = [](std::size_t frame, int channel) {
        return 0.25f * std::sin(static_cast<float>(frame) * (0.057595865f + channel * 0.012f));
    };
    for (int channel = 0; channel < channels; ++channel) {
        for (std::size_t frame = 0; frame < sample.Frames; ++frame) pcm[channel][frame] = signal(frame, channel);
    }
    if (vorbis_analysis_wrote(&encoder.Dsp, static_cast<int>(sample.Frames)) || vorbis_analysis_wrote(&encoder.Dsp, 0)) {
        throw std::runtime_error("Cannot finalize test signal");
    }
    std::string bytes;
    while (vorbis_analysis_blockout(&encoder.Dsp, &encoder.Block) == 1) {
        if (vorbis_analysis(&encoder.Block, nullptr) || vorbis_bitrate_addblock(&encoder.Block)) {
            throw std::runtime_error("Cannot encode test block");
        }
        ogg_packet packet{};
        while (vorbis_bitrate_flushpacket(&encoder.Dsp, &packet)) {
            if (packet.bytes > 65535) throw std::runtime_error("Test packet too large");
            bytes.push_back(static_cast<char>(packet.bytes & 255));
            bytes.push_back(static_cast<char>((packet.bytes >> 8) & 255));
            bytes.append(reinterpret_cast<const char*>(packet.packet), static_cast<std::size_t>(packet.bytes));
        }
    }
    sample.Data = bytes;
    Sound::Vorbis::SetupRegistry registry;
    const auto decoded = Sound::Vorbis::Decode(sample, registry);
    if (decoded.size() != sample.Frames * sample.Channels) throw std::runtime_error("Wrong decoded signal length");
    double error = 0;
    for (std::size_t frame = 0; frame < decoded.size(); ++frame) {
        const auto difference = decoded[frame] - signal(frame / sample.Channels, static_cast<int>(frame % sample.Channels));
        error += difference * difference;
    }
    if (std::sqrt(error / decoded.size()) > 0.01) throw std::runtime_error("Decoded signal does not match input");
    Sound::Vorbis::Decoder stream(sample, registry);
    std::vector<float> streamed(decoded.size());
    std::size_t position = 0;
    while (position < sample.Frames) {
        const auto count = stream.Read(streamed.data() + position * sample.Channels, 127);
        if (!count || count > 127) throw std::runtime_error("Invalid streaming decoder progress");
        position += count;
        if (stream.Position() != position) throw std::runtime_error("Incorrect streaming decoder position");
    }
    float tail = 42;
    if (stream.Read(&tail, 1) || tail != 42 || stream.Read(nullptr, 0)) throw std::runtime_error("Decoder read beyond end");
    if (streamed != decoded) throw std::runtime_error("Streaming changed decoded signal");
    const auto prepared = registry.Prepare(sample.Rate, sample.Channels, *sample.VorbisSetup);
    if (prepared != registry.Prepare(sample.Rate, sample.Channels, *sample.VorbisSetup)) throw std::runtime_error("Vorbis decoder setup was not reused");
    auto parallel = std::async(std::launch::async, [&] { return Sound::Vorbis::Decode(sample, registry); });
    const auto concurrent = Sound::Vorbis::Decode(sample, registry);
    if (parallel.get() != decoded || concurrent != decoded) throw std::runtime_error("Shared Vorbis setup changed concurrent decoding");
    std::unique_ptr<Sound::Vorbis::Decoder> retained;
    {
        Sound::Vorbis::SetupRegistry temporary;
        retained = std::make_unique<Sound::Vorbis::Decoder>(sample, temporary);
    }
    std::vector<float> retainedSamples(decoded.size());
    if (retained->Read(retainedSamples.data(), sample.Frames) != sample.Frames || retainedSamples != decoded) {
        throw std::runtime_error("Decoder did not retain its shared setup");
    }
    auto pcmData = std::make_shared<Audio::PcmData>();
    pcmData->Layout = {sample.Rate, sample.Channels, sample.Frames};
    pcmData->Samples = decoded;
    Audio::PcmSource first(pcmData), second(pcmData);
    pcmData.reset();
    first.Seek(100);
    std::vector<float> fragment(sample.Channels * 12);
    if (first.Read(fragment.data(), 12) != 12
        || !std::equal(fragment.begin(), fragment.end(), decoded.begin() + sample.Channels * 100)) {
        throw std::runtime_error("PCM seek changed sample data");
    }
    if (second.Read(fragment.data(), 12) != 12 || !std::equal(fragment.begin(), fragment.end(), decoded.begin())) {
        throw std::runtime_error("Shared PCM sources did not retain independent cursors");
    }
    sample.Data = std::string_view(bytes).substr(0, 1);
    bool rejected = false;
    try { Sound::Vorbis::Decode(sample, registry); }
    catch (const FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Truncated audio accepted");
}
}
void CheckVorbis() {
    checkSignal(1, 0.4f);
    checkSignal(2, 0.24f);
}
