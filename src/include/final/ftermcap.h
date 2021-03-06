/***********************************************************************
* ftermcap.h - Provides access to terminal capabilities                *
*                                                                      *
* This file is part of the Final Cut widget toolkit                    *
*                                                                      *
* Copyright 2016-2019 Markus Gans                                      *
*                                                                      *
* The Final Cut is free software; you can redistribute it and/or       *
* modify it under the terms of the GNU Lesser General Public License   *
* as published by the Free Software Foundation; either version 3 of    *
* the License, or (at your option) any later version.                  *
*                                                                      *
* The Final Cut is distributed in the hope that it will be useful,     *
* but WITHOUT ANY WARRANTY; without even the implied warranty of       *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU Lesser General Public License for more details.                  *
*                                                                      *
* You should have received a copy of the GNU Lesser General Public     *
* License along with this program.  If not, see                        *
* <http://www.gnu.org/licenses/>.                                      *
***********************************************************************/

/*  Standalone class
 *  ════════════════
 *
 * ▕▔▔▔▔▔▔▔▔▔▔▏
 * ▕ FTermcap ▏
 * ▕▁▁▁▁▁▁▁▁▁▁▏
 */

#ifndef FTERMCAP_H
#define FTERMCAP_H

#if !defined (USE_FINAL_H) && !defined (COMPILE_FINAL_CUT)
  #error "Only <final/final.h> can be included directly."
#endif

#if defined(__sun) && defined(__SVR4)
  #include <termio.h>
  typedef struct termio SGTTY;
  typedef struct termios SGTTYS;

  #ifdef _LP64
    typedef unsigned int chtype;
  #else
    typedef unsigned long chtype;
  #endif  // _LP64

  #include <term.h>  // termcap
#else
  #include <term.h>  // termcap
#endif  // defined(__sun) && defined(__SVR4)

#ifdef F_HAVE_LIBGPM
  #undef buttons  // from term.h
#endif

#include <string>
#include <vector>

// FTermcap string macro
#define TCAP(...)  FTermcap::strings[__VA_ARGS__].string

namespace finalcut
{

// class forward declaration
class FTermData;
class FTermDetection;

//----------------------------------------------------------------------
// class FTermcap
//----------------------------------------------------------------------

class FTermcap final
{
  public:
    // Typedef
    typedef struct
    {
      char* string;
      char  tname[alignof(char*)];
    }
    tcap_map;

    // Constructors
    FTermcap() = default;

    // Destructor
    ~FTermcap() = default;

    // Accessors
    const FString    getClassName() const;

    // Methods
    static void init();

    // Data members
    static bool      background_color_erase;
    static bool      can_change_color_palette;
    static bool      automatic_left_margin;
    static bool      automatic_right_margin;
    static bool      eat_nl_glitch;
    static bool      ansi_default_color;
    static bool      osc_support;
    static bool      no_utf8_acs_chars;
    static int       max_color;
    static int       tabstop;
    static int       attr_without_color;
    static tcap_map  strings[];

  private:
    // Methods
    static void      termcap();
    static void      termcapError (int);
    static void      termcapVariables (char*&);
    static void      termcapBoleans();
    static void      termcapNumerics();
    static void      termcapStrings (char*&);
    static void      termcapKeys (char*&);
    static void      termcapKeysVt100 (char*&);

    // Data member
    static FTermData*      fterm_data;
    static FTermDetection* term_detection;
};


// FTermcap inline functions
//----------------------------------------------------------------------
inline const FString FTermcap::getClassName() const
{ return "FTermcap"; }

}  // namespace finalcut


#endif  // FTERMCAP_H
