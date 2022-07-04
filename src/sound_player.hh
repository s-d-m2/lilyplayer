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
  #include <AL/al.h>
  #include <AL/alc.h>
#endif


class SoundPlayer {
public:
    SoundPlayer();
    ~SoundPlayer() = default;

    void note_on(uint8_t pitch);
    void note_off(uint8_t pitch);
    void all_notes_off();

    void output_music();

private:
    std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)> settings;
    std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)> synth;
    // important audio driver must be initialised after settings and synth, and destroyed before them.
    // hence keep it last
    std::unique_ptr<fluid_audio_driver_t, decltype(&delete_fluid_audio_driver)> audio_driver;

private:

#if USE_OPENAL
    std::unique_ptr<ALCdevice, decltype(&alcCloseDevice)> AL_device;
    std::unique_ptr<ALCcontext, decltype(&alcDestroyContext)> AL_context;
    std::unique_ptr<fluid_sequencer_t, decltype(&delete_fluid_sequencer)> sequencer;
    double sample_rate = -1.0f;
#endif
};

#endif
