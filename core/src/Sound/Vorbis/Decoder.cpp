#include "OpenFmod/Sound/Vorbis/Decoder.hpp"
#include "OpenFmod/Sound/Vorbis/State.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <vorbis/codec.h>
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Sound::Vorbis {
std::vector<float> Decode(const Sample& sample, SetupRegistry& registry) {
    if (!sample.VorbisSetup) throw FormatError("Missing Vorbis setup checksum");
    const auto headers = registry.Resolve(sample.Rate, sample.Channels, *sample.VorbisSetup);
    State decoder;
    ogg_packet packet{};
    packet.b_o_s = 1;
    for (const auto& bytes : headers->Packets) {
        packet.packet = const_cast<unsigned char*>(bytes.data());
        packet.bytes = static_cast<long>(bytes.size());
        if (vorbis_synthesis_headerin(&decoder.Info, &decoder.Comment, &packet)) throw FormatError("Invalid Vorbis header");
        packet.b_o_s = 0;
        ++packet.packetno;
    }
    if (vorbis_synthesis_init(&decoder.Dsp, &decoder.Info) || vorbis_block_init(&decoder.Dsp, &decoder.Block)) {
        throw FormatError("Cannot initialize Vorbis decoder");
    }
    if (sample.Frames > std::numeric_limits<std::size_t>::max() / sample.Channels) throw FormatError("Decoded sample is too large");
    std::vector<float> result(static_cast<std::size_t>(sample.Frames) * sample.Channels);
    Binary::Reader reader(sample.Data);
    std::size_t written = 0;
    packet.granulepos = -1;
    while (written < sample.Frames) {
        const auto size = static_cast<std::size_t>(reader.Unsigned(2));
        if (!size) throw FormatError("Vorbis stream ended before its declared sample count");
        const auto bytes = reader.Read(size);
        packet.packet = reinterpret_cast<unsigned char*>(const_cast<char*>(bytes.data()));
        packet.bytes = static_cast<long>(size);
        if (vorbis_synthesis(&decoder.Block, &packet) || vorbis_synthesis_blockin(&decoder.Dsp, &decoder.Block)) {
            throw FormatError("Invalid Vorbis audio packet");
        }
        ++packet.packetno;
        float** pcm = nullptr;
        const auto available = vorbis_synthesis_pcmout(&decoder.Dsp, &pcm);
        if (available < 0) throw FormatError("Vorbis decoder returned an invalid sample count");
        const auto count = std::min(static_cast<std::size_t>(available), static_cast<std::size_t>(sample.Frames) - written);
        for (std::size_t frame = 0; frame < count; ++frame) {
            for (std::size_t channel = 0; channel < sample.Channels; ++channel) {
                const auto value = pcm[channel][frame];
                if (!std::isfinite(value)) throw FormatError("Nonfinite Vorbis sample");
                result[(written + frame) * sample.Channels + channel] = value;
            }
        }
        written += count;
        if (vorbis_synthesis_read(&decoder.Dsp, static_cast<int>(count))) throw FormatError("Cannot advance Vorbis decoder");
    }
    return result;
}
}
