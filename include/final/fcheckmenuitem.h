/************************************************************************
* fcheckmenuitem.h - Widget FCheckMenuItem                              *
*                                                                       *
* This file is part of the Final Cut widget toolkit                     *
*                                                                       *
* Copyright 2015-2017 Markus Gans                                       *
*                                                                       *
* The Final Cut is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by  *
* the Free Software Foundation; either version 3 of the License, or     *
* (at your option) any later version.                                   *
*                                                                       *
* The Final Cut is distributed in the hope that it will be useful,      *
* but WITHOUT ANY WARRANTY; without even the implied warranty of        *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
* GNU General Public License for more details.                          *
*                                                                       *
* You should have received a copy of the GNU General Public License     *
* along with this program.  If not, see <http://www.gnu.org/licenses/>. *
************************************************************************/

/*  Inheritance diagram
 *  ═══════════════════
 *
 * ▕▔▔▔▔▔▔▔▔▔▏ ▕▔▔▔▔▔▔▔▔▔▏
 * ▕ FObject ▏ ▕  FTerm  ▏
 * ▕▁▁▁▁▁▁▁▁▁▏ ▕▁▁▁▁▁▁▁▁▁▏
 *      ▲           ▲
 *      │           │
 *      └─────┬─────┘
 *            │
 *       ▕▔▔▔▔▔▔▔▔▏
 *       ▕ FVTerm ▏
 *       ▕▁▁▁▁▁▁▁▁▏
 *            ▲
 *            │
 *       ▕▔▔▔▔▔▔▔▔▔▏
 *       ▕ FWidget ▏
 *       ▕▁▁▁▁▁▁▁▁▁▏
 *            ▲
 *            │
 *      ▕▔▔▔▔▔▔▔▔▔▔▔▏
 *      ▕ FMenuItem ▏
 *      ▕▁▁▁▁▁▁▁▁▁▁▁▏
 *            ▲
 *            │
 *    ▕▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▏*       1▕▔▔▔▔▔▔▔▏
 *    ▕ FCheckMenuItem ▏- - - - -▕ FMenu ▏
 *    ▕▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▏         ▕▁▁▁▁▁▁▁▏
 */

#ifndef FCHECKMENUITEM_H
#define FCHECKMENUITEM_H

#include "final/fmenuitem.h"


//----------------------------------------------------------------------
// class FCheckMenuItem
//----------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)

class FCheckMenuItem : public FMenuItem
{
  public:
    // Constructors
    explicit FCheckMenuItem (FWidget* = 0);
    FCheckMenuItem (const FString&, FWidget* = 0);

    // Destructor
    virtual ~FCheckMenuItem();

    // Accessor
    const char* getClassName() const;

  private:
    // Disable copy constructor
    FCheckMenuItem (const FCheckMenuItem&);

    // Disable assignment operator (=)
    FCheckMenuItem& operator = (const FCheckMenuItem&);

    // Methods
    void init (FWidget*);
    void processToggle();
    void processClicked();
};
#pragma pack(pop)


// FCheckMenuItem inline functions
//----------------------------------------------------------------------
inline const char* FCheckMenuItem::getClassName() const
{ return "FCheckMenuItem"; }

#endif  // FCHECKMENUITEM_H
