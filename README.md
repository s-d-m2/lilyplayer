Lilyplayer
========

Lilyplayer plays piano music sheets, as can be seen below

[![Demo](./misc/lilyplayer-fake-video-screenshot.png)](./misc/lilyplayer-demo.webm "demo")

For ready to use builds, you can grab the [appimage on the release page](https://github.com/s-d-m/lilyplayer/releases/tag/continuous),
make it executable and then simply run it.
You can also download [premade music sheets](https://github.com/s-d-m/precompiled_music_sheets_for_lilyplayer) and play them with
lilyplayer.

Web demo
----

You can see lilyplayer in action directly in your browser at https://lilyplayer-web.shuttleapp.rs/

It takes some time to load as it downloads about 50MiB at the start.

Once loaded, click, "input", then "select file". Suitable files for the application are available at
https://github.com/s-d-m/precompiled_music_sheets_for_lilyplayer . Simply download music sheets you like
from there.


Building instructions for Web Assembly
-----------------

To know how to build for WebAssembly, please refer to [the readme file dedicated to webassembly](./README_FOR_WASM.md)

Build dependencies
----------------

`Lilyplayer` requires a C++14 compiler to build. (g++ 5.3 and clang++ 3.6.2 work both fine).
It also depends on the following libraries:

- [`Qt`][qt6]
- [`fluidsynth][fluidsynth]

[qt6]: https://www.qt.io/
[fluidsynth]: https://www.fluidsynth.org

On `debian`, one can install the build dependencies the following way:

	sudo apt-get install qt6-base-dev qt6-base-dev-tools libqt6svg6-dev  g++ libfluidsynth-dev \
	  gawk sed coreutils pkg-config libinstpatch-dev libdbus-1-dev libjack-dev libsdl2-dev \
	  cmake findutils libgomp1

Compiling instructions
-------------------

Once all the dependencies have been installed, you can simply compile `lilyplayer` by entering:

	git clone --recursive 'https://github.com/s-d-m/lilyplayer'
	cd lilyplayer
	make -f Makefile.native

This will generate the `lilyplayer` binary in `./build_dir_for_native_output`

If you want to generate an appimage, you will also need the `wget`, `readlink`, `dirname`.

	sudo apt-get install wget

Then you can generate the appimage using:

	make appimage

A file lilyplayer-x86_64.AppImage should be generated in the bin folder

How to use
----------

This will open an a window showing a piano keyboard.

or you can play a midi file by choosing `select file` in the input menu, or using the `Ctrl + O` shortcut.

When playing a midi file, one can play/pause it using the `space` key, or the `Ctrl + P` shortcut.

Misc
-----

To generate your own music sheets, see [lilydumper](https://github.com/s-d-m/lilydumper) or simply grab
pre-made one [here](https://github.com/s-d-m/precompiled_music_sheets_for_lilyplayer)


Other files you may want to read
--------------------------------

todo.txt contains a list of things that I still need to do.

Bugs & questions
--------------

Report bugs and questions to da.mota.sam@gmail.com (I trust the anti spam filter)
