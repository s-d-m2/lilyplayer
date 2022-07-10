#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring> // for std::memcmp

#include <QSvgRenderer> // to ensure reading the svg files won't cause any problem

#include "bin_file_reader.hh"
#include "utils.hh"

template <typename T, typename Readable>
static T read_big_endian(Readable& file)
{
  T res = 0;
  for (unsigned int i = 0; i < sizeof(T); ++i)
  {
    uint8_t tmp;
    file.read(static_cast<char*>(static_cast<void*>(&tmp)), sizeof(tmp));
    res = static_cast<decltype(res)>( (res << 8) | tmp);
  }
  return res;
};

template <typename Readable>
static bool is_header_correct(Readable& file, const char expected[4])
{
  char buffer[4];
  file.read(buffer, sizeof(buffer));

  return (static_cast<unsigned>(file.gcount()) == sizeof(buffer)) and
    (std::memcmp(buffer, expected, sizeof(buffer)) == 0);
}

template <typename Readable>
static
std::string read_string(Readable& file)
{
  std::string res;

  uint8_t c = read_big_endian<uint8_t>(file);
  while (c != '\0')
  {
    res.push_back(static_cast<char>(c)); // don't add '\0' by hand in a std::string
    c = read_big_endian<uint8_t>(file);
  }

  return res;
}

template <typename Readable>
static
music_sheet_event read_grouped_event(Readable& file)
{
  music_sheet_event res;

  res.time = read_big_endian<uint64_t>(file);
  const auto nb_events = read_big_endian<uint8_t>(file);

  if (nb_events == 0)
  {
    throw std::invalid_argument("Error: a group of events must have at least one event!");
  }

  for (auto i = decltype(nb_events){0} ; i < nb_events; ++i)
  {
    const auto event_id = read_big_endian<uint8_t>(file);

    enum event_type : uint8_t
    {
      press_key      = 0,
      release_key    = 1,
      set_bar_number = 2,
      set_cursor     = 3,
      set_svg_file   = 4,
    };

    switch (event_id)
    {
      case event_type::press_key:
      {
	const auto pitch = read_big_endian<uint8_t>(file);
	const auto staff_number = read_big_endian<uint8_t>(file);
	res.keys_down.emplace_back( key_down{pitch,  staff_number} );
	break;
      }

      case event_type::release_key:
      {
	const auto pitch = read_big_endian<uint8_t>(file);
	res.keys_up.emplace_back( key_up{ pitch });
	break;
      }

      case event_type::set_bar_number:
      {
	if (res.has_bar_number_change())
	{
	  throw std::invalid_argument("Error: two 'bar number change' happening in the same music-sheet event");
	}

	res.add_bar_number_change(read_big_endian<uint16_t>(file));
	break;
      }

      case event_type::set_cursor:
      {
	if (res.has_cursor_pos_change())
	{
	  throw std::invalid_argument("Error: two 'cursor pos change' happening in the same music-sheet event");
	}

	const auto left = read_big_endian<uint32_t>(file);
	const auto right = read_big_endian<uint32_t>(file);
	const auto top = read_big_endian<uint32_t>(file);
	const auto bottom = read_big_endian<uint32_t>(file);

	// sanity check: the binary file format uses top, left, bottom, right coordinates.
	// ensures that for all cursors, top < bottom, and left < right. Point (0,0) is top left.
	// this is to ensure that computing width and height won't overflow.
	if (left >= right) // left and right are not allowed to be equal as it would mean a box of width 0, so invisible.
	{
	  throw std::invalid_argument("Error: invalid values for left and right position in a cursor box");
	}

	if (top >= bottom) // top and bottom are not allowed to be equal as it would mean a box of height 0, so invisible.
	{
	  throw std::invalid_argument("Error: invalid values for top and bottom position in a cursor box");
	}

	const auto width = right - left;
	const auto height = bottom - top;

	const auto to_dotted_str = [] (const auto num) {
	  return std::to_string(num / 10000) + "." + std::to_string(num % 10000);
	};

	const auto str = // the first svg line is unknown at this point. Must be set later.
	  "<rect x=\"" + to_dotted_str(left) + "\" y=\"" + to_dotted_str(top) + "\" width=\""
	  + to_dotted_str(width) + "\" height=\"" + to_dotted_str(height)
	  + "\" ry=\"0.0000\" fill=\"currentColor\" fill-opacity=\"0.4\"/></svg>";

	res.add_cursor_change(str.c_str(), QRectF{ static_cast<qreal>(left) / 10000,
						   static_cast<qreal>(top) / 10000,
						   static_cast<qreal>(width) / 10000,
						   static_cast<qreal>(height) / 10000 } );
	break;
      }

      case event_type::set_svg_file:
      {
	if (res.has_svg_file_change())
	{
	  throw std::invalid_argument("Error: two 'file change' happening in the same music-sheet event");
	}

	res.add_svg_file_change(read_big_endian<uint16_t>(file));
	break;
      }

      default:
	throw std::invalid_argument("Error: invalid event type");
    }
  }

  if (res.has_svg_file_change() and (not res.has_cursor_pos_change()))
  {
    throw std::invalid_argument("Error: How come a change of a page is not linked to a change of "
				"cursor pos");
  }

  res.midi_messages = get_midi_from_keys_events(res.keys_down, res.keys_up);

  return res;
}

template <typename Readable>
size_t get_input_size(Readable& input) {
  // sadly, the input isn't const but at the end of the function, it should be in the same state
  // as at the beginning.
  const auto init_pos = input.tellg();
  if (init_pos < 0) {
    throw std::runtime_error("Error: couldn't get the input file size (failed to save current position)");
  }

  input.seekg(0, std::ios_base::end);
  const auto file_size = input.tellg();
  if (file_size < 0) {
    throw std::runtime_error("Error: couldn't get the input file size");
  }
  input.seekg(init_pos);
  return static_cast<size_t>(file_size);
}

template <typename Readable>
static bin_song_t get_song_from_file(Readable& file)
{
  // file must start by the magic number 'LPYP'
  const char file_header[4] = { 'L', 'P', 'Y', 'P' };
  if (not is_header_correct(file, file_header))
  {
    throw std::invalid_argument("Error: wrong file format (wrong header)");
  }

  // one byte representing the format number
  const auto format_version = read_big_endian<uint8_t>(file);
  if (format_version != 0) // only one format supported for now
  {
    throw std::invalid_argument("Error: unknown file format");
  }

  // one byte for the number of staff_number->instrument name (nb_staff_num)
  const auto nb_instr = read_big_endian<uint8_t>(file);
  if (nb_instr == 0)
  {
    throw std::invalid_argument("Error: at least one instrument must be played");
  }

  bin_song_t res;
  // for nb_instr do
  for (auto i = decltype(nb_instr){0}; i < nb_instr; ++i)
  {
    // read the instrument name
    auto instr_name = read_string(file);
    res.instr_names.emplace_back( std::move(instr_name) );
  }

  // read the number of music_sheet_event
  const auto nb_group_of_events = read_big_endian<uint64_t>(file);
  if (nb_group_of_events == 0)
  {
    throw std::invalid_argument("Error: a song with nothing happening? That doesn't make sense");
  }

  // read all the music_sheet_event (aka group of events)
  for (auto i = decltype(nb_group_of_events){0}; i < nb_group_of_events; ++i)
  {
    auto grouped_event = read_grouped_event(file);
    res.events.emplace_back( std::move(grouped_event) );
  }

  // sanity check: the events must appear in chronological order
  if (not std::is_sorted( res.events.cbegin(), res.events.cend(), [] (const auto& a, const auto& b) {
	return a.time < b.time;
      }))
  {
    throw std::invalid_argument("Error: The events should appear in chronological order");
  }

  // read the svg files
  const auto nb_svg_files = read_big_endian<uint16_t>(file);
  const auto input_file_size = get_input_size(file);
  for (auto i = decltype(nb_svg_files){0}; i < nb_svg_files; ++i)
  {
    const auto cur_svg_file_size = read_big_endian<uint32_t>(file);

    // Make sure not to allocate more data than can be read from the file
    const auto cur_file_pos = file.tellg();
    if (cur_file_pos < 0) {
      throw std::runtime_error("Error: failed to determine current position in file.");
    }
    const size_t cur_file_pos_u = static_cast<size_t>(cur_file_pos);

    if (cur_file_pos_u + cur_svg_file_size > input_file_size) {
      throw std::runtime_error(std::string{"Error: invalid size of svg file "} + std::to_string(static_cast<unsigned>(i)) + " inside the input detected.");
    }

    svg_data this_file;
    this_file.data.resize( cur_svg_file_size );

    auto ptr = static_cast<void*>(this_file.data.data());
    file.read( static_cast<char*>(ptr), static_cast<int>(cur_svg_file_size) );

    res.svg_files.emplace_back( std::move(this_file) );

    // make sure that parsing the newly read svg file won't caues any problem
    const QByteArray sheet (static_cast<const char*>(static_cast<const void*>(res.svg_files.back().data.data())),
			    static_cast<int>(res.svg_files.back().data.size()));

    QSvgRenderer renderer;
    const auto is_load_successfull = renderer.load(sheet);
    if (not is_load_successfull)
    {
      throw std::runtime_error(std::string{"Error: Failed to read svg file "} +
			       std::to_string(static_cast<long unsigned int>(i)));
    }
  }

  // // sanity check: make sure all "change_music_sheet/turn page" events are valids
  // // todo: rewrite this using the following code:
  // // reason it is not used is that some version of g++ segfault when compiling this code.
  // // therefore, check from time to time if an updated version of g++ fixed this bug.
  // const auto is_missing_svgs = std::any_of(res.events.cbegin(), res.events.cend(), [=] (const auto& ev) {
  //     return ev.has_svg_file_change() and (ev.new_svg_file >= nb_svg_files);
  //   });
  // if (is_missing_svgs)
  // {
  //   throw std::runtime_error("Error: the file is missing some pages of the music sheet inside");
  // }
  for (const auto& elt : res.events)
  {
    if (elt.has_svg_file_change() and (elt.new_svg_file >= nb_svg_files))
    {
      throw std::runtime_error("Error: the file is missing some pages of the music sheet inside");
    }
  }

  // sanity check: file must have been entirely read by now (no more remaining
  // bytes)
  file.peek(); // just to set the eof bit.
  if (not file.eof())
  {
    throw std::invalid_argument("Error: invalid file (extra bytes after end of data)");
  }

  // All cursor changes have been translated into an _almost_ svg file. They are missing
  // the first line at this point. Therefore, process go through all the events to add
  // the first line to all of them.
  std::string current_first_line;
  for (auto& elt : res.events)
  {
    if (elt.has_svg_file_change())
    {
      const auto& this_sheet = res.svg_files[ elt.new_svg_file ];
      current_first_line = get_first_svg_line(this_sheet.data);
    }

    if (elt.has_cursor_pos_change())
    {
      const auto tmp = std::move(elt.new_cursor_box);
      elt.new_cursor_box = current_first_line.c_str();
      elt.new_cursor_box += tmp;
    }
  }

  // sanity check: make sure parsing the cursor_boxes won't cause any problem
  for (const auto& elt : res.events)
  {
    if (elt.has_svg_file_change())
    {
      QSvgRenderer renderer;
      const auto is_load_successfull = renderer.load(elt.new_cursor_box);
      if (not is_load_successfull)
      {
	throw std::runtime_error(std::string{"Error: Failed to read cursor box. Data is "} +
				 elt.new_cursor_box.data() + "\n");
      }
    }
  }

  res.nb_events = res.events.size();

  return res;
}


bin_song_t get_song(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::in);

  if (not file.is_open())
  {
    throw std::runtime_error(std::string{"Error: unable to open midi file ["}
			     + filename + "]");
  }

  return get_song_from_file<>(file);
}

bin_song_t get_song(const std::string_view& file_content) {
  std::stringstream file_data;
  file_data.write(file_content.data(), static_cast<std::streamsize>(file_content.size()));

  return get_song_from_file<>(file_data);
}
