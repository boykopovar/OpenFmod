#include "OpenFmod/Sound/Vorbis/DecoderSetup.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <limits>
namespace OpenFmod::Sound::Vorbis {
DecoderSetup::DecoderSetup(const Headers& headers) {
    ogg_packet packet{};
    packet.b_o_s = 1;
    for (const auto& bytes : headers.Packets) {
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<long>::max())) throw FormatError("Vorbis header is too large");
        packet.packet = const_cast<unsigned char*>(bytes.data());
        packet.bytes = static_cast<long>(bytes.size());
        if (vorbis_synthesis_headerin(&state.Info, &state.Comment, &packet)) throw FormatError("Invalid Vorbis header");
        packet.b_o_s = 0;
        ++packet.packetno;
    }
    Initialize(state);
    vorbis_block_clear(&state.Block);
    vorbis_dsp_clear(&state.Dsp);
}
void DecoderSetup::Initialize(State& decoder) const {
    const std::lock_guard lock(mutex);
    if (vorbis_synthesis_init(&decoder.Dsp, &state.Info) || vorbis_block_init(&decoder.Dsp, &decoder.Block)) {
        throw FormatError("Cannot initialize Vorbis decoder");
    }
}
}
