#ifndef SOUND_PLAYER_HH
#define SOUND_PLAYER_HH

#include <memory>
#include <fluidsynth.h>

#if defined(__wasm)
  #define USE_OPENAL 1
#else
  #define USE_OPENAL 0
#endif

#if USE_OPENAL
  #include <vector>
  #include <AL/al.h>
  #include <AL/alc.h>
#endif


class SoundPlayer {
public:
    SoundPlayer();
    ~SoundPlayer();

    void note_on(uint8_t pitch);
    void note_off(uint8_t pitch);
    void all_notes_off();

#if USE_OPENAL
    void output_music();
    void prepare_buffer();
#endif

private:
    std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)> settings;
    std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)> synth;
    // important audio driver must be initialised after settings and synth, and destroyed before them.
    // hence keep it last
    std::unique_ptr<fluid_audio_driver_t, decltype(&delete_fluid_audio_driver)> audio_driver;

#if USE_OPENAL
    std::unique_ptr<ALCdevice, decltype(&alcCloseDevice)> AL_device;
    std::unique_ptr<ALCcontext, decltype(&alcDestroyContext)> AL_context;
    std::unique_ptr<fluid_sequencer_t, decltype(&delete_fluid_sequencer)> sequencer;
    double sample_rate = -1.0f;

    // computing the data of the music buffer takes some time (around 12ms on firefox, 20 on chromium).
    // Therefore, right after playing some notes, pre-compute the next music data to be played. That way,
    // when it will be time to output some music, there won't be these 12 to 20ms delay between the moment
    // the music should be played and when it actually is.
    std::vector<uint16_t> audio_buffer_data;
    ALuint source;
    ALuint buffer_num;
#endif
};

#endif
