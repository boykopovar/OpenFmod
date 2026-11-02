#include "OpenFmod/Audio/Graph.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <variant>
namespace OpenFmod::Audio {
struct Graph::Bus {
    using Processor = std::variant<std::shared_ptr<IAudioProcessor>, std::shared_ptr<IAudioTransform>>;
    Bus(std::uint32_t rate, std::uint64_t parent, SpeakerMode layout) : parent(parent), mixer(rate, layout), layout(layout) {}
    std::uint64_t parent;
    Mixer mixer;
    SpeakerMode layout;
    std::vector<float> scratch;
    float gain{1};
    float pitch{1};
    float effectivePitch{1};
    std::vector<float> buffer;
    std::vector<Processor> before;
    std::vector<Processor> after;
};
Graph::Graph(std::uint32_t sampleRate) : sampleRate(sampleRate) {
    buses.emplace(0, std::make_unique<Bus>(sampleRate, 0, SpeakerMode::Stereo));
}
Graph::~Graph() = default;
Graph::Bus& Graph::requireBus(std::uint64_t id) {
    const auto found = buses.find(id);
    if (found == buses.end()) throw FormatError("Unknown audio bus");
    return *found->second;
}
std::uint64_t Graph::AddBus(std::uint64_t parent, SpeakerMode layout) {
    requireBus(parent);
    if (!nextId) throw FormatError("Audio bus identifier overflow");
    const auto id = nextId++;
    buses.emplace(id, std::make_unique<Bus>(sampleRate, parent, layout));
    auto& node = requireBus(id);
    node.effectivePitch = requireBus(parent).effectivePitch;
    node.mixer.SetPitch(node.effectivePitch);
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
void Graph::AddProcessor(std::uint64_t bus, ProcessingStage stage, std::shared_ptr<IAudioProcessor> processor) {
    if (!processor) throw FormatError("Missing audio processor");
    if (stage != ProcessingStage::BeforeFader && stage != ProcessingStage::AfterFader) {
        throw FormatError("Invalid audio processing stage");
    }
    auto& node = requireBus(bus);
    auto& chain = stage == ProcessingStage::BeforeFader ? node.before : node.after;
    chain.push_back(std::move(processor));
}
void Graph::AddTransform(std::uint64_t bus, ProcessingStage stage, std::shared_ptr<IAudioTransform> processor) {
    if (!processor) throw FormatError("Missing audio transform");
    if (stage != ProcessingStage::BeforeFader && stage != ProcessingStage::AfterFader) throw FormatError("Invalid audio processing stage");
    auto& node = requireBus(bus);
    auto& chain = stage == ProcessingStage::BeforeFader ? node.before : node.after;
    chain.push_back(std::move(processor));
}
void Graph::SetPitch(std::uint64_t bus, float pitch) {
    auto& target = requireBus(bus);
    if (!std::isfinite(pitch) || pitch <= 0) throw FormatError("Invalid audio bus pitch");
    if (target.pitch == pitch) return;
    std::map<std::uint64_t, float> effective;
    for (const auto& [id, node] : buses) {
        const auto inherited = id ? effective.at(node->parent) : 1;
        const auto value = (id == bus ? pitch : node->pitch) * inherited;
        if (!std::isfinite(value) || value <= 0) throw FormatError("Audio bus pitch overflow");
        effective.emplace(id, value);
    }
    requireBus(bus).pitch = pitch;
    for (auto& [id, node] : buses) {
        node->effectivePitch = effective.at(id);
        node->mixer.SetPitch(node->effectivePitch);
    }
}
void Graph::Render(float* stereo, std::size_t frames) {
    if (frames > std::numeric_limits<std::size_t>::max() / 8 || (frames && !stereo)) {
        throw FormatError("Invalid audio graph output buffer");
    }
    if (!frames) {
        for (auto& [id, bus] : buses) {
            bus->mixer.Render(nullptr, 0);
            static_cast<void>(id);
        }
        return;
    }
    for (auto& entry : buses) {
        auto& node = *entry.second;
        node.buffer.resize(frames * Speakers(node.layout).GetChannels());
        node.mixer.Render(node.buffer.data(), frames);
    }
    for (const auto id : order) {
        auto& node = requireBus(id);
        auto layout = node.layout;
        auto process = [&](const auto& chain) {
            for (const auto& processor : chain) {
                if (const auto stereoProcessor = std::get_if<std::shared_ptr<IAudioProcessor>>(&processor)) {
                    if (layout != SpeakerMode::Stereo) throw FormatError("Stereo processor received a different channel layout");
                    (*stereoProcessor)->Process(node.buffer.data(), frames);
                } else {
                    const auto& transform = std::get<std::shared_ptr<IAudioTransform>>(processor);
                    const auto outputLayout = transform->GetOutputLayout(layout);
                    node.scratch.resize(frames * Speakers(outputLayout).GetChannels());
                    transform->Process(node.buffer.data(), node.scratch.data(), frames, layout);
                    node.buffer.swap(node.scratch);
                    layout = outputLayout;
                }
            }
        };
        process(node.before);
        for (auto& value : node.buffer) value *= node.gain;
        process(node.after);
        for (const auto value : node.buffer) {
            if (!std::isfinite(value)) throw FormatError("Nonfinite audio graph output");
        }
        if (id) {
            auto& parent = requireBus(node.parent);
            MixMatrix(layout, parent.layout).Add(node.buffer.data(), parent.buffer.data(), frames);
        } else if (layout != SpeakerMode::Stereo) {
            node.scratch.assign(frames * 2, 0);
            MixMatrix(layout, SpeakerMode::Stereo).Add(node.buffer.data(), node.scratch.data(), frames);
            node.buffer.swap(node.scratch);
        }
    }
    const auto& output = requireBus(0).buffer;
    std::copy(output.begin(), output.end(), stereo);
}
std::optional<std::uint64_t> Graph::FramesUntilNextEnd() const {
    std::optional<std::uint64_t> result;
    for (const auto& [id, bus] : buses) {
        const auto end = bus->mixer.FramesUntilNextEnd();
        if (end && (!result || *end < *result)) result = end;
        static_cast<void>(id);
    }
    return result;
}
}
