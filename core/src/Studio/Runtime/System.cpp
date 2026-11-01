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
void System::UnloadBank(const Bank::Guid& bank) {
    for (const auto& entry : context->Events) if (auto instance = entry.lock()) {
        if (instance->state->BankId == bank) {
            instance->Stop(true);
            instance->state->Available = false;
        }
    }
    context->Resources.Unload(bank);
    context->Globals.Refresh(context->Resources);
    context->Buses.Refresh();
    context->Sources.ClearDecoded();
}
std::vector<Bank::Guid> System::GetEvents() const {
    std::vector<Bank::Guid> result;
    for (const auto& event : context->Resources.GetAll<PropertyModel<Event>>()) result.push_back(event->Definition.Id);
    return result;
}
Bank::Guid System::FindId(const std::string& path) const { return context->Resources.FindId(path); }
std::string System::GetPath(const Bank::Guid& id) const { return context->Resources.GetPath(id); }
std::shared_ptr<EventInstance> System::CreateInstance(const Bank::Guid& event) { return context->Create(event); }
void System::SetBusVolume(const Bank::Guid& bus, float volume) { context->Buses.SetVolume(bus, volume); }
void System::SetParameter(const Bank::Guid& parameter, float value, bool immediate) { context->Globals.SetValue(parameter, value, immediate); }
void System::SetProgrammerSoundFactory(ProgrammerSoundFactory factory) { context->Programmer = std::move(factory); }
void System::Render(float* stereo, std::size_t frames) { context->Render(stereo, frames); }
void System::StopAll(bool immediate) { context->StopAll(immediate); }
}
