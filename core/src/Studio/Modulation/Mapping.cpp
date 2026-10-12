#include "OpenFmod/Studio/Modulation/Mapping.hpp"
#include <cmath>
namespace OpenFmod::Studio {
Mapping::Mapping(Bank::Guid id, std::vector<MappingPoint> points) : id(id), points(std::move(points)) {
    if (this->points.empty()) throw FormatError("Empty property mapping");
    const MappingPoint* previous = nullptr;
    for (const auto& point : this->points) {
        if (!std::isfinite(point.Input) || !std::isfinite(point.Output)) throw FormatError("Nonfinite mapping point");
        if (previous && (point.Input < previous->Input || point.Output < previous->Output)) {
            throw FormatError("Unordered property mapping");
        }
        previous = &point;
    }
}
const Bank::Guid& Mapping::GetId() const { return id; }
float Mapping::GetOutputRange() const { return points.back().Output - points.front().Output; }
float Mapping::GetInputRange() const { return points.back().Input - points.front().Input; }
float Mapping::Transform(float value) const { return evaluate(value, false); }
float Mapping::Inverse(float value) const { return evaluate(value, true); }
float Mapping::evaluate(float value, bool inverse) const {
    if (!std::isfinite(value)) throw FormatError("Nonfinite mapping input");
    float previousInput = inverse ? points.front().Output : points.front().Input;
    float previousOutput = inverse ? points.front().Input : points.front().Output;
    if (value < previousInput) return previousOutput;
    for (const auto& point : points) {
        const auto input = inverse ? point.Output : point.Input;
        const auto output = inverse ? point.Input : point.Output;
        if (value < input) return previousOutput + (output - previousOutput) * ((value - previousInput) / (input - previousInput));
        previousInput = input;
        previousOutput = output;
    }
    return previousOutput;
}
Mapping ReadMapping(std::string_view bytes) {
    Binary::Reader reader(bytes);
    const auto id = Bank::ReadGuid(reader);
    std::vector<MappingPoint> points;
    for (const auto bytes : reader.Elements(8)) {
        Binary::Reader entry(bytes);
        const auto input = entry.Float();
        const auto output = entry.Float();
        points.push_back({input, output});
    }
    reader.Finish();
    return Mapping(id, std::move(points));
}
}
