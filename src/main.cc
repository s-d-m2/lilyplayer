#include <iostream>
#include <QApplication>
#include "signals_handler.hh"
#include "mainwindow.hh"

static const char* const stylesheet =
  #include "../qdarkstyle/style.qss"
  ;

static void usage(const char* const prog_name, std::ostream& out_stream = std::cerr)
{
  out_stream << "Usage: " << prog_name << " [Options] [file]\n"
    "\n"
    "Options:\n"
    "  -h, --help		print this help\n"
    "  -i, --input-port	use midi input port as music source (default when no file is given)\n";
}

struct options
{
    bool has_error;
    bool print_help;
    bool list_ports;
    bool use_midi_input;

    std::string filename;

    options()
      : has_error (false)
      , print_help (false)
      , list_ports (false)
      , use_midi_input (false)
      , filename ("")
    {
    }
};

static
struct options get_opts(const int argc, const char * const * const argv)
{
  struct options res;

  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];
    if ((arg == "-h") or (arg == "--help"))
    {
      res.print_help = true;
      continue;
    }

    if ((arg == "-l") or (arg == "--list"))
    {
      res.list_ports = true;
      continue;
    }

    if ((arg == "-i") or (arg == "--input-port"))
    {
      res.use_midi_input = true;
      continue;
    }

    if ((res.filename != "") || res.use_midi_input)
    {
      res.has_error = true;
      return res;
    }
    else
    {
      res.filename = argv[i];
    }
  }

  return res;
}


int main(const int argc, const char* const * const argv)
{
  set_signal_handler();

  const auto opts = get_opts(argc, argv);
  const auto prog_name = argv[0];

  if (opts.has_error)
  {
    usage(prog_name);
    return 2;
  }

  if (opts.print_help)
  {
    usage(prog_name, std::cout);
    return 0;
  }

  if (opts.list_ports)
  {
    list_midi_ports(std::cout);
    return 0;
  }

  int dummy { 0 };
  QApplication a(dummy, nullptr);
  a.setStyleSheet(stylesheet);
  MainWindow w;
  w.show();

  if (opts.use_midi_input)
  {
    w.set_input_port();
  }

  if (opts.filename != "")
  {
    w.open_file( opts.filename );
  }

  return a.exec();
}
