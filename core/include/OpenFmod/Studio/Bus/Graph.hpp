#ifndef OPENFMOD_STUDIO_BUS_GRAPH_HPP
#define OPENFMOD_STUDIO_BUS_GRAPH_HPP
#include "OpenFmod/Audio/Graph.hpp"
#include "OpenFmod/Studio/Project/Library.hpp"
#include "OpenFmod/Studio/Effect/Processor.hpp"
#include "OpenFmod/Studio/Snapshot/Mix.hpp"
namespace OpenFmod::Studio {
class BusGraph {
public:
    BusGraph(const Library& library, std::uint32_t sampleRate);
    void Refresh();
    void SetSnapshots(const std::vector<SnapshotLayer>& layers);
    std::uint64_t CreateScope(const Event& event, std::uint64_t parentScope = 0, std::optional<Bank::Guid> input = {});
    void RemoveScope(std::uint64_t scope);
    std::uint64_t Resolve(const Bank::Guid& bus, std::uint64_t scope = 0);
    void SetVolume(const Bank::Guid& bus, float volume, std::uint64_t scope = 0);
    void SetPitch(const Bank::Guid& bus, float pitch, std::uint64_t scope = 0);
    Audio::Mixer& GetMixer(const Bank::Guid& bus, std::uint64_t scope = 0);
    void Render(float* stereo, std::size_t frames);
    std::optional<std::uint64_t> FramesUntilNextEnd() const;
private:
    using Key = std::pair<std::uint64_t, Bank::Guid>;
    struct Scope {
        Bank::Guid Master;
        Bank::Guid Input;
        std::set<Bank::Guid> Local;
        std::uint64_t Parent{};
    };
    struct Node {
        std::uint64_t AudioId{};
        std::shared_ptr<const Bus> Model;
        float BaseGain{1};
        float BasePitch{1};
        float UserGain{1};
        float UserPitch{1};
        std::map<Bank::Guid, std::shared_ptr<const Effect>> EffectModels;
        std::map<Bank::Guid, std::shared_ptr<EffectProcessor>> Effects;
        std::map<Bank::Guid, std::shared_ptr<Audio::SignalTap>> Taps;
        std::map<Bank::Guid, std::shared_ptr<const Sidechain>> Sidechains;
    };
    Key key(const Bank::Guid& bus, std::uint64_t scope) const;
    std::uint64_t resolve(const Bank::Guid& bus, std::uint64_t scope);
    void attach(Node& node, const std::vector<BusEffect>& effects, Audio::ProcessingStage stage);
    void connect();
    void applySnapshots(Node& node);
    float snapshotProperty(const Bank::Guid& target, std::uint32_t id, float base, const std::map<std::uint32_t, Property>& properties, AutomationType type) const;
    MixControls baseLevels(const Bus& bus) const;
    const Library& library;
    std::uint32_t sampleRate;
    Audio::Graph graph;
    SnapshotMix snapshots;
    std::map<std::uint64_t, Scope> scopes;
    std::map<Key, Node> nodes;
    std::map<Bank::Guid, std::vector<Bank::Guid>> providers;
    std::map<Bank::Guid, std::vector<Bank::Guid>> controls;
    std::set<Key> resolving;
    std::set<std::pair<std::uint64_t, std::uint64_t>> connections;
    std::uint64_t nextScope{1};
};
}
#endif
