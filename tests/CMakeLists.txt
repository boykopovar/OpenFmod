add_executable(OpenFmodChecks Container.cpp Catalog.cpp Sound.cpp Vorbis.cpp Playlist.cpp Modulation.cpp Condition.cpp Audio/Mixer.cpp Audio/Filters.cpp Audio/Dynamics.cpp Audio/Reverb.cpp Audio/Spatial.cpp Studio/Timeline.cpp Studio/Event.cpp Studio/Automation.cpp Studio/Instrument.cpp Studio/Parameter.cpp Studio/Effect.cpp Studio/Snapshot.cpp Studio/Project.cpp)
target_link_libraries(OpenFmodChecks PRIVATE OpenFmod::OpenFmod vorbisenc)
target_sources(OpenFmodChecks PRIVATE Studio/Playback/Multi.cpp)
target_sources(OpenFmodChecks PRIVATE Studio/Playback/Command.cpp)
target_sources(OpenFmodChecks PRIVATE Studio/Playback/Timeline.cpp Studio/Playback/Transport.cpp Studio/Playback/Schedule.cpp)
set_target_properties(OpenFmodChecks PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME OpenFmodChecks COMMAND OpenFmodChecks)





target_sources(OpenFmodChecks PRIVATE Studio/Playback/Follower.cpp)
