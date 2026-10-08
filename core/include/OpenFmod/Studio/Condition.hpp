#ifndef OPENFMOD_STUDIO_CONDITION_HPP
#define OPENFMOD_STUDIO_CONDITION_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include <functional>
#include <variant>
namespace OpenFmod::Studio {
class Condition {
public:
    explicit Condition(std::string_view bytes);
    bool Evaluate(const std::function<float(const Bank::Guid&)>& parameter, bool stopping) const;
private:
    struct Instruction {
        std::uint32_t Word{};
        Bank::Guid Parameter{};
        float First{};
        float Second{};
    };
    using Value = std::variant<std::monostate, bool, float>;
    Value evaluate(std::size_t& position, const std::function<float(const Bank::Guid&)>& parameter, bool stopping) const;
    static Value apply(Value current, Value argument, std::uint32_t operation);
    std::vector<Instruction> instructions;
};
}
#endif
