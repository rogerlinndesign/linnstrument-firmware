/***************************** ls_debug: LinnStrument debug macros ********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
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

//#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED

#define DEBUGPRINT(x) debugPrint x

#else

#define DEBUGPRINT(x)

#endif
