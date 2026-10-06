#include "OpenFmod/Sound/Vorbis/Setup.hpp"
#include "OpenFmod/Sound/Vorbis/State.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include "OpenFmod/Binary/Checksum.hpp"
#include <vorbis/vorbisenc.h>
#include <limits>
namespace OpenFmod::Sound::Vorbis {
namespace {
std::shared_ptr<Headers> generate(std::uint32_t rate, std::uint32_t channels, int quality, int coupling) {
    State encoder;
    if (vorbis_encode_setup_vbr(&encoder.Info, static_cast<long>(channels), static_cast<long>(rate), quality / 100.0f)) {
        throw FormatError("Cannot configure Vorbis encoder");
    }
    if (vorbis_encode_ctl(&encoder.Info, OV_ECTL_COUPLING_SET, &coupling) || vorbis_encode_setup_init(&encoder.Info)) {
        throw FormatError("Cannot initialize Vorbis setup");
    }
    if (vorbis_analysis_init(&encoder.Dsp, &encoder.Info)) throw FormatError("Cannot initialize Vorbis headers");
    std::array<ogg_packet, 3> packets{};
    if (vorbis_analysis_headerout(&encoder.Dsp, &encoder.Comment, &packets[0], &packets[1], &packets[2])) {
        throw FormatError("Cannot generate Vorbis headers");
    }
    auto result = std::make_shared<Headers>();
    for (std::size_t index = 0; index < packets.size(); ++index) {
        result->Packets[index].assign(packets[index].packet, packets[index].packet + packets[index].bytes);
    }
    return result;
}
}
std::shared_ptr<const Headers> SetupRegistry::Resolve(std::uint32_t rate, std::uint32_t channels, std::uint32_t expected) {
    if (!rate || rate > static_cast<std::uint32_t>(std::numeric_limits<long>::max()) || !channels || channels > 255) {
        throw FormatError("Invalid Vorbis sample rate or channel count");
    }
    const std::array<std::uint32_t, 3> key{rate, channels, expected};
    std::lock_guard<std::mutex> lock(mutex);
    const auto found = headers.find(key);
    if (found != headers.end()) return found->second;
    auto& position = positions[{rate, channels}];
    const int modes = channels > 1 ? 2 : 1;
    while (position < 111 * modes) {
        auto candidate = generate(rate, channels, position / modes - 10, position % modes);
        const auto& packet = candidate->Packets[2];
        const auto actual = Binary::Crc32({reinterpret_cast<const char*>(packet.data()), packet.size()});
        const auto inserted = headers.emplace(std::array<std::uint32_t, 3>{rate, channels, actual}, std::move(candidate));
        ++position;
        if (actual == expected) return inserted.first->second;
    }
    throw FormatError("Unknown Vorbis setup checksum: " + std::to_string(expected));
}
}
