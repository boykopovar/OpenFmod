#ifndef OPENFMOD_STUDIO_RUNTIME_CONTEXT_HPP
#define OPENFMOD_STUDIO_RUNTIME_CONTEXT_HPP
#include "OpenFmod/Studio/Runtime/Event.hpp"
#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "OpenFmod/Studio/Parameter/Scope.hpp"
#include "OpenFmod/Studio/Playback/Factory.hpp"
#include "OpenFmod/Studio/Timeline/Timing/Clock.hpp"
#include <random>
namespace OpenFmod::Studio {
struct EventRouting {
    EventRouting(BusGraph& graph, const Event& event, std::shared_ptr<EventRouting> parent, std::optional<Bank::Guid> input);
    ~EventRouting();
    BusGraph& Graph;
    std::shared_ptr<EventRouting> Parent;
    std::uint64_t Scope{};
};
class RuntimeContext : public std::enable_shared_from_this<RuntimeContext> {
public:
    RuntimeContext(std::uint32_t sampleRate, std::uint32_t seed);
    std::shared_ptr<EventInstance> Create(const Bank::Guid& event, std::shared_ptr<EventRouting> parent = {}, std::optional<Bank::Guid> input = {});
    void Retain(const std::shared_ptr<EventInstance>& event);
    void StopAll(bool immediate);
    void Render(float* stereo, std::size_t frames);
    float Random();
    Library Resources;
    SourceLibrary Sources;
    BusGraph Buses;
    ParameterScope Globals;
    PlaylistState Playlists;
    ProgrammerSoundFactory Programmer;
    TimelineClock Clock;
    std::uint32_t SampleRate;
    std::vector<std::function<void()>> Commands;
    std::vector<std::weak_ptr<EventInstance>> Events;
private:
    std::mt19937 generator;
    std::vector<std::shared_ptr<EventInstance>> retained;
    bool rendering{};
};
}
#endif
