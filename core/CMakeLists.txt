add_library(OpenFmod STATIC
    src/Binary/Reader.cpp
    src/Bank/Container.cpp
    src/Bank/Catalog.cpp
    src/Bank/Sources.cpp
    src/Sound/Bank.cpp
    src/Sound/Metadata.cpp
    src/Sound/Vorbis/Setup.cpp
    src/Sound/Vorbis/Decoder.cpp
    src/Audio/VorbisSource.cpp
    src/Audio/PcmSource.cpp
    src/Audio/Voice.cpp
    src/Audio/Mixer.cpp
    src/Audio/Graph.cpp
    src/Audio/Spatial/Layout.cpp
    src/Audio/Spatial/Speakers.cpp
    src/Audio/Spatial/Panner.cpp
    src/Audio/Spatial/PannerMatrix.cpp
    src/Audio/Spatial/Projection.cpp
    src/Audio/Reverb/Reverb.cpp
    src/Audio/Reverb/Design.cpp
    src/Audio/Reverb/Settings.cpp
    src/Audio/Dynamics/SignalTap.cpp
    src/Audio/Dynamics/Compressor.cpp
    src/Audio/Filters/Biquad.cpp
    src/Audio/Filters/ThreeBandEq.cpp
    src/Audio/Filters/EqualizerDesign.cpp
    src/Audio/Filters/MultibandEq.cpp
    src/Studio/Project/Model.cpp
    src/Studio/Project/Library.cpp
    src/Studio/Project/Sources.cpp
    src/Studio/Playlist.cpp
    src/Studio/Condition.cpp
    src/Studio/Automation/Controller.cpp
    src/Studio/Automation/Bindings.cpp
    src/Studio/Automation/Property.cpp
    src/Studio/Automation/Resolver.cpp
    src/Studio/Effect/Configuration.cpp
    src/Studio/Effect/PannerConfiguration.cpp
    src/Studio/Effect/Processor.cpp
    src/Studio/Effect/Model.cpp
    src/Studio/Snapshot/Model.cpp
    src/Studio/Snapshot/Resolver.cpp
    src/Studio/Bus/Controls.cpp
    src/Studio/Bus/Model.cpp
    src/Studio/Bus/Graph.cpp
    src/Studio/Bus/Effects.cpp
    src/Studio/Bus/Scope.cpp
    src/Studio/Parameter/Model.cpp
    src/Studio/Parameter/State.cpp
    src/Studio/Parameter/Scope.cpp
    src/Studio/Event/Model.cpp
    src/Studio/Instrument/Settings.cpp
    src/Studio/Instrument/Definition.cpp
    src/Studio/Instrument/Model.cpp
    src/Studio/Timeline/Model.cpp
    src/Studio/Timeline/Clock.cpp
    src/Studio/Timeline/Transition.cpp
    src/Studio/Timeline/Sequence.cpp
    src/Studio/Timeline/TempoGrid.cpp
    src/Studio/Modulation/Property.cpp
    src/Studio/Modulation/Model.cpp
    src/Studio/Modulation/EnvelopeState.cpp
    src/Studio/Modulation/FollowerState.cpp
    src/Studio/Modulation/RandomState.cpp
    src/Studio/Modulation/SlewState.cpp
    src/Studio/Modulation/Curve.cpp
    src/Studio/Modulation/Mapping.cpp
)
add_library(OpenFmod::OpenFmod ALIAS OpenFmod)
target_include_directories(OpenFmod PUBLIC include)
target_compile_features(OpenFmod PUBLIC cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(OpenFmod PRIVATE vorbis vorbisenc Threads::Threads)
set_target_properties(OpenFmod PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(OpenFmod PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(OpenFmod PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors)
endif()






