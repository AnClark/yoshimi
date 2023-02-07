#
# CMakeLists.txt (DPF)
#
# Copyright 2014, Andrew Deryabin <andrewderyabin@gmail.com> Copyright
# 2016-2021, Will Godfrey & others. Copyright 2023, AnClark Liu
# <anclarkliu@outlook.com>
#
# This file is part of yoshimi, which is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# yoshimi is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# yoshimi.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.20)

project(
  Yoshimi
  LANGUAGES C CXX
  VERSION 2.2.2.1)
set(YOSHIMI_VERSION ${PROJECT_VERSION})

#
# Variables & Global compile options
#

# Yoshimi source dir
set(YOSHIMI_SRC_DIR ${PROJECT_SOURCE_DIR}/yoshimi/src)

# Yoshimi definitions
add_definitions(-DMIN_CONFIG_MAJOR=2)
add_definitions(-DMIN_CONFIG_MINOR=0)
add_definitions(
  -D'YOSHIMI_VERSION="${YOSHIMI_VERSION}"'
  -D'BASE_INSTALL_DIR="${CMAKE_INSTALL_PREFIX}"'
  -DYOSHI_FIFO_DIR="${FifoDirectory}")
add_definitions(-DYOSHIMI="yoshimi")

# Add -fPIC to prevent linker error about Glibc
add_compile_options(-fPIC)

#
# Options
#

# option to build DPF-based plugins
option(VSTPlugin "Build yoshimi vst 2.4 plugin interface" ON)
option(VST3Plugin "Build yoshimi vst3 plugin interface" ON)
option(CLAPPlugin "Build yoshimi clap plugin interface" ON)
option(LV2Plugin_DPF "Build yoshimi lv2 plugin interface (DPF edition)" ON)

#
# Dependency checker - Yoshimi dep libs
#

find_package(PkgConfig REQUIRED)

pkg_check_modules(MXML IMPORTED_TARGET REQUIRED mxml>=2.5)
if(MXML_FOUND)
  find_library(
    MXML_LIBRARY
    NAMES ${MXML_LIBRARIES}
    HINTS ${MXML_LIBDIR})
endif()

pkg_check_modules(FFTW3F IMPORTED_TARGET REQUIRED fftw3f>=0.22)
if(FFTW3F_FOUND)
  find_library(
    FFTW3F_LIBRARY
    NAMES ${FFTW3F_LIBRARIES}
    HINTS ${FFTW3F_LIBDIR})
endif()

pkg_check_modules(SNDFILE IMPORTED_TARGET REQUIRED sndfile)
if(SNDFILE_FOUND)
  find_library(
    SNDFILE_LIBRARY
    NAMES ${SNDFILE_LIBRARIES}
    HINTS ${SNDFILE_LIBDIR})
endif()

#
# Dependency checker - DPF
#

if(VSTPlugin
   OR VST3Plugin
   OR CLAPPlugin
   OR LV2Plugin_DPF)
  set(DPFPlugin ON)
endif()

if(DPFPlugin)
  set(DPF_SOURCE_DIR ${PROJECT_SOURCE_DIR}/dpf)

  # Include DPF
  if(NOT EXISTS "${DPF_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "DPF NOT FOUND")
    message(STATUS "Attempting to checkout submodule")
    find_package(Git REQUIRED)
    execute_process(COMMAND git submodule update --init --recursive)
    if(NOT EXISTS "${DPF_SOURCE_DIR}/CMakeLists.txt")
      message(WARNING "FAILED TO CHECKOUT DPF\n" "Plugins will NOT build.\n"
                      "Please check file permissions and your network")
      set(VSTPlugin OFF)
      set(VST3Plugin OFF)
      set(CLAPPlugin OFF)
      set(LV2Plugin_DPF OFF)
      set(DPFPlugin OFF)
    else()
      add_subdirectory(${DPF_SOURCE_DIR})
    endif()
  else()
    message(STATUS "Found DPF Submodule...")
    add_subdirectory(${DPF_SOURCE_DIR})
  endif()
endif()

#
# Base code stuff
#

# file(GLOB yoshimi_dpf_plugin_files YoshimiPlugin.cpp YoshimiPlugin.h
# YoshimiMusicIO.cpp YoshimiMusicIO.h) file(GLOB yoshimi_dpf_ui_files
# YoshimiEditor.cpp YoshimiEditor.h ${YOSHIMI_SRC_DIR}/UI/MiscGui.cpp
# ${YOSHIMI_SRC_DIR}/UI/MiscGui.h)
file(GLOB
  yoshimi_dsp_src
  ${YOSHIMI_SRC_DIR}/DSP/AnalogFilter.cpp
  ${YOSHIMI_SRC_DIR}/DSP/Filter.cpp
  ${YOSHIMI_SRC_DIR}/DSP/FormantFilter.cpp
  ${YOSHIMI_SRC_DIR}/DSP/SVFilter.cpp
  ${YOSHIMI_SRC_DIR}/DSP/Unison.cpp
  ${YOSHIMI_SRC_DIR}/DSP/FFTwrapper.h
  ${YOSHIMI_SRC_DIR}/DSP/AnalogFilter.h
  ${YOSHIMI_SRC_DIR}/DSP/FormantFilter.h
  ${YOSHIMI_SRC_DIR}/DSP/SVFilter.h
  ${YOSHIMI_SRC_DIR}/DSP/Filter.h
  ${YOSHIMI_SRC_DIR}/DSP/Unison.h)
file(GLOB
  yoshimi_effects_src
  ${YOSHIMI_SRC_DIR}/Effects/Alienwah.cpp
  ${YOSHIMI_SRC_DIR}/Effects/Chorus.cpp
  ${YOSHIMI_SRC_DIR}/Effects/Echo.cpp
  ${YOSHIMI_SRC_DIR}/Effects/EffectLFO.cpp
  ${YOSHIMI_SRC_DIR}/Effects/EffectMgr.cpp
  ${YOSHIMI_SRC_DIR}/Effects/Effect.cpp
  ${YOSHIMI_SRC_DIR}/Effects/Phaser.cpp
  ${YOSHIMI_SRC_DIR}/Effects/Reverb.cpp
  ${YOSHIMI_SRC_DIR}/Effects/EQ.cpp
  ${YOSHIMI_SRC_DIR}/Effects/Distorsion.cpp
  ${YOSHIMI_SRC_DIR}/Effects/DynamicFilter.cpp
  ${YOSHIMI_SRC_DIR}/Effects/Alienwah.h
  ${YOSHIMI_SRC_DIR}/Effects/Chorus.h
  ${YOSHIMI_SRC_DIR}/Effects/Echo.h
  ${YOSHIMI_SRC_DIR}/Effects/EffectLFO.h
  ${YOSHIMI_SRC_DIR}/Effects/EffectMgr.h
  ${YOSHIMI_SRC_DIR}/Effects/Effect.h
  ${YOSHIMI_SRC_DIR}/Effects/Phaser.h
  ${YOSHIMI_SRC_DIR}/Effects/Reverb.h
  ${YOSHIMI_SRC_DIR}/Effects/EQ.h
  ${YOSHIMI_SRC_DIR}/Effects/Distorsion.h
  ${YOSHIMI_SRC_DIR}/Effects/DynamicFilter.h)
file(GLOB
  yoshimi_misc_src
  ${YOSHIMI_SRC_DIR}/Misc/Config.cpp
  ${YOSHIMI_SRC_DIR}/Misc/Config.h
  ${YOSHIMI_SRC_DIR}/Misc/ConfBuild.h
  ${YOSHIMI_SRC_DIR}/Misc/SynthEngine.cpp
  ${YOSHIMI_SRC_DIR}/Misc/Bank.cpp
  ${YOSHIMI_SRC_DIR}/Misc/Microtonal.cpp
  ${YOSHIMI_SRC_DIR}/Misc/Part.cpp
  ${YOSHIMI_SRC_DIR}/Misc/XMLwrapper.cpp
  ${YOSHIMI_SRC_DIR}/Misc/WavFile.cpp
  ${YOSHIMI_SRC_DIR}/Misc/SynthEngine.h
  ${YOSHIMI_SRC_DIR}/Misc/Bank.h
  ${YOSHIMI_SRC_DIR}/Misc/Microtonal.h
  ${YOSHIMI_SRC_DIR}/Misc/Part.h
  ${YOSHIMI_SRC_DIR}/Misc/XMLwrapper.h
  ${YOSHIMI_SRC_DIR}/Misc/WavFile.h
  ${YOSHIMI_SRC_DIR}/Misc/Alloc.h
  ${YOSHIMI_SRC_DIR}/Misc/BuildScheduler.cpp
  ${YOSHIMI_SRC_DIR}/Misc/BuildScheduler.h
  ${YOSHIMI_SRC_DIR}/Misc/TestInvoker.h
  ${YOSHIMI_SRC_DIR}/Misc/TestSequence.h
  ${YOSHIMI_SRC_DIR}/Misc/WaveShapeSamples.h)
file(GLOB
  yoshimi_interface_src
  ${YOSHIMI_SRC_DIR}/Interface/MidiDecode.cpp
  ${YOSHIMI_SRC_DIR}/Interface/MidiDecode.h
  ${YOSHIMI_SRC_DIR}/Interface/InterChange.cpp
  ${YOSHIMI_SRC_DIR}/Interface/InterChange.h
  ${YOSHIMI_SRC_DIR}/Interface/Data2Text.cpp
  ${YOSHIMI_SRC_DIR}/Interface/Text2Data.cpp
  ${YOSHIMI_SRC_DIR}/Interface/Text2Data.h
  ${YOSHIMI_SRC_DIR}/Interface/RingBuffer.h
  ${YOSHIMI_SRC_DIR}/Interface/MidiLearn.cpp
  ${YOSHIMI_SRC_DIR}/Interface/MidiLearn.h)
file(GLOB
  yoshimi_params_src
  ${YOSHIMI_SRC_DIR}/Params/ADnoteParameters.cpp
  ${YOSHIMI_SRC_DIR}/Params/EnvelopeParams.cpp
  ${YOSHIMI_SRC_DIR}/Params/FilterParams.cpp
  ${YOSHIMI_SRC_DIR}/Params/LFOParams.cpp
  ${YOSHIMI_SRC_DIR}/Params/SUBnoteParameters.cpp
  ${YOSHIMI_SRC_DIR}/Params/PADnoteParameters.cpp
  ${YOSHIMI_SRC_DIR}/Params/Controller.cpp
  ${YOSHIMI_SRC_DIR}/Params/Presets.cpp
  ${YOSHIMI_SRC_DIR}/Params/PresetsStore.cpp
  ${YOSHIMI_SRC_DIR}/Params/UnifiedPresets.cpp
  ${YOSHIMI_SRC_DIR}/Params/ADnoteParameters.h
  ${YOSHIMI_SRC_DIR}/Params/EnvelopeParams.h
  ${YOSHIMI_SRC_DIR}/Params/FilterParams.h
  ${YOSHIMI_SRC_DIR}/Params/LFOParams.h
  ${YOSHIMI_SRC_DIR}/Params/SUBnoteParameters.h
  ${YOSHIMI_SRC_DIR}/Params/PADnoteParameters.h
  ${YOSHIMI_SRC_DIR}/Params/PADStatus.h
  ${YOSHIMI_SRC_DIR}/Params/RandomWalk.h
  ${YOSHIMI_SRC_DIR}/Params/OscilParameters.cpp
  ${YOSHIMI_SRC_DIR}/Params/OscilParameters.h
  ${YOSHIMI_SRC_DIR}/Params/Controller.h
  ${YOSHIMI_SRC_DIR}/Params/Presets.h
  ${YOSHIMI_SRC_DIR}/Params/PresetsStore.h
  ${YOSHIMI_SRC_DIR}/Params/UnifiedPresets.h)
file(GLOB
  yoshimi_synth_src
  ${YOSHIMI_SRC_DIR}/Synth/ADnote.cpp
  ${YOSHIMI_SRC_DIR}/Synth/Envelope.cpp
  ${YOSHIMI_SRC_DIR}/Synth/LFO.cpp
  ${YOSHIMI_SRC_DIR}/Synth/OscilGen.cpp
  ${YOSHIMI_SRC_DIR}/Synth/SUBnote.cpp
  ${YOSHIMI_SRC_DIR}/Synth/Resonance.cpp
  ${YOSHIMI_SRC_DIR}/Synth/PADnote.cpp
  ${YOSHIMI_SRC_DIR}/Synth/ADnote.h
  ${YOSHIMI_SRC_DIR}/Synth/Envelope.h
  ${YOSHIMI_SRC_DIR}/Synth/LFO.h
  ${YOSHIMI_SRC_DIR}/Synth/OscilGen.h
  ${YOSHIMI_SRC_DIR}/Synth/SUBnote.h
  ${YOSHIMI_SRC_DIR}/Synth/Resonance.h
  ${YOSHIMI_SRC_DIR}/Synth/PADnote.h
  ${YOSHIMI_SRC_DIR}/Synth/WaveInterpolator.h
  ${YOSHIMI_SRC_DIR}/Synth/XFadeManager.h)
file(GLOB yoshimi_musicio_src ${YOSHIMI_SRC_DIR}/MusicIO/MusicIO.cpp
                                   ${YOSHIMI_SRC_DIR}/MusicIO/MusicIO.h)

add_library(yoshimi_core STATIC
  ${yoshimi_dsp_src}
  ${yoshimi_effects_src}
  ${yoshimi_misc_src}
  ${yoshimi_interface_src}
  ${yoshimi_params_src}
  ${yoshimi_synth_src}
  ${yoshimi_musicio_src}
)

include_directories(
  ${YOSHIMI_SRC_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR}/plugin)

#
# Plugin build commands
#

set(DPF_PLUGIN_TYPES)
if(VSTPlugin)
  list(APPEND DPF_PLUGIN_TYPES "vst2")
endif()
if(VST3Plugin)
  list(APPEND DPF_PLUGIN_TYPES "vst3")
endif()
if(CLAPPlugin)
  list(APPEND DPF_PLUGIN_TYPES "clap")
endif()
if(LV2Plugin_DPF)
  list(APPEND DPF_PLUGIN_TYPES "lv2")
endif()

dpf_add_plugin(
  yoshimi_plugin
  TARGETS
  ${DPF_PLUGIN_TYPES}
  FILES_DSP
  plugin/YoshimiPlugin.cpp
  plugin/YoshimiMusicIO.cpp)

# Link against Yoshimi modules
# NOTICE: You MUST Mind the library order here! The former one depends on the latters.
target_link_libraries(
  yoshimi_plugin-dsp
  PRIVATE yoshimi_core)

# Link against 3rdparty deps
target_link_libraries(
  yoshimi_plugin PRIVATE ${MXML_LIBRARIES} ${SNDFILE_LIBRARIES}
                         ${FFTW3F_LIBRARIES} z)

# Global linker diagnostic options
# NOTICE: This must be put AFTER all build / link commands!
target_link_libraries(
  yoshimi_plugin PRIVATE -Wl,--no-undefined,--unresolved-symbols=report-all)
