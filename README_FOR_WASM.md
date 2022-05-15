Lilyplayer on the browser
======

Lilyplayer can be built as a standalone webpage to be used on a web browser supporting web assembly.


Build dependencies
-------

# Qt with Wasm

`Lilyplayer` is based on the [Qt](https://qt.io) library and requires a prebuilt version supporting wasm. Versions 5.15
and above should be fine. In these instructions, I'll use version 6.3.0 as an example.

Download a version of Qt at https://www.qt.io/download and select the `WebAssembly` as part of the components to install.
Choose a folder where to install, for example `/home/<username>/Qt`.

Set the path to QT in the `make_wasm.sh` file at the top and also the version.

# Emscripten compiler

To compile to WebAssembly, you will need an Emscripten compiler.
Follow the instructions at `https://emscripten.org/docs/getting_started/downloads.html`

Set the path to the emscripten folder at the top of `make_wasm.sh`

Compiling
---

Now, simply run `./make_wasm.sh`. The generated html page should appear in the `build_dir_for_wasm_target` folder.

Opening the app
---

To open it, run `emrun --browser="${BROWSER}" ./build_dir_for_wasm_target/lilyplayer.html
