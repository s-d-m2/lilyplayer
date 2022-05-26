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

THIS_SCRIPT_DIR="$("${DIRNAME}" "${BASH_SOURCE[0]}")"
FULL_SCRIPT_DIR="$("${REALPATH}" "${THIS_SCRIPT_DIR}")"
BUILD_DIR_FOR_WASM_TARGET="${FULL_SCRIPT_DIR}/build_dir_for_wasm_target"

"${EMSDK_ROOT}/emsdk" activate
. "${EMSDK_ROOT}/emsdk_env.sh"
"${MKDIR}" -p "${BUILD_DIR_FOR_WASM_TARGET}"

"${WASM_QMAKE}" -o "${BUILD_DIR_FOR_WASM_TARGET}" "${THIS_SCRIPT_DIR}/wasm/lilyplayer.pro"

"${RCC}" -o src/ressources.cc ./qdarkstyle/style.qrc

"${UIC}" -o "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"  "${THIS_SCRIPT_DIR}/src/mainwindow.ui"
sed -i '1i // Avoid warnings on generated headers\n#if !defined(__clang__)\n  #pragma GCC system_header\n#endif\n' "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"
sed -i 's/^#include <mainwindow.h>$/#include "mainwindow.hh"/' "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"
sed -i 's/QObject::connect(\(.*\), MainWindow, qOverload<>(&QMainWindow::/QObject::connect(\1, reinterpret_cast<::MainWindow*>(MainWindow), qOverload<>(\&::MainWindow::/' "${THIS_SCRIPT_DIR}/src/ui_mainwindow.hh"


OUT_MOC_FILE="${THIS_SCRIPT_DIR}/src/moc_mainwindow.cc"
"${MOC}" -o "${OUT_MOC_FILE}" "${THIS_SCRIPT_DIR}/src/mainwindow.hh"

# the generated code must be compiled in C++17 mode minimum, and the generated Makefile
# doesn't specify the version.

make -C "${BUILD_DIR_FOR_WASM_TARGET}" CXXFLAGS='-std=c++17'
