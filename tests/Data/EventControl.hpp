#ifndef OPENFMOD_TEST_EVENTCONTROL_HPP
#define OPENFMOD_TEST_EVENTCONTROL_HPP
#include "OpenFmod/Studio/Playback/Control/IEventControl.hpp"
namespace TestData {
class EventControl final : public OpenFmod::Studio::IEventControl {
public:
    void StartEvent(const OpenFmod::Bank::Guid& event) override { Started = event; }
    void StopEvents(const OpenFmod::Bank::Guid& event, bool immediate) override { Stopped = event; Immediate = immediate; }
    float GetParameter(const OpenFmod::Bank::Guid&) const override { return Value; }
    const OpenFmod::Studio::Parameter& GetParameterModel(const OpenFmod::Bank::Guid&) const override { return Model; }
    void SetParameter(const OpenFmod::Bank::Guid&, float value) override { Value = value; }
    OpenFmod::Studio::Parameter Model;
    OpenFmod::Bank::Guid Started{};
    OpenFmod::Bank::Guid Stopped{};
    float Value{};
    bool Immediate{};
};
}
#endif
