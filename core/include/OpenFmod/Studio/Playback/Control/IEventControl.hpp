#ifndef OPENFMOD_STUDIO_PLAYBACK_IEVENTCONTROL_HPP
#define OPENFMOD_STUDIO_PLAYBACK_IEVENTCONTROL_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include "OpenFmod/Studio/Parameter/Model.hpp"
namespace OpenFmod::Studio {
class IEventControl {
public:
    virtual ~IEventControl() = default;
    virtual void StartEvent(const Bank::Guid& event) = 0;
    virtual void StopEvents(const Bank::Guid& event, bool immediate) = 0;
    virtual float GetParameter(const Bank::Guid& parameter) const = 0;
    virtual const Parameter& GetParameterModel(const Bank::Guid& parameter) const = 0;
    virtual void SetParameter(const Bank::Guid& parameter, float value) = 0;
};
}
#endif
