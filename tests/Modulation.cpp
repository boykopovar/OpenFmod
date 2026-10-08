#include "OpenFmod/Studio/Modulation/Curve.hpp"
#include "OpenFmod/Studio/Modulation/Mapping.hpp"
#include <cmath>
#include <stdexcept>
void CheckModulation() {
    using namespace OpenFmod::Studio;
    Mapping mapping({}, {{-1, -24}, {0, 0}, {1, 24}});
    if (mapping.Transform(0.5f) != 12 || mapping.Inverse(-6) != -0.25f) throw std::runtime_error("Incorrect property mapping");
    if (mapping.Transform(-2) != -24 || mapping.Inverse(50) != 1) throw std::runtime_error("Mapping exceeded boundary");
    Curve curve({}, {}, {{0, 0, 0, Interpolation::Exponential}, {100, 1, 0, Interpolation::Constant}, {200, 3, 0, Interpolation::Exponential}});
    if (curve.EvaluateTick(50) != 0.5f || curve.EvaluateTick(150) != 1 || curve.EvaluateTick(200) != 3) {
        throw std::runtime_error("Incorrect timeline curve evaluation");
    }
    for (const auto type : {Interpolation::Exponential, Interpolation::Symmetric, Interpolation::Power}) {
        if (Interpolate(-500, 500, 0, 0.7f, type) != -500 || Interpolate(-500, 500, 1, 0.7f, type) != 500) {
            throw std::runtime_error("Interpolation changed endpoints");
        }
        for (const float tension : {-1.0f, 0.0f, 1.0f}) {
            float previous = 0;
            for (int index = 0; index <= 100; ++index) {
                const auto value = Interpolate(0, 1, index / 100.0f, tension, type);
                if (!std::isfinite(value) || value < previous || value > 1) throw std::runtime_error("Invalid interpolation shape");
                previous = value;
            }
        }
    }
    const auto power = Interpolate(0, 1, 0.5f, 0, Interpolation::Power);
    if (std::abs(power - std::log2(2.5f) / 2) > 0.000001f) throw std::runtime_error("Incorrect power interpolation");
}
