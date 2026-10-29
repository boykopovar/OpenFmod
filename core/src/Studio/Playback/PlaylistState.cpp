#include "OpenFmod/Studio/Playback/PlaylistState.hpp"
namespace OpenFmod::Studio {
PlaylistState::PlaylistState(const Library& library) : library(library) {}
PlaylistState::PlaylistState(const Library& library, PlaylistState& global) : library(library), global(&global) {
    if (global.global || &library != &global.library) throw FormatError("Invalid global playlist state");
}
std::shared_ptr<PlaylistCursor> PlaylistState::Acquire(const Bank::Guid& instrument) {
    const auto& model = library.Require<Instrument>(instrument);
    if (!std::holds_alternative<MultiInstrument>(model.Definition.Source) || !model.Variants) throw FormatError("Playlist state requires a multi instrument");
    const auto mode = model.Variants->PlayMode;
    if (global && mode != 0 && mode != 4) return global->Acquire(instrument);
    const auto found = cursors.find(instrument);
    if (found != cursors.end()) return found->second;
    std::vector<bool> silence;
    silence.reserve(model.Variants->Entries.size());
    for (const auto& entry : model.Variants->Entries) silence.push_back(std::holds_alternative<SilenceInstrument>(library.Require<Instrument>(entry.Id).Definition.Source));
    auto cursor = std::make_shared<PlaylistCursor>(*model.Variants, std::move(silence));
    cursors.emplace(instrument, cursor);
    return cursor;
}
}
