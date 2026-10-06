#include "OpenFmod/Sound/Vorbis/Decoder.hpp"
#include "OpenFmod/Sound/Vorbis/State.hpp"
#include "OpenFmod/Binary/Checksum.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <vorbis/vorbisenc.h>
#include <cmath>
#include <stdexcept>
void CheckVorbis() {
    using namespace OpenFmod;
    Sound::Vorbis::State encoder;
    if (vorbis_encode_init_vbr(&encoder.Info, 1, 48000, 0.4f)) throw std::runtime_error("Cannot encode test signal");
    if (vorbis_analysis_init(&encoder.Dsp, &encoder.Info) || vorbis_block_init(&encoder.Dsp, &encoder.Block)) {
        throw std::runtime_error("Cannot initialize test encoder");
    }
    ogg_packet identification{}, comment{}, setup{};
    if (vorbis_analysis_headerout(&encoder.Dsp, &encoder.Comment, &identification, &comment, &setup)) {
        throw std::runtime_error("Cannot generate test headers");
    }
    Sound::Sample sample;
    sample.Rate = 48000;
    sample.Channels = 1;
    sample.Frames = 4800;
    sample.VorbisSetup = Binary::Crc32({reinterpret_cast<char*>(setup.packet), static_cast<std::size_t>(setup.bytes)});
    auto pcm = vorbis_analysis_buffer(&encoder.Dsp, static_cast<int>(sample.Frames));
    if (!pcm) throw std::runtime_error("Cannot allocate test signal");
    const auto signal = [](std::size_t frame) { return 0.25f * std::sin(static_cast<float>(frame) * 0.057595865f); };
    for (std::size_t frame = 0; frame < sample.Frames; ++frame) pcm[0][frame] = signal(frame);
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
    if (decoded.size() != sample.Frames) throw std::runtime_error("Wrong decoded signal length");
    double error = 0;
    for (std::size_t frame = 0; frame < decoded.size(); ++frame) {
        const auto difference = decoded[frame] - signal(frame);
        error += difference * difference;
    }
    if (std::sqrt(error / decoded.size()) > 0.01) throw std::runtime_error("Decoded signal does not match input");
    sample.Data = std::string_view(bytes).substr(0, 1);
    bool rejected = false;
    try { Sound::Vorbis::Decode(sample, registry); }
    catch (const FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Truncated audio accepted");
}
