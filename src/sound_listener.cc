#include <stdexcept>
#include "sound_listener.hh"
#include "mainwindow.hh"
#include "utils.hh"

namespace {
__attribute__((used))
int on_midi_input_events(void* data, fluid_midi_event_t *event) {
  if (event == nullptr) {
    return FLUID_FAILED;
  }

  if (data == nullptr) {
    return FLUID_FAILED;
  }

  auto* window_ptr = reinterpret_cast<MainWindow*>(data);

  const auto pitch = fluid_midi_event_get_pitch(event);
  const auto vel = fluid_midi_event_get_velocity(event);

  std::vector<key_down> keys_down;
  std::vector<key_up> keys_up;

  if (vel == 0) {
    keys_up.emplace_back(pitch);
  } else {
    keys_down.emplace_back(pitch, 2);
  }

  const auto midi_message = get_midi_from_keys_events(keys_down, keys_up);

  window_ptr->process_keyboard_event(keys_down, keys_up, midi_message);

  return FLUID_OK;
}
}

SoundListener::SoundListener(MainWindow& window_to_update_on_midi_events)
  : settings(nullptr, &delete_fluid_settings)
  , midi_driver(nullptr, &delete_fluid_midi_driver)
{
  settings.reset(new_fluid_settings());
  if (settings == nullptr) {
    throw std::runtime_error("Error: failed to create the settings for fluidsynth midi input");
  }

  fluid_settings_setstr(settings.get(), "midi.portname", "Lilyplayer input midi port via fluidsynth");

  midi_driver.reset(new_fluid_midi_driver(settings.get(), &on_midi_input_events, &window_to_update_on_midi_events));
}
