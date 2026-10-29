#ifndef OPENFMOD_STUDIO_MODULATION_ENVELOPESTATE_HPP
#define OPENFMOD_STUDIO_MODULATION_ENVELOPESTATE_HPP
#include "OpenFmod/Studio/Modulation/Model.hpp"
#include <optional>
namespace OpenFmod::Studio {
class EnvelopeState {
public:
    explicit EnvelopeState(Envelope envelope);
    float Evaluate(double milliseconds) const;
    void Release(double milliseconds);
    bool IsFinished(double milliseconds) const;
private:
    float evaluateHeld(double milliseconds) const;
    Envelope envelope;
    std::optional<double> released;
    float releaseValue{};
};
}
#endif
