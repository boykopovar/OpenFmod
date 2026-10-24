#include "OpenFmod/Audio/Graph.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Audio {
struct Graph::Bus {
    explicit Bus(std::uint32_t rate, std::uint64_t parent) : parent(parent), mixer(rate) {}
    std::uint64_t parent;
    Mixer mixer;
    float gain{1};
    std::vector<float> buffer;
    std::vector<std::unique_ptr<IAudioProcessor>> before;
    std::vector<std::unique_ptr<IAudioProcessor>> after;
};
Graph::Graph(std::uint32_t sampleRate) : sampleRate(sampleRate) {
    buses.emplace(0, std::make_unique<Bus>(sampleRate, 0));
}
Graph::~Graph() = default;
Graph::Bus& Graph::requireBus(std::uint64_t id) {
    const auto found = buses.find(id);
    if (found == buses.end()) throw FormatError("Unknown audio bus");
    return *found->second;
}
std::uint64_t Graph::AddBus(std::uint64_t parent) {
    requireBus(parent);
    if (!nextId) throw FormatError("Audio bus identifier overflow");
    const auto id = nextId++;
    buses.emplace(id, std::make_unique<Bus>(sampleRate, parent));
    order.insert(order.begin(), id);
    return id;
}
void Graph::RemoveBus(std::uint64_t bus) {
    requireBus(bus);
    if (!bus) throw FormatError("Cannot remove the output bus");
    for (const auto& entry : buses) {
        if (entry.second->parent == bus) throw FormatError("Cannot remove a bus with children");
    }
    for (const auto& edge : dependencies) {
        if (edge.first == bus || edge.second == bus) throw FormatError("Cannot remove a bus with processing dependencies");
    }
    buses.erase(bus);
    order.erase(std::find(order.begin(), order.end(), bus));
}
std::vector<std::uint64_t> Graph::resolveOrder(const std::set<std::pair<std::uint64_t, std::uint64_t>>& requested) const {
    auto edges = requested;
    for (const auto& entry : buses) {
        if (entry.first) edges.emplace(entry.first, entry.second->parent);
    }
    std::map<std::uint64_t, std::size_t> waiting;
    for (const auto& entry : buses) waiting.emplace(entry.first, 0);
    for (const auto& edge : edges) ++waiting.at(edge.second);
    std::set<std::uint64_t, std::greater<std::uint64_t>> ready;
    for (const auto& entry : waiting) if (!entry.second) ready.insert(entry.first);
    std::vector<std::uint64_t> result;
    result.reserve(buses.size());
    while (!ready.empty()) {
        const auto current = *ready.begin();
        ready.erase(ready.begin());
        result.push_back(current);
        for (auto edge = edges.lower_bound({current, 0}); edge != edges.end() && edge->first == current; ++edge) {
            if (--waiting.at(edge->second) == 0) ready.insert(edge->second);
        }
    }
    if (result.size() != buses.size()) throw FormatError("Cyclic audio processing dependency");
    return result;
}
void Graph::AddDependency(std::uint64_t source, std::uint64_t consumer) {
    requireBus(source);
    requireBus(consumer);
    auto updated = dependencies;
    if (!updated.emplace(source, consumer).second) throw FormatError("Duplicate audio dependency");
    auto updatedOrder = resolveOrder(updated);
    dependencies = std::move(updated);
    order = std::move(updatedOrder);
}
void Graph::RemoveDependency(std::uint64_t source, std::uint64_t consumer) {
    auto updated = dependencies;
    if (!updated.erase({source, consumer})) throw FormatError("Unknown audio dependency");
    auto updatedOrder = resolveOrder(updated);
    dependencies = std::move(updated);
    order = std::move(updatedOrder);
}
Mixer& Graph::GetMixer(std::uint64_t bus) { return requireBus(bus).mixer; }
void Graph::SetGain(std::uint64_t bus, float gain) {
    if (!std::isfinite(gain) || gain < 0) throw FormatError("Invalid audio bus gain");
    requireBus(bus).gain = gain;
}
void Graph::AddProcessor(std::uint64_t bus, ProcessingStage stage, std::unique_ptr<IAudioProcessor> processor) {
    if (!processor) throw FormatError("Missing audio processor");
    if (stage != ProcessingStage::BeforeFader && stage != ProcessingStage::AfterFader) {
        throw FormatError("Invalid audio processing stage");
    }
    auto& node = requireBus(bus);
    auto& chain = stage == ProcessingStage::BeforeFader ? node.before : node.after;
    chain.push_back(std::move(processor));
}
void Graph::Render(float* stereo, std::size_t frames) {
    if (frames > std::numeric_limits<std::size_t>::max() / 2 || (frames && !stereo)) {
        throw FormatError("Invalid audio graph output buffer");
    }
    if (!frames) return;
    const auto samples = frames * 2;
    for (auto& entry : buses) {
        auto& node = *entry.second;
        node.buffer.resize(samples);
        node.mixer.Render(node.buffer.data(), frames);
    }
    for (const auto id : order) {
        auto& node = requireBus(id);
        for (auto& processor : node.before) processor->Process(node.buffer.data(), frames);
        for (auto& value : node.buffer) value *= node.gain;
        for (auto& processor : node.after) processor->Process(node.buffer.data(), frames);
        for (const auto value : node.buffer) {
            if (!std::isfinite(value)) throw FormatError("Nonfinite audio graph output");
        }
        if (id) {
            auto& parent = requireBus(node.parent);
            for (std::size_t index = 0; index < samples; ++index) parent.buffer[index] += node.buffer[index];
        }
    }
    const auto& output = requireBus(0).buffer;
    std::copy(output.begin(), output.end(), stereo);
}
}
