#!/usr/bin/env bash

set -eux
set -o pipefail

EMSDK_ROOT='/home/sam/code/emsdk'
QT_HOME='/home/sam/code/Qt'
QT_VERSION='6.3.0'

WASM_QMAKE="${QT_HOME}/${QT_VERSION}/wasm_32/bin/qmake"

UIC="${QT_HOME}/${QT_VERSION}/gcc_64/libexec/uic"
MOC="${QT_HOME}/${QT_VERSION}/gcc_64/libexec/moc"
RCC="${QT_HOME}/${QT_VERSION}/gcc_64/libexec/rcc"

MKDIR='mkdir'
DIRNAME='dirname'
REALPATH='realpath'
SED='sed'

THIS_SCRIPT_DIR="$(realpath "$("${DIRNAME}" "${BASH_SOURCE[0]}")")"
FULL_SCRIPT_DIR="$("${REALPATH}" "${THIS_SCRIPT_DIR}")"
BUILD_DIR_FOR_WASM_TARGET="${FULL_SCRIPT_DIR}/build_dir_for_wasm_target"

"${EMSDK_ROOT}/emsdk" activate
. "${EMSDK_ROOT}/emsdk_env.sh"
"${MKDIR}" -p "${BUILD_DIR_FOR_WASM_TARGET}"

"${WASM_QMAKE}" -o "${BUILD_DIR_FOR_WASM_TARGET}" "${THIS_SCRIPT_DIR}/wasm/lilyplayer.pro"

"${RCC}" -o src/ressources.cc ./qdarkstyle/style.qrc

"${UIC}" -o "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"  "${THIS_SCRIPT_DIR}/src/mainwindow.ui"
"${SED}" -i '1i // Avoid warnings on generated headers\n#if !defined(__clang__)\n  #pragma GCC system_header\n#endif\n' "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"
"${SED}" -i 's/^#include <mainwindow.h>$/#include "mainwindow.hh"/' "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"
"${SED}" -i 's/QObject::connect(\(.*\), MainWindow, qOverload<>(&QMainWindow::/QObject::connect(\1, reinterpret_cast<::MainWindow*>(MainWindow), qOverload<>(\&::MainWindow::/' "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"

"${SED}" -i 's/INITIAL_MEMORY=50MB/INITIAL_MEMORY=64MB  -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4/' "${BUILD_DIR_FOR_WASM_TARGET}/Makefile"
#"${SED}" -i 's/ALLOW_MEMORY_GROWTH=1/ALLOW_MEMORY_GROWTH=0  -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4/' "${BUILD_DIR_FOR_WASM_TARGET}/Makefile"

OUT_MOC_FILE="${THIS_SCRIPT_DIR}/src/moc_mainwindow.cc"
"${MOC}" -o "${OUT_MOC_FILE}" "${THIS_SCRIPT_DIR}/src/mainwindow.hh"

# the generated code must be compiled in C++17 mode minimum, and the generated Makefile
# doesn't specify the version.

# make -C "${BUILD_DIR_FOR_WASM_TARGET}" \
#      CXXFLAGS='-std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthreads' # \
# #     LDFLAGS='/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/src/libfluidsynth.a --embed-file /tmp/Yamaha-Grand-Lite-v2.0.sf2@/tmp/Yamaha-Grand-Lite-v2.0.sf2'


pushd "${BUILD_DIR_FOR_WASM_TARGET}"
"${MKDIR}" -p "${BUILD_DIR_FOR_WASM_TARGET}/fluidsynth-emscripten"
emcmake cmake -Denable-oss=off \
 	-D CMAKE_BUILD_TYPE=Release \
 	-D BUILD_SHARED_LIBS=off \
 	-S "${THIS_SCRIPT_DIR}/3rd-party/fluidsynth-emscripten/" \
 	-B "${BUILD_DIR_FOR_WASM_TARGET}/fluidsynth-emscripten"


emmake make -C "${BUILD_DIR_FOR_WASM_TARGET}/fluidsynth-emscripten"  V=1 VERBOSE=1 -j

#pushd 3rd_party_Qt_dir
# ./init-repository --module-subset=qtsvg,qtbase,
## Need to install qtsharedtools with the maintenance tools
#  ./configure -opensource -confirm-license -xplatform wasm-emscripten -release -static -feature-thread -nomake examples -no-dbus -no-ssl -sse2  -no-guess-compiler -release -skip qtdeclarative,qtdoc,qttools,qtdatavis3d,qtcharts,qt5compat,qtmultimedia,qtlottie,qtmqtt,qtopcua,qtquicktimeline,qtquick3d,qtscxml,qttranslations,qtvirtualkeyboard,qtwebengine,qtwebview,qtquick3d,qtwayland,qtwebglplugin,qtxmlpatterns,qtpositioning,qtgamepad,qtcanvas3d,qtconnectivity,qt3d,qtremoteobjects   -- -DQT_HOST_PATH=/home/sam/code/Qt/6.3.0/gcc_64/
# ninja all

COMPILE_FLAGS='-pthread'
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o main.o ../src/main.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o mainwindow.o ../src/mainwindow.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o keyboard.o ../src/keyboard.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o signals_handler.o ../src/signals_handler.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o bin_file_reader.o ../src/bin_file_reader.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o utils.o ../src/utils.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o measures_sequence_extractor.o ../src/measures_sequence_extractor.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o moc_mainwindow.o ../src/moc_mainwindow.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o load_file_from_wasm.o ../src/load_file_from_wasm.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o sound_player.o ../src/sound_player.cc
em++ ${COMPILE_FLAGS} -c -std=c++17 -O2 -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/include -I/home/sam/code/fluisynth_test/fluidsynth-emscripten/build/include -pthread -I../wasm -I. -I../../Qt/6.3.0/wasm_32/include -I../../Qt/6.3.0/wasm_32/include/QtSvgWidgets -I../../Qt/6.3.0/wasm_32/include/QtWidgets -I../../Qt/6.3.0/wasm_32/include/QtSvg -I../../Qt/6.3.0/wasm_32/include/QtGui -I../../Qt/6.3.0/wasm_32/include/QtCore -I. -I. -I/home/sam/.emscripten_ports/openssl/include -I../../Qt/6.3.0/wasm_32/mkspecs/wasm-emscripten -o lilyplayer.js_plugin_import.o /home/sam/code/lilyplayer-gui/build_dir_for_wasm_target/lilyplayer.js_plugin_import.cpp
"${SED}" -e s/@APPNAME@/lilyplayer/g /home/sam/code/Qt/6.3.0/wasm_32/plugins/platforms/wasm_shell.html > /home/sam/code/lilyplayer-gui/build_dir_for_wasm_target/lilyplayer.html
cp -f /home/sam/code/Qt/6.3.0/wasm_32/plugins/platforms/qtloader.js /home/sam/code/lilyplayer-gui/build_dir_for_wasm_target
cp -f /home/sam/code/Qt/6.3.0/wasm_32/plugins/platforms/qtlogo.svg /home/sam/code/lilyplayer-gui/build_dir_for_wasm_target

em++ ${COMPILE_FLAGS} -s WASM=1 -s FULL_ES2=1 -s FULL_ES3=1 -s USE_WEBGL2=1 \
     -s ASYNCIFY=1 \
     -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s EXPORTED_RUNTIME_METHODS=[UTF16ToString,stringToUTF16] \
     --bind -s FETCH=1 -s MODULARIZE=1 -s EXPORT_NAME=createQtAppInstance \
     -g2 -s WASM=1 -s FULL_ES2=1 -s FULL_ES3=1 -s USE_WEBGL2=1 \
     -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s EXPORTED_RUNTIME_METHODS=[UTF16ToString,stringToUTF16] \
     --bind -s FETCH=1 -s MODULARIZE=1 -s EXPORT_NAME=createQtAppInstance -s ASSERTIONS=2 \
     -s DEMANGLE_SUPPORT=1 -s GL_DEBUG=1 --profiling-funcs -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=64MB  \
     -o ./lilyplayer.js  main.o mainwindow.o keyboard.o signals_handler.o bin_file_reader.o utils.o measures_sequence_extractor.o \
     moc_mainwindow.o load_file_from_wasm.o sound_player.o lilyplayer.js_plugin_import.o  \
     --embed-file /home/sam/Yamaha-Grand-Lite-v2.0.sf2@/tmp/Yamaha-Grand-Lite-v2.0.sf2 \
     "${BUILD_DIR_FOR_WASM_TARGET}/fluidsynth-emscripten/src/libfluidsynth.a" \
     -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 \
     ../3rd-party/qt5/qtbase/lib/libQt6Svg.a \
	../3rd-party/qt5/qtbase/lib/libQt6SvgWidgets.a \
	../3rd-party/qt5/qtbase/lib/libQt6Widgets.a \
	../3rd-party/qt5/qtbase/lib/libQt6Core.a \
	../3rd-party/qt5/qtbase/plugins/platforms/libqwasm.a \
	../3rd-party/qt5/qtbase/src/plugins/platforms/wasm/CMakeFiles/QWasmIntegrationPlugin_resources_1.dir/.rcc/qrc_wasmfonts.cpp.o \
	../3rd-party/qt5/qtbase/src/gui/CMakeFiles/Gui_resources_1.dir/.rcc/qrc_qpdf.cpp.o \
	../3rd-party/qt5/qtbase/lib/libQt6OpenGL.a \
	../3rd-party/qt5/qtbase/plugins/iconengines/libqsvgicon.a \
	../3rd-party/qt5/qtbase/plugins/imageformats/libqgif.a \
	../3rd-party/qt5/qtbase/plugins/imageformats/libqico.a \
	../3rd-party/qt5/qtbase/plugins/imageformats/libqsvg.a \
	../3rd-party/qt5/qtbase/plugins/imageformats/libqjpeg.a \
	../3rd-party/qt5/qtbase/lib/libQt6BundledLibjpeg.a \
	../3rd-party/qt5/qtbase/lib/libQt6Gui.a \
	../3rd-party/qt5/qtbase/lib/libQt6BundledHarfbuzz.a \
	../3rd-party/qt5/qtbase/lib/libQt6BundledLibpng.a \
	../3rd-party/qt5/qtbase/lib/libQt6BundledPcre2.a \
	../3rd-party/qt5/qtbase/src/widgets/CMakeFiles/Widgets_resources_1.dir/.rcc/qrc_qstyle.cpp.o \
	../3rd-party/qt5/qtbase/src/widgets/CMakeFiles/Widgets_resources_2.dir/.rcc/qrc_qstyle1.cpp.o \
	../3rd-party/qt5/qtbase/src/widgets/CMakeFiles/Widgets_resources_3.dir/.rcc/qrc_qmessagebox.cpp.o \
	../3rd-party/qt5/qtbase/lib/libQt6BundledFreetype.a
