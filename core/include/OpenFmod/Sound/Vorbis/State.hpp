#ifndef OPENFMOD_SOUND_VORBIS_STATE_HPP
#define OPENFMOD_SOUND_VORBIS_STATE_HPP
#include <vorbis/codec.h>
namespace OpenFmod::Sound::Vorbis {
struct State {
    vorbis_info Info{};
    vorbis_comment Comment{};
    vorbis_dsp_state Dsp{};
    vorbis_block Block{};
    State() {
        vorbis_info_init(&Info);
        vorbis_comment_init(&Comment);
    }
    ~State() {
        vorbis_block_clear(&Block);
        vorbis_dsp_clear(&Dsp);
        vorbis_comment_clear(&Comment);
        vorbis_info_clear(&Info);
    }
    State(const State&) = delete;
    State& operator=(const State&) = delete;
};
}
#endif
