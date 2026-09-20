cmake_minimum_required(VERSION 3.20)

project(UglyReverb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
  vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
  GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(vst3sdk)

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(UglyReverb
  source/version.h
  source/ids.h
  source/parameters.h
  source/processor.h
  source/processor.cpp
  source/controller.h
  source/controller.cpp
  source/UglyControls.h
  source/UglyControls.cpp
  source/entry.cpp
  resource/ugly_reverb.uidesc
)

target_link_libraries(UglyReverb PRIVATE sdk vstgui_support)

smtg_target_add_plugin_resources(UglyReverb
  RESOURCES
    resource/ugly_reverb.uidesc
    resource/125A_Logo_Master_FINAL.svg
    resource/ugly_reverb_knob_master.png
    resource/ugly_reverb_toggle_off.png
    resource/ugly_reverb_toggle_on.png
)

smtg_target_configure_version_file(UglyReverb)

add_executable(UglyReverbMeasurements
  tests/dsp_measurements.cpp
  source/processor.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/memorystream.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/parameterchanges.cpp
)
target_compile_definitions(UglyReverbMeasurements PRIVATE UGLY_REVERB_TESTING=1)
target_include_directories(UglyReverbMeasurements PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(UglyReverbMeasurements PRIVATE sdk)

enable_testing()
add_test(NAME UglyReverbMeasurements COMMAND UglyReverbMeasurements)

if(SMTG_WIN AND MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT UglyReverb)
endif()
