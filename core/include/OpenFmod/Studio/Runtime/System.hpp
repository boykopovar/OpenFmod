#ifndef OPENFMOD_STUDIO_RUNTIME_SYSTEM_HPP
#define OPENFMOD_STUDIO_RUNTIME_SYSTEM_HPP
#include "OpenFmod/Studio/Playback/Sound.hpp"
#include <memory>
namespace OpenFmod::Studio {
class EventInstance;
class RuntimeContext;
class System {
public:
    explicit System(std::uint32_t sampleRate = 48000, std::uint32_t seed = 0);
    ~System();
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    Bank::Guid LoadBank(std::string bytes);
    void UnloadBank(const Bank::Guid& bank);
    std::vector<Bank::Guid> GetEvents() const;
    std::vector<Bank::Guid> GetEvents(const Bank::Guid& bank) const;
    Bank::Guid FindParameter(const std::string& name) const;
    Bank::Guid FindId(const std::string& path) const;
    std::string GetPath(const Bank::Guid& id) const;
    std::shared_ptr<EventInstance> CreateInstance(const Bank::Guid& event);
    void SetBusVolume(const Bank::Guid& bus, float volume);
    void SetParameter(const Bank::Guid& parameter, float value, bool immediate = false);
    void SetProgrammerSoundFactory(ProgrammerSoundFactory factory);
    void Render(float* stereo, std::size_t frames);
    void StopAll(bool immediate = true);
private:
    std::shared_ptr<RuntimeContext> context;
};
}
#endif
