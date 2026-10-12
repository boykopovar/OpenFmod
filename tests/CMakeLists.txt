add_executable(OpenFmodChecks Container.cpp Catalog.cpp Sound.cpp Vorbis.cpp Playlist.cpp Modulation.cpp Condition.cpp Audio/Mixer.cpp Studio/Timeline.cpp Studio/Event.cpp)
target_link_libraries(OpenFmodChecks PRIVATE OpenFmod::OpenFmod vorbisenc)
set_target_properties(OpenFmodChecks PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME OpenFmodChecks COMMAND OpenFmodChecks)
