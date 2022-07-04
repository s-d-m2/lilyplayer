#include <stdexcept>
#include <iostream>
#include "sound_player.hh"
#include "filename_to_catch.h"

#if USE_OPENAL
  #include <vector>
  #include <cstdint>
  #include <AL/al.h>
  #include <AL/alc.h>
#endif

extern "C" {
  const char* const soundfile_filename = "Yamaha-Grand-Lite-v2.0.sf2";
}

SoundPlayer::SoundPlayer()
  : settings(nullptr, &delete_fluid_settings)
  , synth(nullptr, &delete_fluid_synth)
  , audio_driver(nullptr, &delete_fluid_audio_driver)
#if USE_OPENAL
  , AL_device(nullptr, &alcCloseDevice)
  , AL_context(nullptr, &alcDestroyContext)
  , sequencer(nullptr, &delete_fluid_sequencer)
#endif
{
  std::cerr << "starting loading the sound player" << std::endl;

  settings.reset(new_fluid_settings());
  if (settings == nullptr) {
    throw std::runtime_error("Error: failed to create the settings for fluidsynth");
  }

  synth.reset(new_fluid_synth(settings.get()));
  if (synth == nullptr) {
    throw std::runtime_error("Error: failed to create the synthesiser for fluidsynth");
  }

  if (fluid_synth_sfload(synth.get(), soundfile_filename, 1) == FLUID_FAILED) {
    throw std::runtime_error("Error: failed to load the soundfont");
  }

  std::cerr << "Creating the audio driver" << std::endl;

  audio_driver.reset(new_fluid_audio_driver(settings.get(), synth.get()));
  if (audio_driver == nullptr) {
    throw std::runtime_error("Error: failed to create the audio driver for fluidsynth");
  }

  std::cerr << "Done loading the sound player" << std::endl;

#if USE_OPENAL
  AL_device.reset(alcOpenDevice(nullptr));
  AL_context.reset(alcCreateContext(AL_device.get(), nullptr));

  alcMakeContextCurrent(AL_context.get());

  fluid_settings_setstr(settings.get(), "player.timing-source", "sample");
  fluid_settings_setint(settings.get(), "synth.lock-memory", 0);

  sequencer.reset(new_fluid_sequencer2(false));
  fluid_sequencer_register_fluidsynth(sequencer.get(), synth.get());
  fluid_settings_getnum(settings.get(), "synth.sample-rate", &sample_rate);
  std::cout << "synth.sample-rate: " << sample_rate << std::endl;
#endif
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

#if USE_OPENAL
namespace {
  void load_to_buffer(fluid_synth_t * synth, ALuint  buffer, unsigned sample_rate) {
    std::vector<int16_t> buf_data(sample_rate * 6); // stereo

    fluid_synth_write_s16(synth, static_cast<int>(sample_rate),
			  buf_data.data(), 0, 2,
			  buf_data.data(), 1, 2);

    alBufferData(buffer, AL_FORMAT_STEREO16, buf_data.data(),
		 static_cast<int>(sizeof(int16_t) * buf_data.size()), static_cast<int>(sample_rate));
  }
}
#endif

void SoundPlayer::output_music() {
#if USE_OPENAL
  constexpr int BUFFER_NUM = 1;
  ALuint buffers[BUFFER_NUM];

  alGenBuffers(BUFFER_NUM, buffers);
  for (int i = 0; i < BUFFER_NUM; ++i) {
    std::cout << "Tick: " << fluid_sequencer_get_tick(sequencer.get()) << std::endl;
    load_to_buffer(synth.get(), buffers[i], static_cast<unsigned int>(sample_rate));
  }

  ALuint source;
  alGenSources(1, &source);
  alSourceQueueBuffers(source, 1, buffers);
  alSourcePlay(source);
#endif
}
