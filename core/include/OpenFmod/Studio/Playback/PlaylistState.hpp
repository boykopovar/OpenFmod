#ifndef OPENFMOD_STUDIO_PLAYBACK_PLAYLISTSTATE_HPP
#define OPENFMOD_STUDIO_PLAYBACK_PLAYLISTSTATE_HPP
#include "OpenFmod/Studio/Project/Library.hpp"
namespace OpenFmod::Studio {
class PlaylistState {
public:
    explicit PlaylistState(const Library& library);
    PlaylistState(const Library& library, PlaylistState& global);
    std::shared_ptr<PlaylistCursor> Acquire(const Bank::Guid& instrument);
private:
    const Library& library;
    PlaylistState* global{};
    std::map<Bank::Guid, std::shared_ptr<PlaylistCursor>> cursors;
};
}
#endif
