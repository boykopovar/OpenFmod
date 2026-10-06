#include "OpenFmod/Studio/Playlist.hpp"
#include <stdexcept>
void CheckPlaylist() {
    using namespace OpenFmod::Studio;
    Playlist playlist{2, 1, {{{}, 100}, {{}, 100}, {{}, 100}}};
    PlaylistCursor cursor(playlist, {false, false, false});
    for (std::size_t index = 0; index < 3; ++index) {
        if (cursor.Next(0) != index % 3) throw std::runtime_error("Shuffle repeated an exhausted variant");
    }
    auto previous = std::optional<std::size_t>(2);
    for (std::size_t index = 0; index < 30; ++index) {
        const auto selected = cursor.Next(0);
        if (selected == previous) throw std::runtime_error("Shuffle immediately repeated its last variant");
        previous = selected;
    }
    playlist.PlayMode = 1;
    playlist.Entries[0].Weight = 10;
    playlist.Entries[1].Weight = 20;
    playlist.Entries[2].Weight = 70;
    PlaylistCursor random(playlist, {false, false, false});
    if (random.Next(0.05f) != 0 || random.Next(0.2f) != 1 || random.Next(0.9f) != 2) {
        throw std::runtime_error("Random selection ignored variant weights");
    }
    playlist.SelectionMode = 0;
    PlaylistCursor once(playlist, {false, false, false});
    if (once.Next(0.2f) != 1 || once.Next(0.9f) != 1) throw std::runtime_error("One-time selection changed variant");
    PlaylistCursor empty({2, 1, {}}, {});
    if (empty.Next(0)) throw std::runtime_error("Empty playlist selected a variant");
}
