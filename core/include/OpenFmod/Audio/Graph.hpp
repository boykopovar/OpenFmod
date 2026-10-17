#ifndef OPENFMOD_AUDIO_GRAPH_HPP
#define OPENFMOD_AUDIO_GRAPH_HPP
#include "OpenFmod/Audio/Mixer.hpp"
#include "OpenFmod/Audio/IAudioProcessor.hpp"
#include <vector>
namespace OpenFmod::Audio {
enum class ProcessingStage { BeforeFader, AfterFader };
class Graph {
public:
    explicit Graph(std::uint32_t sampleRate);
    ~Graph();
    std::uint64_t AddBus(std::uint64_t parent);
    void RemoveBus(std::uint64_t bus);
    Mixer& GetMixer(std::uint64_t bus);
    void SetGain(std::uint64_t bus, float gain);
    void AddProcessor(std::uint64_t bus, ProcessingStage stage, std::unique_ptr<IAudioProcessor> processor);
    void Render(float* stereo, std::size_t frames);
private:
    struct Bus;
    Bus& requireBus(std::uint64_t id);
    std::map<std::uint64_t, std::unique_ptr<Bus>> buses;
    std::uint64_t nextId{1};
    std::uint32_t sampleRate;
};
}
#endif
