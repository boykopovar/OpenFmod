#ifndef OPENFMOD_STUDIO_TIMELINE_TEMPOGRID_HPP
#define OPENFMOD_STUDIO_TIMELINE_TEMPOGRID_HPP
#include "OpenFmod/Studio/Timeline/Model.hpp"
namespace OpenFmod::Studio {
enum class QuantizationMode : std::uint32_t { None, Bars, EighthNotes };
class TempoGrid {
public:
    explicit TempoGrid(std::vector<TempoMarker> markers);
    std::uint64_t Quantize(std::uint64_t position, QuantizationMode mode, std::uint32_t interval) const;
    double ToBars(std::uint32_t position, std::uint32_t duration) const;
    std::uint32_t ToTicks(std::uint32_t position, double bars) const;
private:
    std::size_t nextMarker(std::uint32_t position) const;
    double rate(std::size_t next) const;
    std::vector<TempoMarker> markers;
};
}
#endif
