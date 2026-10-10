cmake_minimum_required(VERSION 3.22)
project(VirtualLinnStrument VERSION 0.1.0 LANGUAGES C CXX)

# Virtual LinnStrument: the real firmware, compiled for the desktop against the stand-ins the
# tests use, behind a window that draws its lights and turns the mouse into touches, with a
# virtual MIDI port named like the instrument. See README.md.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

set(JUCE_VERSION_TAG 9.0.4)
set(JUCE_PATH "" CACHE PATH "A local JUCE checkout to use instead of fetching ${JUCE_VERSION_TAG}")
if(JUCE_PATH)
  add_subdirectory(${JUCE_PATH} juce)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${JUCE_VERSION_TAG}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SUPPORT ${REPO}/tests/support)
set(MERGED_DIR ${CMAKE_CURRENT_BINARY_DIR}/sketch)
set(MERGED ${MERGED_DIR}/linnstrument-firmware.cpp)

# the sketch becomes one C++ file the way the Arduino build and the desktop tests do it
file(GLOB SKETCH_SOURCES CONFIGURE_DEPENDS ${REPO}/*.ino ${REPO}/*.h)
add_custom_command(
  OUTPUT ${MERGED}
  COMMAND go run ./tools/linnstrument merge ${MERGED}
  WORKING_DIRECTORY ${REPO}
  DEPENDS ${SKETCH_SOURCES} ${REPO}/tools/linnstrument/test.go ${REPO}/tools/linnstrument/main.go
  COMMENT "Merging the sketch into one file")
add_custom_target(MergedSketch DEPENDS ${MERGED})

juce_add_gui_app(VirtualLinnStrument
  PRODUCT_NAME "Virtual LinnStrument"
  COMPANY_NAME "Roger Linn Design"
  BUNDLE_ID com.rogerlinndesign.virtuallinnstrument
  ICON_BIG Resources/Icon/Icon-512.png
  VERSION ${PROJECT_VERSION})
juce_generate_juce_header(VirtualLinnStrument)
add_dependencies(VirtualLinnStrument MergedSketch)

juce_add_binary_data(VirtualLinnStrumentData SOURCES
  Resources/Fonts/Mulish-Bold.ttf
  Resources/Fonts/Mulish-Regular.ttf
  Resources/Fonts/Sarpanch-Black.ttf
  Resources/Fonts/Sarpanch-Bold.ttf)

set(FIRMWARE_SOURCES Source/Firmware/FirmwareBridge.cpp ${SUPPORT}/arduino_mock.cpp)
target_sources(VirtualLinnStrument PRIVATE
  Source/Main.cpp
  Source/MainComponent.cpp
  Source/Surface.cpp
  Source/Engine.cpp
  Source/LinnLookAndFeel.cpp
  ${FIRMWARE_SOURCES})

# the firmware is compiled as the Arduino build would, with its warnings silenced, and only
# these two files see the Arduino stand-ins, whose min and max macros must not reach JUCE
set_source_files_properties(${FIRMWARE_SOURCES} PROPERTIES
  COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>"
  INCLUDE_DIRECTORIES "${SUPPORT};${MERGED_DIR};${REPO}"
  OBJECT_DEPENDS ${MERGED})

target_compile_definitions(VirtualLinnStrument PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:VirtualLinnStrument,JUCE_PRODUCT_NAME>"
  JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:VirtualLinnStrument,JUCE_VERSION>")

target_link_libraries(VirtualLinnStrument PRIVATE
  VirtualLinnStrumentData
  juce::juce_gui_extra
  juce::juce_audio_devices
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)
