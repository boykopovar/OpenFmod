#ifndef OPENFMOD_AUDIO_GRAPH_HPP
#define OPENFMOD_AUDIO_GRAPH_HPP
#include "OpenFmod/Audio/Mixer.hpp"
#include "OpenFmod/Audio/IAudioProcessor.hpp"
#include <vector>
#include <set>
namespace OpenFmod::Audio {
enum class ProcessingStage { BeforeFader, AfterFader };
class Graph {
public:
    explicit Graph(std::uint32_t sampleRate);
    ~Graph();
    std::uint64_t AddBus(std::uint64_t parent);
    void RemoveBus(std::uint64_t bus);
    void AddDependency(std::uint64_t source, std::uint64_t consumer);
    void RemoveDependency(std::uint64_t source, std::uint64_t consumer);
    Mixer& GetMixer(std::uint64_t bus);
    void SetGain(std::uint64_t bus, float gain);
    void AddProcessor(std::uint64_t bus, ProcessingStage stage, std::unique_ptr<IAudioProcessor> processor);
    void Render(float* stereo, std::size_t frames);
private:
    struct Bus;
    Bus& requireBus(std::uint64_t id);
    std::vector<std::uint64_t> resolveOrder(const std::set<std::pair<std::uint64_t, std::uint64_t>>& dependencies) const;
    std::map<std::uint64_t, std::unique_ptr<Bus>> buses;
    std::set<std::pair<std::uint64_t, std::uint64_t>> dependencies;
    std::vector<std::uint64_t> order{0};
    std::uint64_t nextId{1};
    std::uint32_t sampleRate;
};
}
#endif
