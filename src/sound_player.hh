#ifndef SOUND_PLAYER_HH
#define SOUND_PLAYER_HH

#include <memory>
#include <fluidsynth.h>

class SoundPlayer {
public:
    SoundPlayer();
    ~SoundPlayer() = default;

    void note_on(uint8_t pitch);
    void note_off(uint8_t pitch);
    void all_notes_off();
private:
    std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)> settings;
    std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)> synth;
    // important audio driver must be initialised after settings and synth, and destroyed before them.
    // hence keep it last
    std::unique_ptr<fluid_audio_driver_t, decltype(&delete_fluid_audio_driver)> audio_driver;
};

#endif
