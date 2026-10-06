#ifndef OPENFMOD_STUDIO_PLAYLIST_HPP
#define OPENFMOD_STUDIO_PLAYLIST_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include <optional>
namespace OpenFmod::Studio {
struct PlaylistEntry {
    Bank::Guid Id;
    float Weight;
};
struct Playlist {
    std::uint32_t PlayMode;
    std::uint32_t SelectionMode;
    std::vector<PlaylistEntry> Entries;
};
Playlist ReadPlaylist(std::string_view bytes);
class PlaylistCursor {
public:
    PlaylistCursor(Playlist playlist, std::vector<bool> silence);
    std::optional<std::size_t> Next(float random);
private:
    struct Weight {
        float Available;
        float Deferred;
    };
    float prepareWeights();
    std::size_t choose(float random);
    Playlist playlist;
    std::vector<bool> silence;
    std::vector<Weight> weights;
    std::optional<std::size_t> previous;
    std::optional<std::size_t> previousSound;
    std::size_t soundCount{};
    float minimum{};
    bool equalWeights = true;
};
}
#endif
