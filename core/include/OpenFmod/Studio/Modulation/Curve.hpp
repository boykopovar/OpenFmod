#ifndef OPENFMOD_STUDIO_MODULATION_CURVE_HPP
#define OPENFMOD_STUDIO_MODULATION_CURVE_HPP
#include "OpenFmod/Bank/Identity.hpp"
namespace OpenFmod::Studio {
enum class Interpolation : std::uint32_t { Exponential, Symmetric, Power, Constant };
float Interpolate(float start, float end, float fraction, float tension, Interpolation type);
struct CurvePoint {
    std::uint32_t Coordinate{};
    float Value{};
    float Tension{};
    Interpolation Type{};
};
class Curve {
public:
    Curve(Bank::Guid id, Bank::Guid input, std::vector<CurvePoint> points);
    const Bank::Guid& GetId() const;
    const Bank::Guid& GetInput() const;
    bool IsEmpty() const;
    std::pair<std::uint32_t, std::uint32_t> GetTickRange() const;
    float Evaluate(float position) const;
    float EvaluateTick(std::uint32_t tick) const;
private:
    float evaluate(double position, bool timeline) const;
    Bank::Guid id;
    Bank::Guid input;
    std::vector<CurvePoint> points;
};
Curve ReadCurve(std::string_view bytes);
}
#endif
