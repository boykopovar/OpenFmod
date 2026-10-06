#include "OpenFmod/Sound/Vorbis/Decoder.hpp"
#include "OpenFmod/Sound/Vorbis/State.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Sound::Vorbis {
class Decoder::Stream {
public:
    Stream(const Sample& sample, SetupRegistry& registry) : reader(sample.Data), channels(sample.Channels), frames(sample.Frames) {
        if (!sample.VorbisSetup) throw FormatError("Missing Vorbis setup checksum");
        const auto headers = registry.Resolve(sample.Rate, channels, *sample.VorbisSetup);
        if (frames > std::numeric_limits<std::size_t>::max() / channels) throw FormatError("Decoded sample is too large");
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
        packet.granulepos = -1;
    }
    std::size_t Read(float* output, std::size_t requested) {
        if (requested && !output) throw FormatError("Missing decoded audio destination");
        const auto count = std::min(requested, frames - position);
        std::size_t written = 0;
        while (written < count) {
            float** pcm = nullptr;
            const auto available = vorbis_synthesis_pcmout(&decoder.Dsp, &pcm);
            if (available < 0) throw FormatError("Vorbis decoder returned an invalid sample count");
            if (!available) {
                decodePacket();
                continue;
            }
            const auto chunk = std::min(static_cast<std::size_t>(available), count - written);
            for (std::size_t frame = 0; frame < chunk; ++frame) {
                for (std::size_t channel = 0; channel < channels; ++channel) {
                    const auto value = pcm[channel][frame];
                    if (!std::isfinite(value)) throw FormatError("Nonfinite Vorbis sample");
                    output[(written + frame) * channels + channel] = value;
                }
            }
            if (vorbis_synthesis_read(&decoder.Dsp, static_cast<int>(chunk))) throw FormatError("Cannot advance Vorbis decoder");
            written += chunk;
            position += chunk;
        }
        return written;
    }
    std::size_t Position() const { return position; }
private:
    void decodePacket() {
        const auto size = static_cast<std::size_t>(reader.Unsigned(2));
        if (!size) throw FormatError("Vorbis stream ended before its declared sample count");
        const auto bytes = reader.Read(size);
        packet.packet = reinterpret_cast<unsigned char*>(const_cast<char*>(bytes.data()));
        packet.bytes = static_cast<long>(size);
        if (vorbis_synthesis(&decoder.Block, &packet) || vorbis_synthesis_blockin(&decoder.Dsp, &decoder.Block)) {
            throw FormatError("Invalid Vorbis audio packet");
        }
        ++packet.packetno;
    }
    State decoder;
    Binary::Reader reader;
    ogg_packet packet{};
    std::size_t channels;
    std::size_t frames;
    std::size_t position{};
};
Decoder::Decoder(const Sample& sample, SetupRegistry& registry) : stream(std::make_unique<Stream>(sample, registry)) {}
Decoder::~Decoder() = default;
std::size_t Decoder::Read(float* output, std::size_t frames) { return stream->Read(output, frames); }
std::size_t Decoder::Position() const { return stream->Position(); }
std::vector<float> Decode(const Sample& sample, SetupRegistry& registry) {
    Decoder decoder(sample, registry);
    std::vector<float> result(static_cast<std::size_t>(sample.Frames) * sample.Channels);
    decoder.Read(result.data(), sample.Frames);
    return result;
}
}
