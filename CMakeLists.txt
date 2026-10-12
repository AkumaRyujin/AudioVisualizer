cmake_minimum_required(VERSION 3.22)
project(AudioViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Tipo de compilación" FORCE)
endif()

add_compile_options(-O3 -fno-math-errno)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PULSE REQUIRED libpulse)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

include_directories(
    src
    src/audio
    src/graphics
    src/config
    ${PULSE_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

set(AUDIO_SOURCES
    src/audio/AudioCapture.h
    src/audio/AudioCapture.cpp
    src/audio/FftProcessor.h
    src/audio/FftProcessor.cpp
    src/audio/AudioSpectrumSource.h
    src/audio/AudioSpectrumSource.cpp
)

set(GRAPHICS_SOURCES
    src/graphics/ColorPalette.h
    src/graphics/ColorPalette.cpp
    src/graphics/CartesianVisualizer.h
    src/graphics/CartesianVisualizer.cpp
)

set(CONFIG_SOURCES
    src/config/ConfigManager.h
    src/config/ConfigManager.cpp
)

# Módulo y plugin QML de AudioViewer
qt_add_qml_module(AudioViewerPlugin
    URI "AudioViewer"
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml/AudioViewer"
    SOURCES
        ${AUDIO_SOURCES}
        ${GRAPHICS_SOURCES}
        ${CONFIG_SOURCES}
)

target_link_libraries(AudioViewerPlugin PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
PRIVATE
    ${PULSE_LIBRARIES}
    ${FFTW3_LIBRARIES}
)

target_compile_options(AudioViewerPlugin PRIVATE
    ${PULSE_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
    -Wall -Wextra
)

set(RESOURCES
    resources.qrc
)

# Aplicación GUI de Configuración
add_executable(AudioViewer
    src/main.cpp
    ${RESOURCES}
)

target_link_libraries(AudioViewer PRIVATE
    AudioViewerPlugin
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
)

target_compile_options(AudioViewer PRIVATE
    -Wall -Wextra
)

# Reglas de instalación estándar del sistema
include(GNUInstallDirs)

set_target_properties(AudioViewer PROPERTIES
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR};$ORIGIN"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS AudioViewer AudioViewerPlugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES audioviewer.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES resources/icons/audioviewer.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

install(FILES quickshell/desktop.qml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/audioviewer
)

install(PROGRAMS
    scripts/audioviewer-bg
    scripts/audioviewer-toggle
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/qml/AudioViewer
    DESTINATION ${CMAKE_INSTALL_DATADIR}/audioviewer/qml
)

