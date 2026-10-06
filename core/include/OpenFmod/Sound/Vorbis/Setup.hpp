#ifndef OPENFMOD_SOUND_VORBIS_SETUP_HPP
#define OPENFMOD_SOUND_VORBIS_SETUP_HPP
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
namespace OpenFmod::Sound::Vorbis {
struct Headers {
    std::array<std::vector<unsigned char>, 3> Packets;
};
class SetupRegistry {
public:
    std::shared_ptr<const Headers> Resolve(std::uint32_t rate, std::uint32_t channels, std::uint32_t checksum);
private:
    std::mutex mutex;
    std::map<std::array<std::uint32_t, 3>, std::shared_ptr<const Headers>> headers;
    std::map<std::array<std::uint32_t, 2>, int> positions;
};
}
#endif
