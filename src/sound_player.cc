#include <stdexcept>
#include "sound_player.hh"

SoundPlayer::SoundPlayer()
  : settings(nullptr, &delete_fluid_settings)
  , synth(nullptr, &delete_fluid_synth)
  , audio_driver(nullptr, &delete_fluid_audio_driver)
{
  settings.reset(new_fluid_settings());
  if (settings == nullptr) {
    throw std::runtime_error("Error: failed to create the settings for fluidsynth");
  }

  synth.reset(new_fluid_synth(settings.get()));
  if (synth == nullptr) {
    throw std::runtime_error("Error: failed to create the synthesiser for fluidsynth");
  }

  if (fluid_synth_sfload(synth.get(), "/tmp/Yamaha-Grand-Lite-v2.0.sf2", 1) == FLUID_FAILED) {
    throw std::runtime_error("Error: failed to load the soundfont");
  }

  audio_driver.reset(new_fluid_audio_driver(settings.get(), synth.get()));
  if (audio_driver == nullptr) {
    throw std::runtime_error("Error: failed to create the audio driver for fluidsynth");
  }
}

void SoundPlayer::note_on(uint8_t pitch) {
  fluid_synth_noteon(synth.get(), 0 /* channel */, pitch, 100 /* vel == volume */);
}

void SoundPlayer::note_off(uint8_t pitch) {
  fluid_synth_noteon(synth.get(), 0 /* channel */, pitch, 0 /* vel == volume */);
}

void SoundPlayer::all_notes_off() {
  fluid_synth_all_notes_off(synth.get(), 0 /* channel */);
}
