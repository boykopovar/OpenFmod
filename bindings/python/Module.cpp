#include "OpenFmod/Studio/Runtime/System.hpp"
#include "OpenFmod/Studio/Runtime/Event.hpp"
#include "OpenFmod/Audio/PcmSource.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <limits>
namespace py = pybind11;
namespace {
py::bytes render(OpenFmod::Studio::System& system, std::size_t frames) {
    if (frames > static_cast<std::size_t>(std::numeric_limits<py::ssize_t>::max()) / (2 * sizeof(float))) {
        throw OpenFmod::FormatError("PCM output size overflow");
    }
    std::vector<float> samples(frames * 2);
    {
        py::gil_scoped_release release;
        system.Render(samples.data(), frames);
    }
    return py::bytes(reinterpret_cast<const char*>(samples.data()), samples.size() * sizeof(float));
}
void renderInto(OpenFmod::Studio::System& system, const py::buffer& output) {
    const auto buffer = output.request(true);
    if (buffer.ndim != 1 || buffer.format != py::format_descriptor<float>::format() || buffer.itemsize != sizeof(float)
        || buffer.strides[0] != sizeof(float) || buffer.size % 2 || reinterpret_cast<std::uintptr_t>(buffer.ptr) % alignof(float)) {
        throw OpenFmod::FormatError("PCM output requires a contiguous writable float32 stereo buffer");
    }
    py::gil_scoped_release release;
    system.Render(static_cast<float*>(buffer.ptr), static_cast<std::size_t>(buffer.size) / 2);
}
}
PYBIND11_MODULE(_openfmod, module) {
    using namespace OpenFmod;
    using namespace OpenFmod::Studio;
    py::register_exception<FormatError>(module, "FormatError", PyExc_RuntimeError);
    py::enum_<PlaybackState>(module, "PlaybackState")
        .value("Stopped", PlaybackState::Stopped)
        .value("Playing", PlaybackState::Playing)
        .value("Sustaining", PlaybackState::Sustaining)
        .value("Stopping", PlaybackState::Stopping);
    py::class_<SoundStart>(module, "SoundStart")
        .def(py::init<>())
        .def_readwrite("OffsetTicks", &SoundStart::OffsetTicks)
        .def_readwrite("OffsetPercent", &SoundStart::OffsetPercent)
        .def_readwrite("Repeats", &SoundStart::Repeats);
    py::class_<Audio::Format>(module, "Format")
        .def(py::init<>())
        .def_readwrite("Rate", &Audio::Format::Rate)
        .def_readwrite("Channels", &Audio::Format::Channels)
        .def_readwrite("Frames", &Audio::Format::Frames);
    py::class_<Audio::PcmData, std::shared_ptr<Audio::PcmData>>(module, "PcmData")
        .def(py::init<>())
        .def_readwrite("Layout", &Audio::PcmData::Layout)
        .def_readwrite("Samples", &Audio::PcmData::Samples);
    py::class_<EventInstance, std::shared_ptr<EventInstance>>(module, "EventInstance")
        .def("Start", &EventInstance::Start, py::arg("position") = SoundStart{})
        .def("Stop", &EventInstance::Stop, py::arg("immediate") = true)
        .def("End", &EventInstance::End)
        .def("IsPlaying", &EventInstance::IsPlaying)
        .def("IsOneshot", &EventInstance::IsOneshot)
        .def("SetMix", &EventInstance::SetMix)
        .def("SetParameter", &EventInstance::SetParameter)
        .def("SetSnapshotIntensity", &EventInstance::SetSnapshotIntensity)
        .def("KeyOff", &EventInstance::KeyOff)
        .def("GetState", &EventInstance::GetState)
        .def("GetTimelinePosition", &EventInstance::GetTimelinePosition)
        .def("GetId", &EventInstance::GetId);
    py::class_<System>(module, "System")
        .def(py::init<std::uint32_t, std::uint32_t>(), py::arg("sample_rate") = 48000, py::arg("seed") = 0)
        .def("LoadBank", [](System& system, const py::bytes& bytes) { return system.LoadBank(static_cast<std::string>(bytes)); })
        .def("UnloadBank", &System::UnloadBank)
        .def("PrepareBank", &System::PrepareBank, py::arg("bank"), py::arg("decodeCompressed") = false, py::call_guard<py::gil_scoped_release>())
        .def("GetEvents", py::overload_cast<>(&System::GetEvents, py::const_))
        .def("GetEvents", py::overload_cast<const Bank::Guid&>(&System::GetEvents, py::const_))
        .def("FindParameter", &System::FindParameter)
        .def("FindId", &System::FindId)
        .def("GetPath", &System::GetPath)
        .def("CreateInstance", &System::CreateInstance)
        .def("SetBusVolume", &System::SetBusVolume)
        .def("SetParameter", &System::SetParameter, py::arg("parameter"), py::arg("value"), py::arg("immediate") = false)
        .def("SetProgrammerSoundFactory", [](System& system, std::function<std::shared_ptr<Audio::PcmData>(const std::string&)> factory) {
            if (!factory) throw FormatError("Missing programmer sound factory");
            system.SetProgrammerSoundFactory([factory = std::move(factory)](const auto& name) { return std::make_unique<Audio::PcmSource>(factory(name)); });
        })
        .def("Render", render)
        .def("RenderInto", renderInto)
        .def("StopAll", &System::StopAll, py::arg("immediate") = true);
}
