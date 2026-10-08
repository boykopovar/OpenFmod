#include "OpenFmod/Studio/Condition.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
Condition::Condition(std::string_view bytes) {
    Binary::Reader reader(bytes);
    std::size_t depth = 0;
    while (reader.Remaining()) {
        Instruction instruction;
        instruction.Word = static_cast<std::uint32_t>(reader.Unsigned(4));
        if (instruction.Word & 0xffff0000u) throw FormatError("Invalid condition instruction flags");
        switch (instruction.Word & 255) {
            case 0: case 1: case 3: break;
            case 2: case 0x12:
                if (++depth > 64) throw FormatError("Condition nesting limit exceeded");
                break;
            case 0x10: instruction.First = reader.Float(); break;
            case 0x11: instruction.Parameter = Bank::ReadGuid(reader); break;
            case 0x20:
                instruction.First = reader.Float();
                instruction.Second = reader.Float();
                if (instruction.First > instruction.Second) throw FormatError("Invalid condition range");
                break;
            case 0x30:
                if (!depth-- || instruction.Word != 0x30) throw FormatError("Unexpected condition group end");
                break;
            default: throw FormatError("Invalid condition argument");
        }
        if (!std::isfinite(instruction.First) || !std::isfinite(instruction.Second)) throw FormatError("Nonfinite condition constant");
        instructions.push_back(instruction);
    }
    if (depth) throw FormatError("Unterminated condition group");
}
Condition::Value Condition::apply(Value current, Value argument, std::uint32_t operation) {
    if (!operation) return argument;
    if (std::holds_alternative<bool>(argument)) {
        const auto right = std::get<bool>(argument);
        if (operation == 0x100) return !right;
        if (!std::holds_alternative<bool>(current)) throw FormatError("Expected boolean condition operand");
        const auto left = std::get<bool>(current);
        switch (operation) {
            case 0x1000: return left && right;
            case 0x1100: return left || right;
            case 0x1200: return left != right;
            case 0x1300: return !(left && right);
            case 0x1400: return !(left || right);
            case 0x1500: case 0x1600: return left == right;
            default: throw FormatError("Invalid boolean condition operation");
        }
    }
    if (!std::holds_alternative<float>(current) || !std::holds_alternative<float>(argument)) {
        throw FormatError("Expected numeric condition operand");
    }
    const auto left = std::get<float>(current);
    const auto right = std::get<float>(argument);
    Value result;
    switch (operation) {
        case 0x2000: return left == right;
        case 0x2100: return left != right;
        case 0x2200: return left > right;
        case 0x2300: return left >= right;
        case 0x2400: return left < right;
        case 0x2500: return left <= right;
        case 0x3000: result = left + right; break;
        case 0x3100: result = left - right; break;
        case 0x3200: result = left * right; break;
        case 0x3300: return std::min(left, right);
        case 0x3400: return std::max(left, right);
        default: throw FormatError("Invalid numeric condition operation");
    }
    if (!std::isfinite(std::get<float>(result))) throw FormatError("Nonfinite condition arithmetic result");
    return result;
}
Condition::Value Condition::evaluate(std::size_t& position, const std::function<float(const Bank::Guid&)>& parameter, bool stopping) const {
    Value current;
    while (position < instructions.size()) {
        const auto& instruction = instructions[position++];
        const auto type = instruction.Word & 255;
        const auto operation = instruction.Word & 0xff00;
        Value argument;
        switch (type) {
            case 0: argument = false; break;
            case 1: argument = true; break;
            case 3: argument = stopping; break;
            case 2: case 0x12:
                argument = evaluate(position, parameter, stopping);
                if ((type == 2 && !std::holds_alternative<bool>(argument)) || (type == 0x12 && !std::holds_alternative<float>(argument))) {
                    throw FormatError("Incorrect condition group type");
                }
                break;
            case 0x10: argument = instruction.First; break;
            case 0x11: {
                if (!parameter) throw FormatError("Missing condition parameter resolver");
                const auto value = parameter(instruction.Parameter);
                if (!std::isfinite(value)) throw FormatError("Nonfinite condition parameter");
                argument = value;
                break;
            }
            case 0x20: {
                if (!std::holds_alternative<float>(current)) throw FormatError("Expected numeric condition range operand");
                const auto value = std::get<float>(current);
                const auto minimum = instruction.First;
                const auto maximum = instruction.Second;
                switch (operation) {
                    case 0x4000: current = std::clamp(value, minimum, maximum); break;
                    case 0x5000: current = value >= minimum && value <= maximum; break;
                    case 0x5100: current = value < minimum || value > maximum; break;
                    case 0x5200: current = value >= minimum && value < maximum; break;
                    case 0x5300: current = value < minimum || value >= maximum; break;
                    default: throw FormatError("Invalid condition range operation");
                }
                continue;
            }
            case 0x30: return current;
            default: throw FormatError("Invalid compiled condition");
        }
        current = apply(current, argument, operation);
    }
    return current;
}
bool Condition::Evaluate(const std::function<float(const Bank::Guid&)>& parameter, bool stopping) const {
    if (instructions.empty()) return true;
    std::size_t position = 0;
    const auto result = evaluate(position, parameter, stopping);
    if (!std::holds_alternative<bool>(result)) throw FormatError("Condition did not produce a boolean result");
    return std::get<bool>(result);
}
}
