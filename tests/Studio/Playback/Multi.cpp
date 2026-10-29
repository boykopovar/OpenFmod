#include "OpenFmod/Studio/Playback/Multi.hpp"
#include "OpenFmod/Studio/Playback/Sound.hpp"
#include "OpenFmod/Studio/Playback/PlaylistState.hpp"
#include "../../Data/Writer.hpp"
#include <stdexcept>
namespace {
std::string instrument(unsigned char id, bool multi, unsigned mode, bool empty) {
    using namespace TestData;
    std::string settings(80, '\0');
    Append(settings, 1, 4);
    Append(settings, 24, 2);
    settings.append(24, '\0');
    settings.append(20, '\0');
    std::string identity(16, '\0');
    identity[0] = static_cast<char>(id);
    if (!multi) AppendFloat(identity, 7);
    auto body = std::string(multi ? "MUIT" : "SLNI") + Chunk("INST", settings)
        + Chunk(multi ? "MUIB" : "SLNB", identity) + Chunk("LIST", "PRPS" + Chunk("LCNT", std::string(4, '\0')));
    if (multi) {
        std::string playlist;
        Append(playlist, mode, 4);
        Append(playlist, 1, 4);
        Append(playlist, empty ? 0 : 3, 2);
        if (!empty) {
        Append(playlist, 20, 2);
        std::string child(16, '\0');
        child[0] = 2;
        playlist += child;
        AppendFloat(playlist, 1);
        }
        body += Chunk("PLST", playlist);
    }
    return Chunk("LIST", body);
}
std::string bank(unsigned mode = 0, bool empty = false) {
    using namespace TestData;
    std::string version;
    Append(version, 142, 4);
    std::string count;
    Append(count, 1, 4);
    auto body = std::string("PROJ") + Chunk("BNKI", std::string(16, '\0')) + Chunk("SNDH", {}) + Chunk("STDT", {}) + Chunk("LIST", "WAVS");
    body += Chunk("LIST", "MUIS" + Chunk("LCNT", count) + instrument(1, true, mode, empty));
    body += Chunk("LIST", "SLNS" + Chunk("LCNT", count) + instrument(2, false, mode, empty));
    return Chunk("RIFF", "FEV " + Chunk("FMT ", version) + Chunk("LIST", body));
}
}
void CheckMultiPlayback() {
    using namespace OpenFmod;
    using namespace OpenFmod::Studio;
    Library library;
    library.Load(bank());
    SourceLibrary sources(library);
    AutomationBindings bindings(library, Event{});
    Bank::Guid id{};
    id[0] = 1;
    auto run = [&](std::size_t block) {
        Audio::Mixer mixer(1000);
        auto cursor = std::make_shared<PlaylistCursor>(*library.Require<Instrument>(id).Variants, std::vector<bool>{true});
        std::size_t created = 0;
        MultiPlayback playback(library, id, bindings, [] { return 0.5f; }, cursor, [&](const auto& child) {
            ++created;
            return std::make_unique<SoundPlayback>(library, sources, mixer, child, bindings, [] { return 0.5f; }, 1000);
        });
        playback.Start({0, 0, 2}, {});
        std::uint64_t position = 0;
        std::vector<float> output(block * 2);
        while (playback.IsPlaying()) {
            const auto frames = std::min<std::uint64_t>(block, mixer.FramesUntilNextEnd().value_or(block));
            if (!frames || position > 100) throw std::runtime_error("Playlist stalled at its audio boundary");
            mixer.Render(output.data(), frames);
            position += frames;
            playback.Update({}, static_cast<double>(position));
        }
        if (position != 21 || created != 3) throw std::runtime_error("Playlist repeats introduced a gap or lost an entry");
        playback.Stop(true, static_cast<double>(position));
    };
    run(1);
    run(64);
    for (unsigned mode = 0; mode <= 4; ++mode) {
        Library scopedLibrary;
        scopedLibrary.Load(bank(mode));
        PlaylistState global(scopedLibrary);
        PlaylistState first(scopedLibrary, global);
        PlaylistState second(scopedLibrary, global);
        const bool shared = first.Acquire(id) == second.Acquire(id);
        if (shared != (mode != 0 && mode != 4)) throw std::runtime_error("Incorrect playlist state scope");
    }
    Library emptyLibrary;
    emptyLibrary.Load(bank(0, true));
    PlaylistState state(emptyLibrary);
    AutomationBindings emptyBindings(emptyLibrary, Event{});
    MultiPlayback empty(emptyLibrary, id, emptyBindings, [] { return 0.5f; }, state.Acquire(id), [](const auto&) -> std::unique_ptr<IPlayback> {
        throw std::runtime_error("Empty playlist attempted to construct a child");
    });
    empty.Start({}, {});
    if (empty.IsPlaying()) throw std::runtime_error("Empty playlist remained active");
    empty.Stop(true, 0);
}
