#include "OpenFmod/Studio/Runtime/State.hpp"
#include <algorithm>
#include <limits>
namespace OpenFmod::Studio {
RuntimeContext::RuntimeContext(std::uint32_t sampleRate, std::uint32_t seed)
    : Sources(Resources), Buses(Resources, sampleRate), Globals(Resources), Playlists(Resources), Clock(sampleRate), SampleRate(sampleRate), generator(seed) {}
float RuntimeContext::Random() { return std::generate_canonical<float, 24>(generator); }
std::shared_ptr<EventInstance> RuntimeContext::Create(const Bank::Guid& event, std::shared_ptr<EventRouting> parent, std::optional<Bank::Guid> input) {
    auto result = std::shared_ptr<EventInstance>(new EventInstance(shared_from_this(), event, std::move(parent), input));
    Events.push_back(result);
    return result;
}
void RuntimeContext::Retain(const std::shared_ptr<EventInstance>& event) {
    if (std::find(retained.begin(), retained.end(), event) == retained.end()) retained.push_back(event);
}
void RuntimeContext::StopAll(bool immediate) {
    for (const auto& entry : Events) if (auto event = entry.lock()) event->Stop(immediate);
    if (immediate) { retained.clear(); Commands.clear(); }
}
void RuntimeContext::Render(float* stereo, std::size_t frames) {
    if (rendering || (frames && !stereo) || frames > std::numeric_limits<std::size_t>::max() / 2) throw FormatError("Invalid or recursive system render");
    rendering = true;
    try {
        std::size_t frame = 0;
        while (frame < frames) {
            const auto block = std::min<std::size_t>(64, frames - frame);
            auto after = Clock;
            after.Advance(block);
            const auto endTick = after.GetTick();
            std::size_t prepared = Events.size();
            for (std::size_t i = 0; i < prepared; ++i) if (auto event = Events[i].lock(); event && event->IsPlaying()) {
                event->state->Update();
                event->state->Prepare(static_cast<std::uint32_t>(endTick - Clock.GetTick()));
            }
            std::size_t remaining = block;
            while (remaining) {
                for (unsigned pass = 0;; ++pass) {
                    if (pass == 100) throw FormatError("Event commands contain an instantaneous start cycle");
                    for (std::size_t i = 0; i < Events.size(); ++i) if (auto event = Events[i].lock(); event && event->IsPlaying()) {
                        if (i > prepared + 1000) throw FormatError("Nested events contain an instantaneous start cycle");
                        if (i >= prepared) event->state->Prepare(static_cast<std::uint32_t>(endTick - Clock.GetTick()));
                        event->state->Update();
                    }
                    prepared = Events.size();
                    if (Commands.empty()) break;
                    auto commands = std::move(Commands);
                    Commands.clear();
                    for (auto& command : commands) command();
                }
                std::vector<SnapshotLayer> snapshots;
                auto count = remaining;
                for (const auto& entry : Events) if (auto event = entry.lock(); event && event->IsPlaying()) {
                    if (const auto next = event->state->NextBoundary()) {
                        if (*next <= Clock.GetTick()) throw FormatError("Event scheduler did not consume a due boundary");
                        count = std::min<std::uint64_t>(count, Clock.FramesUntil(*next));
                    }
                    if (event->state->Snapshot) {
                        const auto& layers = event->state->Snapshot->GetLayers();
                        snapshots.insert(snapshots.end(), layers.begin(), layers.end());
                    }
                }
                Buses.SetSnapshots(snapshots);
                Buses.Render(nullptr, 0);
                if (const auto end = Buses.FramesUntilNextEnd()) count = std::min<std::uint64_t>(count, *end);
                if (!count) throw FormatError("Audio render made no progress");
                Buses.Render(stereo + frame * 2, count);
                Clock.Advance(count);
                Globals.Advance(static_cast<double>(count) / SampleRate);
                frame += count;
                remaining -= count;
            }
            retained.erase(std::remove_if(retained.begin(), retained.end(), [](const auto& event) { return !event->IsPlaying(); }), retained.end());
            Events.erase(std::remove_if(Events.begin(), Events.end(), [](const auto& event) { return event.expired(); }), Events.end());
        }
        for (std::size_t i = 0; i < Events.size(); ++i) if (auto event = Events[i].lock(); event && event->IsPlaying()) event->state->Update();
        rendering = false;
    } catch (...) {
        rendering = false;
        throw;
    }
}
}
