Lilyplayer on the browser
======

Lilyplayer can be built as a standalone webpage to be used on a web browser supporting web assembly.


Build dependencies
-------

# Qt with Wasm

`Lilyplayer` is based on the [Qt](https://qt.io) and requires a host compiler. Download Qt from
https://www.qt.io/download-open-source . Make sure qt select Qt >= 6.3, Wasm and Qtsharedtools at
installation.

Then set and export the QT\_HOME variable to where you installed it. $QT\_HOME must contain
`gcc_64/libexec/uic`

# Emscripten compiler

To compile to WebAssembly, you will need an Emscripten compiler.
Follow the instructions at `https://emscripten.org/docs/getting_started/downloads.html`

Note however that this requires `emcc` version `14.18_2`, not the latest version.

Don't forget to run:
```
"/path/to/where/you/downloaded/emscripten/emsdk" activate
. "/path/to/where/you/downloaded/emscripten/emsdk_env.sh"
```

Ultimately, you will need to export the EMSDK\_ROOT environment variable to the root of where you downloaded it.
$EMSDK\_ROOT must contain `upstream/emscripten/{emcc,em++,emcmake,emmake}` in there.

# Other build dependencies

On top of the above mentioned dependencies, the following programs are also required to build lilyplayer for webassembly:

- sed
- ninja
- cmake
- cp
- mkdir
- make

On `debian`, you can install these using:

	sudo apt-get install sed ninja cmake coreutils make


Compiling
---

Once all the dependencies have been installed, and the environment variables have been exported,
you can simply compile `lilyplayer` by entering:

	git clone --recursive 'https://github.com/s-d-m/lilyplayer'
	cd lilyplayer
	make -f Makefile.wasm

The generated html page should appear in the `build_dir_for_wasm_target` folder.

Opening the app
---

To open it, run `emrun --browser="${BROWSER}" ./build_dir_for_wasm_target/lilyplayer.html`
