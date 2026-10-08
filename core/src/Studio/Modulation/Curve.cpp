#include "OpenFmod/Studio/Modulation/Curve.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace OpenFmod::Studio {
float Interpolate(float start, float end, float fraction, float tension, Interpolation type) {
    if (!std::isfinite(start) || !std::isfinite(end) || !std::isfinite(fraction) || !std::isfinite(tension)) {
        throw FormatError("Nonfinite interpolation parameter");
    }
    if (fraction < 0 || fraction > 1) throw FormatError("Interpolation fraction is outside its segment");
    if (type > Interpolation::Constant) throw FormatError("Invalid interpolation type");
    if (fraction == 0 || type == Interpolation::Constant) return start;
    if (fraction == 1) return end;
    double position = fraction;
    switch (type) {
        case Interpolation::Exponential:
            if (std::abs(tension) >= 0.00578f) {
                const double exponent = 6.952197551727295 * tension;
                position = exponent > 0
                    ? std::exp(exponent * (position - 1)) * -std::expm1(-exponent * position) / -std::expm1(-exponent)
                    : std::expm1(exponent * position) / std::expm1(exponent);
            }
            break;
        case Interpolation::Symmetric:
            if (tension > 0) {
                const double power = 2.0 * tension + 1;
                position = position <= 0.5 ? 0.5 * std::pow(2 * position, power)
                    : 1 - 0.5 * std::pow(2 * (1 - position), power);
            } else {
                const auto centered = 2 * position - 1;
                position += (position - (centered * centered * centered * 0.5 + 0.5)) * tension;
            }
            break;
        case Interpolation::Power: {
            const double maximum = std::max(start, end);
            const auto power = (1 - position) * std::exp2(2 * (start - maximum))
                + position * std::exp2(2 * (end - maximum));
            return static_cast<float>(maximum + std::log2(power) * 0.5);
        }
        case Interpolation::Constant: return start;
        default: throw FormatError("Invalid interpolation type");
    }
    return static_cast<float>(start + (static_cast<double>(end) - start) * position);
}
Curve::Curve(Bank::Guid id, Bank::Guid input, std::vector<CurvePoint> points)
    : id(id), input(input), points(std::move(points)) {
    for (const auto& point : this->points) {
        if (!std::isfinite(point.Value) || !std::isfinite(point.Tension) || point.Type > Interpolation::Constant) {
            throw FormatError("Invalid curve point");
        }
    }
}
const Bank::Guid& Curve::GetId() const { return id; }
const Bank::Guid& Curve::GetInput() const { return input; }
float Curve::Evaluate(float position) const { return evaluate(position, false); }
float Curve::EvaluateTick(std::uint32_t tick) const { return evaluate(tick, true); }
float Curve::evaluate(double position, bool timeline) const {
    if (!std::isfinite(position)) throw FormatError("Nonfinite curve position");
    const CurvePoint* previous = nullptr;
    double previousPosition = 0;
    for (const auto& point : points) {
        float coordinate;
        std::memcpy(&coordinate, &point.Coordinate, sizeof(coordinate));
        const double current = timeline ? static_cast<double>(point.Coordinate) : coordinate;
        if (!std::isfinite(current) || (previous && current < previousPosition)) throw FormatError("Invalid curve coordinate order");
        if (current > position) {
            if (!previous) return point.Value;
            return Interpolate(previous->Value, point.Value, static_cast<float>((position - previousPosition) / (current - previousPosition)), previous->Tension, previous->Type);
        }
        previous = &point;
        previousPosition = current;
    }
    return previous ? previous->Value : 0;
}
Curve ReadCurve(std::string_view bytes) {
    Binary::Reader reader(bytes);
    const auto id = Bank::ReadGuid(reader);
    const auto input = Bank::ReadGuid(reader);
    std::vector<CurvePoint> points;
    for (const auto bytes : reader.Elements(16)) {
        Binary::Reader entry(bytes);
        CurvePoint point;
        point.Coordinate = static_cast<std::uint32_t>(entry.Unsigned(4));
        point.Value = entry.Float();
        point.Tension = entry.Float();
        point.Type = static_cast<Interpolation>(entry.Unsigned(4));
        points.push_back(point);
    }
    reader.Finish();
    return Curve(id, input, std::move(points));
}
}
