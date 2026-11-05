#include "OpenFmod/Studio/Runtime/System.hpp"
#include "OpenFmod/Studio/Runtime/State.hpp"
namespace OpenFmod::Studio {
System::System(std::uint32_t sampleRate, std::uint32_t seed) : context(std::make_shared<RuntimeContext>(sampleRate, seed)) {}
System::~System() { context->StopAll(true); }
Bank::Guid System::LoadBank(std::string bytes) {
    const auto bank = context->Resources.Load(std::move(bytes));
    context->Globals.Refresh(context->Resources);
    context->Buses.Refresh();
    return bank;
}
void System::PrepareBank(const Bank::Guid& bank, bool decodeCompressed) {
    for (const auto& wave : context->Resources.GetProject(bank)->Sources.Waves) context->Sources.Prepare(wave.first, decodeCompressed);
}
void System::UnloadBank(const Bank::Guid& bank) {
    context->Resources.GetProject(bank);
    for (const auto& entry : context->Events) if (auto instance = entry.lock()) {
        if (instance->state->BankId == bank) {
            instance->Stop(true);
            instance->state->Available = false;
            instance->state->Routing.reset();
        }
    }
    context->Resources.Unload(bank);
    context->Globals.Refresh(context->Resources);
    context->Buses.Refresh();
    context->Sources.PruneDecoded();
}
std::vector<Bank::Guid> System::GetEvents() const {
    std::vector<Bank::Guid> result;
    for (const auto& event : context->Resources.GetAll<PropertyModel<Event>>()) if (event->Definition.Flags & 1) result.push_back(event->Definition.Id);
    return result;
}
std::vector<Bank::Guid> System::GetEvents(const Bank::Guid& bank) const {
    std::vector<Bank::Guid> result;
    const auto project = context->Resources.GetProject(bank);
    for (const auto& [id, event] : project->Events) if (event.Definition.Flags & 1) result.push_back(id);
    return result;
}
Bank::Guid System::FindParameter(const std::string& name) const { return context->Globals.FindId(name); }
Bank::Guid System::FindId(const std::string& path) const { return context->Resources.FindId(path); }
std::string System::GetPath(const Bank::Guid& id) const { return context->Resources.GetPath(id); }
std::shared_ptr<EventInstance> System::CreateInstance(const Bank::Guid& event) { return context->Create(event); }
void System::SetBusVolume(const Bank::Guid& bus, float volume) { context->Buses.SetVolume(bus, volume); }
void System::SetParameter(const Bank::Guid& parameter, float value, bool immediate) { context->Globals.SetValue(parameter, value, immediate); }
void System::SetProgrammerSoundFactory(ProgrammerSoundFactory factory) { context->Programmer = std::move(factory); }
void System::Render(float* stereo, std::size_t frames) { context->Render(stereo, frames); }
void System::StopAll(bool immediate) { context->StopAll(immediate); }
}
