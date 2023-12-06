/***************************** ls_debug: LinnStrument debug macros ********************************
Copyright 2023 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
***************************************************************************************************
Macros used for debugging output.  Note that you need to use two parenthesis to enclose the
argument - this allows debug code to be completely removed when the DEBUG_ENABLED is not defined.

Examples:

      DEBUGPRINT((-1,"This is ALWAYS printed"));
      DEBUGPRINT((0,"This is printed for debugLevel >= 0"));
      DEBUGPRINT((1,"This is printed for debugLevel >= 1"));

Normally, debug output is enabled but selectively controlled with debugLevel. Higher debugLevel
values enable more and more debugging output. Typically, level 0 is used for things that you're
currently debugging. After debugging something, either remove them or move them to higher levels.
**************************************************************************************************/

// #define DEBUG_ENABLED

#ifdef DEBUG_ENABLED

#define DEBUGPRINT(x) debugPrint x

#else

#define DEBUGPRINT(x)

#endif
