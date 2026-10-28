#!/bin/sh
# Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Run on every build: writes the commit the app was built from to OUT, rewriting the file only
# when it changes, so that an unchanged commit recompiles nothing.
execute_process(COMMAND git rev-parse --short=7 HEAD WORKING_DIRECTORY ${REPO}
  OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT commit)
  set(commit "unknown")
endif()
execute_process(COMMAND git status --porcelain --untracked-files=no -- ":(glob)*.ino" ":(glob)*.h"
  WORKING_DIRECTORY ${REPO} OUTPUT_VARIABLE changes ERROR_QUIET)
if(changes)
  string(APPEND commit "-modified")
endif()
set(text "#define VIRTUAL_COMMIT \"${commit}\"\n")
if(EXISTS ${OUT})
  file(READ ${OUT} old)
endif()
if(NOT "${old}" STREQUAL "${text}")
  file(WRITE ${OUT} "${text}")
endif()
