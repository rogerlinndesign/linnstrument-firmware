# The firmware as the apps compile it: the sketch merged into one C++ file the way the Arduino
# build and the desktop tests do it, then COUNT copies of it with the Arduino stand-ins, each in a
# namespace of its own (Source/Firmware/FirmwareInstance.inc), so that each has its own state.
# Adds the MergedSketch target and sets FIRMWARE_SOURCES to the files a target compiles.
function(linn_firmware REPO COUNT)
  set(support ${REPO}/tests/support)
  set(bridge ${REPO}/tools/virtual/Source/Firmware)
  set(merged_dir ${CMAKE_CURRENT_BINARY_DIR}/sketch)
  set(merged ${merged_dir}/linnstrument-firmware.cpp)
  file(GLOB sketch CONFIGURE_DEPENDS ${REPO}/*.ino ${REPO}/*.h)
  add_custom_command(
    OUTPUT ${merged}
    COMMAND go run ./tools/linnstrument merge ${merged}
    WORKING_DIRECTORY ${REPO}
    DEPENDS ${sketch} ${REPO}/tools/linnstrument/test.go ${REPO}/tools/linnstrument/main.go
    COMMENT "Merging the sketch into one file")
  add_custom_target(MergedSketch DEPENDS ${merged})

  if(COUNT LESS 1)
    message(FATAL_ERROR "LINN_FIRMWARE_COPIES must be at least 1")
  endif()
  set(copies)
  math(EXPR last "${COUNT} - 1")
  foreach(i RANGE ${last})
    set(copy ${CMAKE_CURRENT_BINARY_DIR}/firmware/FirmwareInstance${i}.cpp)
    file(CONFIGURE OUTPUT ${copy} CONTENT "#define LINN_INSTANCE linnFirmware${i}\n#include \"FirmwareInstance.inc\"\n")
    list(APPEND copies ${copy})
  endforeach()

  # compiled as the Arduino build would, with its warnings silenced, and only these files see the
  # Arduino stand-ins, whose min and max macros must not reach JUCE
  set_source_files_properties(${copies} PROPERTIES
    COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>"
    INCLUDE_DIRECTORIES "${support};${merged_dir};${REPO};${bridge}"
    OBJECT_DEPENDS "${merged};${bridge}/FirmwareInstance.inc;${support}/arduino_mock.cpp")
  set(FIRMWARE_SOURCES ${copies} ${bridge}/FirmwareBridge.cpp PARENT_SCOPE)
endfunction()
