#ifndef SOUND_LISTENER_HH
#define SOUND_LISTENER_HH

#include <memory>
#include <fluidsynth.h>

class MainWindow;

class SoundListener
{
  public:
    SoundListener(MainWindow& window_to_update_on_midi_events);

  private:
    std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)> settings;
    std::unique_ptr<fluid_midi_driver_t, decltype(&delete_fluid_midi_driver)> midi_driver;
};

#endif
