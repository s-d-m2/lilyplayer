requires(qtHaveModule(gui))

CONFIG += no_docs_target debug

QT += core widgets svg svgwidgets gui

RESOURCE_CODE = resources.cc

SOURCES += \
	../src/main.cc  \
	../src/mainwindow.cc \
	../src/keyboard.cc \
	../src/signals_handler.cc \
	../src/bin_file_reader.cc \
	../src/utils.cc \
	../src/measures_sequence_extractor.cc \
        ../src/moc_mainwindow.cc \
        ../src/load_file_from_wasm.cc \
        ../src/sound_player.cc

#        ../src/${MOC_FILES} \

# QMAKE_CXXFLAGS += --embed-file /tmp/Yamaha-Grand-Lite-v2.0.sf2@/tmp/Yamaha-Grand-Lite-v2.0.sf2

LIBS += /home/sam/code/fluisynth_test/fluidsynth-emscripten/build/src/libfluidsynth.a --embed-file /tmp/Yamaha-Grand-Lite-v2.0.sf2@/tmp/Yamaha-Grand-Lite-v2.0.sf2

target.path = Wasm_output
INSTALLS += target

FORMS += ../src/mainwindow.ui

TEMPLATE = app
