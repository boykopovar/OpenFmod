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
    return id;
}
void Graph::RemoveBus(std::uint64_t bus) {
    requireBus(bus);
    if (!bus) throw FormatError("Cannot remove the output bus");
    for (const auto& entry : buses) {
        if (entry.second->parent == bus) throw FormatError("Cannot remove a bus with children");
    }
    buses.erase(bus);
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
    for (auto iterator = buses.rbegin(); iterator != buses.rend(); ++iterator) {
        auto& node = *iterator->second;
        for (auto& processor : node.before) processor->Process(node.buffer.data(), frames);
        for (auto& value : node.buffer) value *= node.gain;
        for (auto& processor : node.after) processor->Process(node.buffer.data(), frames);
        for (const auto value : node.buffer) {
            if (!std::isfinite(value)) throw FormatError("Nonfinite audio graph output");
        }
        if (iterator->first) {
            auto& parent = requireBus(node.parent);
            for (std::size_t index = 0; index < samples; ++index) parent.buffer[index] += node.buffer[index];
        }
    }
    const auto& output = requireBus(0).buffer;
    std::copy(output.begin(), output.end(), stereo);
}
}
