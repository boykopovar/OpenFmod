#include "OpenFmod/Studio/Playback/Factory.hpp"
#include "OpenFmod/Studio/Playback/Multi.hpp"
#include "OpenFmod/Studio/Playback/Control/Nested.hpp"
#include "OpenFmod/Studio/Playback/Control/Command.hpp"
namespace OpenFmod::Studio {
InstrumentFactory::InstrumentFactory(const Library& library, SourceLibrary& sources, const AutomationBindings& bindings, PlaylistState& playlists, std::function<float()> random, std::uint32_t sampleRate, MixerResolver mixers, RoutedEventFactory events, IEventControl& commands, ProgrammerSoundFactory programmer)
    : library(library), sources(sources), bindings(bindings), playlists(playlists), random(std::move(random)), sampleRate(sampleRate),
      mixers(std::move(mixers)), events(std::move(events)), commands(commands), programmer(std::move(programmer)) {
    if (!this->random || !sampleRate || !this->mixers || !this->events) throw FormatError("Missing instrument factory dependency");
}
std::unique_ptr<IPlayback> InstrumentFactory::Create(const Bank::Guid& instrument) {
    return create(instrument, library.Require<Instrument>(instrument).Settings.Routing.Bus, {});
}
std::unique_ptr<IPlayback> InstrumentFactory::create(const Bank::Guid& instrument, const Bank::Guid& routing, std::set<Bank::Guid> ancestors) {
    if (!ancestors.insert(instrument).second) throw FormatError("Cyclic playlist instrument reference");
    const auto& model = library.Require<Instrument>(instrument);
    if (ancestors.size() > 1 && model.Settings.Routing.Bus != Bank::Guid{}) throw FormatError("Playlist child has an independent output bus");
    if (std::holds_alternative<MultiInstrument>(model.Definition.Source)) {
        return std::make_unique<MultiPlayback>(library, instrument, bindings, random, playlists.Acquire(instrument),
            [this, routing, ancestors = std::move(ancestors)](const auto& child) { return create(child, routing, ancestors); });
    }
    if (std::holds_alternative<CommandInstrument>(model.Definition.Source)) {
        return std::make_unique<CommandPlayback>(library, instrument, bindings, random, commands);
    }
    if (std::holds_alternative<EventInstrument>(model.Definition.Source)) {
        return std::make_unique<NestedPlayback>(library, instrument, bindings, random,
            [this, routing](const auto& event) { return events(event, routing); });
    }
    return std::make_unique<SoundPlayback>(library, sources, mixers(routing), instrument, bindings, random, sampleRate, programmer);
}
}
