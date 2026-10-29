#ifndef OPENFMOD_STUDIO_MODULATION_INSTANCE_HPP
#define OPENFMOD_STUDIO_MODULATION_INSTANCE_HPP
#include "OpenFmod/Studio/Modulation/Runtime/EnvelopeState.hpp"
#include "OpenFmod/Studio/Modulation/Runtime/FollowerState.hpp"
#include "OpenFmod/Studio/Modulation/Runtime/RandomState.hpp"
#include "OpenFmod/Studio/Automation/Resolver.hpp"
#include <memory>
namespace OpenFmod::Studio {
class ModulationInstance {
public:
    ModulationInstance(Modulation model, std::shared_ptr<const Mapping> mapping, bool discrete, bool angular, float random);
    ModulationValue Evaluate(double milliseconds) const;
    void ProcessAudio(const float* samples, std::size_t count, double milliseconds);
    void Release(double milliseconds);
    bool IsFinished(double milliseconds) const;
private:
    using State = std::variant<EnvelopeState, RandomState, FollowerState>;
    State create(bool discrete, bool angular, float random) const;
    Modulation model;
    std::shared_ptr<const Mapping> mapping;
    State state;
    bool released{};
};
}
#endif
