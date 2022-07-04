#include <signal.h>
#include <iostream>
#include <sstream>
#include <QFileDialog>
#include <QActionGroup>
#include <QMessageBox>
#include <QKeyEvent>
#include <QGraphicsSvgItem>
#include <QGraphicsRectItem>
#include "mainwindow.hh"
#include "ui_mainwindow.hh"
#include "measures_sequence_extractor.hh"

#if defined(__wasm)
#include "load_file_from_wasm.hh"
#endif

// Global variables to "share" state between the signal handler and
// the main event loop.  Only these two pieces should be allowed to
// use these global variables.  To avoid any other piece piece of code
// from using it, the declaration is not written on a header file on
// purpose.
extern volatile sig_atomic_t pause_requested;
extern volatile sig_atomic_t continue_requested;
extern volatile sig_atomic_t exit_requested;
extern volatile sig_atomic_t new_signal_received;

void MainWindow::look_for_signals_change()
{
  if (not new_signal_received)
  {
    return;
  }

  if (exit_requested)
  {
    close();
  }

  if (pause_requested)
  {
    pause_music();
  }

  if (continue_requested)
  {
    continue_music();
  }

  new_signal_received = 0;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
  const auto pressed_key = event->key();

  if ((pressed_key == Qt::Key_Space) or
      (pressed_key == Qt::Key_P) or
      (pressed_key == Qt::Key_Pause))
  {
    toggle_play_pause();
  }
}

void MainWindow::process_keyboard_event(const std::vector<key_down>& keys_down,
					const std::vector<key_up>& keys_up,
					const std::vector<midi_message_t>& /* messages */)
{
  for (auto& key_down : keys_down)
  {
    sound_player.note_on(key_down.pitch);
  }

  for (auto& key_up : keys_up)
  {
    sound_player.note_off(key_up.pitch);
  }

  sound_player.output_music();

  update_keyboard(keys_down, keys_up, this->keyboard);
}

void MainWindow::display_music_sheet(const unsigned music_sheet_pos)
{
  // remove all the music sheets
  music_sheet_scene->clear();

  const auto nb_svg = this->song.svg_files.size();
  const auto nb_rendered = rendered_sheets.size();

  if (nb_svg != nb_rendered)
  {
    throw std::runtime_error("Invalid state detected. The numbered of rendered file should match the number"
			     " of svg files");
  }

  if (music_sheet_pos >= nb_rendered)
  {
    throw std::runtime_error("Invalid file format: it doesn't have enough music sheets.\n"
			     "This should have been prevented from happening while reading the input file.");
  }

  auto sheet = new QGraphicsSvgItem;
  sheet->setSharedRenderer(rendered_sheets[music_sheet_pos].rendered);
  sheet->setFlags(QGraphicsItem::ItemClipsToShape);
  sheet->setCacheMode(QGraphicsItem::NoCache);
  sheet->setZValue(0);
  music_sheet_scene->addItem(sheet);

  // if there was a rectangle displayed the clear function would have called the destructor
  svg_rect = new QGraphicsSvgItem;
  svg_rect->setFlags(QGraphicsItem::ItemClipsToShape);
  svg_rect->setCacheMode(QGraphicsItem::NoCache);
  svg_rect->setZValue(1);
  svg_rect->setSharedRenderer( cursor_rect );
  music_sheet_scene->addItem(svg_rect);

  QByteArray svg_str_rectangle (rendered_sheets[music_sheet_pos].svg_first_line.c_str());
  current_page_viewbox = rendered_sheets[music_sheet_pos].rendered->viewBoxF();
  svg_str_rectangle +=
    "<rect x=\"0.0000\" y=\"0.0000\" width=\"0.0000\" height=\"0.0000\""
    " ry=\"0.0000\" fill=\"currentColor\" fill-opacity=\"0.4\"/></svg>";
  cursor_rect->load(svg_str_rectangle);
}

void MainWindow::process_music_sheet_event(const music_sheet_event& event)
{
  // process the keyboard event. Must have one.
  this->process_keyboard_event(event.keys_down, event.keys_up, event.midi_messages);

  bool update_required = false;

  // is there a svg file change?
  if (event.has_svg_file_change())
  {
    display_music_sheet(event.new_svg_file);
    update_required = true;
  }

  // is there a cursor pos change here?
  if (event.has_cursor_pos_change())
  {
    cursor_rect->load(event.new_cursor_box);

    const auto scene_bounding_rect = svg_rect->sceneBoundingRect();
    const auto scene_bounding_rect_height = scene_bounding_rect.height();
    const auto half_cursor_box_height = event.cursor_box_coord.height() / 2;
    const auto current_page_viewbox_height = current_page_viewbox.height();

    const auto to_scene_y = [&] (const auto y) {
      return y * scene_bounding_rect_height / current_page_viewbox_height;
    };

    const auto rect_to_center = QRectF{scene_bounding_rect.left(),
				       to_scene_y(std::max(event.cursor_box_coord.top() - half_cursor_box_height, 0.0)),
				       scene_bounding_rect.width(),
				       to_scene_y(3 * half_cursor_box_height)};

    this->ui->music_sheet->setSceneRect(rect_to_center);
    update_required = true;
  }

  if (update_required) {
    this->update();
  }
}

void MainWindow::song_event_loop()
{
  this->ts_at_cur_event = std::chrono::steady_clock::now();
  if (is_in_pause or (song_pos == INVALID_SONG_POS))
  {
    QTimer::singleShot(100, this, SLOT(song_event_loop()));
    return;
  }

  if (song_pos == stop_pos)
  {
    stop_song();
    QTimer::singleShot(100, this, SLOT(song_event_loop()));
    return;
  }

  if (song_pos > song.nb_events)
  {
    throw std::runtime_error("Invalid song position found");
  }

  process_music_sheet_event( song.events[song_pos] );



  std::chrono::nanoseconds time_to_wait;
  if (song_pos < song.nb_events)
  {
    const std::chrono::nanoseconds interval_between_two_events {
      (song.events[song_pos + 1].time - song.events[song_pos].time) };

    const std::chrono::nanoseconds time_to_process_cur_event =
      std::chrono::steady_clock::now() - this->ts_at_cur_event;

    // std::cout << " It took " << time_to_process_cur_event.count() << " ns to process one event!!!\n";
    // std::cout << " Max waiting time was " << interval_between_two_events.count() << " ns!!!\n";

    time_to_wait = std::max(interval_between_two_events - time_to_process_cur_event, std::chrono::nanoseconds{0});
  }
  else
  {
    time_to_wait = std::chrono::seconds{3}; // wait 3s after last event
  }

  song_pos++;
  QTimer::singleShot(std::chrono::duration_cast<std::chrono::milliseconds>(time_to_wait), this, SLOT(song_event_loop()));
}

void MainWindow::clear_music_scheet()
{
  stop_song();

  music_sheet_scene->clear();
  const auto nb_rendered = rendered_sheets.size();
  for (auto i = decltype(nb_rendered){0}; i < nb_rendered; ++i)
  {
    delete rendered_sheets[i].rendered;
  }
  rendered_sheets.clear();

  this->ui->start_measure->setMinimum(1);
  this->ui->start_measure->setValue(1);
  this->ui->start_measure->setMaximum(1);
  this->ui->stop_measure->setMinimum(1);
  this->ui->stop_measure->setMaximum(1);
  this->ui->stop_measure->setValue(1);

  // reinitialise the song field
  this->song = bin_song_t();
  this->song_pos = INVALID_SONG_POS;
  this->start_pos = INVALID_SONG_POS;
  this->stop_pos = INVALID_SONG_POS;

  this->update();
}

void MainWindow::pause_music()
{
  this->is_in_pause = true;
  continue_requested = 0;
  this->ui->PlayPauseButton->setText("Play");
  sound_player.all_notes_off();
  sound_player.output_music();
}

void MainWindow::continue_music()
{
  continue_requested = 0;
  is_in_pause = false;
  this->ui->PlayPauseButton->setText("Pause");
}

void MainWindow::stop_song()
{
  pause_music();

  // reset all keys to up on the keyboard (doesn't play key_released events).
  reset_color(keyboard);
  this->update();
}

void MainWindow::replay()
{
  stop_song();
  this->start_pos = 0;
  this->stop_pos = static_cast<decltype(stop_pos)>(this->song.nb_events);
  this->song_pos = this->start_pos;
  continue_music();
}

void MainWindow::open_file(const std::string& filename)
{
  try {
    auto input_song = get_song(filename);
    this->play_song(std::move(input_song));
  } catch (...) {
  }
}

void MainWindow::open_file(const std::string_view& file_data)
{
  try {
    auto input_song = get_song(file_data);
    this->play_song(std::move(input_song));
  } catch (...) {
  }
}

void MainWindow::play_song(bin_song_t input_song)
{
  try
  {
    ui->music_sheet->setScene(music_sheet_scene);

    continue_music();
    clear_music_scheet();
    this->song = std::move(input_song);
    this->start_pos = 0;
    this->stop_pos = static_cast<decltype(stop_pos)>(this->song.nb_events);

    const auto max_measure = find_last_measure(song.events);
    this->ui->start_measure->setMinimum(1);
    this->ui->start_measure->setValue(1);
    this->ui->start_measure->setMaximum(max_measure);
    this->ui->stop_measure->setMinimum(1);
    this->ui->stop_measure->setMaximum(max_measure);
    this->ui->stop_measure->setValue(max_measure);

    this->song_pos = this->start_pos;

    const auto nb_svg = song.svg_files.size();
    if (rendered_sheets.size() != 0)
    {
	throw std::runtime_error("Invalid state detected. There should be no rendered_sheets.");
    }

    // pre-render each svg files first, so when there will be a turn page event, it is already parsed.
    for (unsigned int i = 0; i < nb_svg; ++i)
    {
      const auto& this_sheet = this->song.svg_files[i];
      const char* const sheet_data = static_cast<const char*>(static_cast<const void*>(this_sheet.data.data()));
      const auto sheet_size = this_sheet.data.size();
      const QByteArray music_sheet (sheet_data, static_cast<int>(sheet_size));

      auto current_renderer = new QSvgRenderer;
      const auto is_load_successfull = current_renderer->load(music_sheet);
      if (not is_load_successfull)
      {
	delete current_renderer;
	throw std::runtime_error("Invalid file format: failed to parse a music sheet page.");
      }

      rendered_sheets.emplace_back(sheet_property{ current_renderer,
						   get_first_svg_line(this_sheet.data) });
    }

    display_music_sheet(0);
    continue_music();
  }
  catch (std::exception& e)
  {
    // delete already rendered sheets
    const auto nb_elts = rendered_sheets.size();
    for (unsigned i = 0; i < nb_elts; ++i)
    {
      delete rendered_sheets[i].rendered;
    }
    rendered_sheets.clear();

    const auto err_msg = e.what();
    QMessageBox::critical(this, tr("Failed to open file."),
			  err_msg,
			  QMessageBox::Ok,
			  QMessageBox::Ok);
    clear_music_scheet();
    ui->music_sheet->setScene(instruction_scene);
    pause_music();
  }
}

#if defined(__wasm)
void MainWindow::open_file()
{
  load_from_wasm(".bin, .*, *", [=](const std::string_view file_content) {
    this->open_file(file_content);
  });
}
#else
void MainWindow::open_file()
{
  const QStringList filters = [] () { QStringList tmp;
				      tmp << "Binary files (*.bin)"
				          << "Any files (*)";
				      return tmp;
				    }();

  QFileDialog dialog;
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setViewMode(QFileDialog::List);
  dialog.setNameFilters(filters);

  const auto dialog_ret = dialog.exec();
  if (dialog_ret == QDialog::Accepted)
  {
    const auto files = dialog.selectedFiles();
    if (files.length() == 1)
    {
      const auto file = files[0];
      const auto filename = file.toStdString();

      open_file(filename);
    }
  }
}
#endif

void MainWindow::toggle_play_pause() {
  if (this->is_in_pause)
  {
    continue_music();
  }
  else
  {
    pause_music();
  }
}

void MainWindow::sub_sequence_click()
{
  stop_song();

  const auto start_measure = this->ui->start_measure->value();
  const auto stop_measure = this->ui->stop_measure->value();
  const auto sequences = get_measures_sequence_pos(song,
						   static_cast<unsigned short>(start_measure),
						   static_cast<unsigned short>(stop_measure));

  if (sequences.empty())
  {
    std::cerr << "Error, no sequence going from measure " << start_measure << " to measure " << stop_measure << " found\n";
  }
  else
  {
    if (sequences.size() != 1)
    {
      std::cerr << "several possibilities found. picking first one\n";
    }

    this->start_pos = static_cast<decltype(song_pos)>(sequences[0].first);
    this->stop_pos = static_cast<decltype(song_pos)>(sequences[0].second);
    this->song_pos = this->start_pos;
    const auto music_sheet_pos = find_music_sheet_pos(song.events, song_pos);
    display_music_sheet(music_sheet_pos);
    continue_music();
  }

}

void MainWindow::handle_input_midi(const std::vector<unsigned char> message)
{
  const auto key_events = midi_to_key_events(message);
  const std::vector<midi_message_t> messages { { message } };
  this->process_keyboard_event(key_events.keys_down, key_events.keys_up, messages);
}

void MainWindow::play_fuer_Elise__Beethoven() {
#if defined(__wasm)
  open_file(std::string{"fur_Elise.bin"});
#else
  extern char _binary_fur_Elise_bin_start;
  extern char _binary_fur_Elise_bin_end;

  play_song(&_binary_fur_Elise_bin_start, &_binary_fur_Elise_bin_end);
#endif
}

void MainWindow::play_Prelidium_1__Bach() {
#if defined(__wasm)
  open_file(std::string{"BachJS_BWV846_wtk1-prelude1_wtk1-prelude1.bin"});
#else
  extern char _binary_BachJS_BWV846_wtk1_prelude1_wtk1_prelude1_bin_start;
  extern char _binary_BachJS_BWV846_wtk1_prelude1_wtk1_prelude1_bin_end;

  play_song(&_binary_BachJS_BWV846_wtk1_prelude1_wtk1_prelude1_bin_start,
	    &_binary_BachJS_BWV846_wtk1_prelude1_wtk1_prelude1_bin_end);
#endif
}

void MainWindow::play_Etude_As_dur__Chopin() {
#if defined(__wasm)
  open_file(std::string{"ChopinFF_O25_chopin-op-25-01_chopin-op-25-01.bin"});
#else
  extern char _binary_ChopinFF_O25_chopin_op_25_01_chopin_op_25_01_bin_start;
  extern char _binary_ChopinFF_O25_chopin_op_25_01_chopin_op_25_01_bin_end;

  play_song(&_binary_ChopinFF_O25_chopin_op_25_01_chopin_op_25_01_bin_start,
	    &_binary_ChopinFF_O25_chopin_op_25_01_chopin_op_25_01_bin_end);
#endif
}

void MainWindow::play_song(const char* data_start, const char* data_end) {
  const auto size = static_cast<size_t>(data_end - data_start);
  const auto song_content = std::string_view(data_start, size);

  open_file(song_content);

}

#if !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant" // Qt is not effective-C++ friendy
#endif

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  instruction_scene(new QGraphicsScene(this)),
  keyboard_scene(new QGraphicsScene(this)),
  keyboard(*keyboard_scene),
  music_sheet_scene(new QGraphicsScene(this)),
  rendered_sheets(),
  current_page_viewbox(),
  cursor_rect(new QSvgRenderer(this)),
  svg_rect(nullptr),
#if !defined(__wasm)
  signal_checker_timer(),
#endif
  song(),
  sound_player(),
  sound_listener_via_fluidsynth(*this),
  ts_at_cur_event(),
  is_in_pause(true)
{
  ui->setupUi(this);
  ui->keyboard->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  ui->keyboard->setScene(keyboard_scene);

  QFont font;
  font.setPointSize(20);
  instruction_scene->addText("Select a music sheet using the \n\n         \"input\" menu\n\n    on the top left corner.", font);
  ui->music_sheet->setScene(instruction_scene);

#if !defined(__wasm)
  connect(&signal_checker_timer, SIGNAL(timeout()), this, SLOT(look_for_signals_change()));
  signal_checker_timer.start(100 /* ms */);
#endif

  qRegisterMetaType<std::vector<unsigned char>>("std::vector<unsigned char>");
  connect(this, SIGNAL(midi_message_received(std::vector<unsigned char>)), this, SLOT(handle_input_midi(std::vector<unsigned char>)));
  song_event_loop();
}

#if !defined(__clang__)
  #pragma GCC diagnostic pop
#endif

MainWindow::~MainWindow()
{
  clear_music_scheet();
  delete ui;
}
