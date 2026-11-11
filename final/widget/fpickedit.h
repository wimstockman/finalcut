/***********************************************************************
* fpickedit.h - Widget FPickEdit (a text field with a pick button)     *
*                                                                      *
* This file is part of the FINAL CUT widget toolkit                    *
*                                                                      *
* Copyright 2026 Wim Stockman <wim@thinkerwim.org>                     *
*                                                                      *
* FINAL CUT is free software; you can redistribute it and/or modify    *
* it under the terms of the GNU Lesser General Public License as       *
* published by the Free Software Foundation; either version 3 of       *
* the License, or (at your option) any later version.                  *
*                                                                      *
* FINAL CUT is distributed in the hope that it will be useful, but     *
* WITHOUT ANY WARRANTY; without even the implied warranty of           *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU Lesser General Public License for more details.                  *
*                                                                      *
* You should have received a copy of the GNU Lesser General Public     *
* License along with this program.  If not, see                        *
* <http://www.gnu.org/licenses/>.                                      *
***********************************************************************/

/*  Inheritance diagram
 *  ═══════════════════
 *
 * ▕▔▔▔▔▔▔▔▔▔▏ ▕▔▔▔▔▔▔▔▔▔▏
 * ▕ FVTerm  ▏ ▕ FObject ▏
 * ▕▁▁▁▁▁▁▁▁▁▏ ▕▁▁▁▁▁▁▁▁▁▏
 *      ▲           ▲
 *      │           │
 *      └─────┬─────┘
 *            │
 *       ▕▔▔▔▔▔▔▔▔▔▏
 *       ▕ FWidget ▏
 *       ▕▁▁▁▁▁▁▁▁▁▏
 *            ▲
 *            │
 *      ▕▔▔▔▔▔▔▔▔▔▔▔▔▔▏
 *      ▕ FPickEdit   ▏
 *      ▕▁▁▁▁▁▁▁▁▁▁▁▁▁▏
 */

#ifndef FPICKEDIT_H
#define FPICKEDIT_H

#if !defined (USE_FINAL_H) && !defined (COMPILE_FINAL_CUT)
  #error "Only <final/final.h> can be included directly."
#endif

#include <string>
#include <vector>

#include "final/widget/fbutton.h"
#include "final/widget/flabel.h"
#include "final/widget/flineedit.h"

namespace finalcut
{

//----------------------------------------------------------------------
// class FPickEdit
//----------------------------------------------------------------------

class FPickEdit : public FWidget
{
  public:
    // A choice: the value goes in the field, the label is shown in the list
    struct Choice
    {
      std::string value;
      std::string label;
    };

    // Constructor
    explicit FPickEdit (FWidget* = nullptr);

    // Destructor
    ~FPickEdit() noexcept override;

    // Accessors
    auto getClassName() const -> FString override;
    auto getText() const -> FString;
    auto getLabelObject() const -> FLabel*;
    auto getPickButton() -> FButton&;

    // Mutators
    void setGeometry (const FPoint&, const FSize&, bool = true) override;
    void setSize (const FSize&, bool = true) override;
    void setLabelText (const FString&);
    void setText (const FString&);
    void setChoices (const std::vector<Choice>&);

  protected:
    void onFocusIn (FFocusEvent*) override;
    void onMouseDown (FMouseEvent*) override;

  private:
    // Constants
    static constexpr std::size_t BUTTON_WIDTH = 5;

    // Methods
    void placeChildren();
    void cb_pick();
    void cb_activate();

    // Data members
    std::vector<Choice> choices{};
    FLineEdit           edit;
    FButton             pick_button;
    FLabel              shadow_row;  // ▀ under the field and the button
    FLabel              shadow_end;  // ▄ at the right of the button
};


// FPickEdit inline functions
//----------------------------------------------------------------------
inline auto FPickEdit::getClassName() const -> FString
{ return "FPickEdit"; }

//----------------------------------------------------------------------
inline auto FPickEdit::getText() const -> FString
{ return edit.getText(); }

//----------------------------------------------------------------------
inline auto FPickEdit::getLabelObject() const -> FLabel*
{ return edit.getLabelObject(); }

//----------------------------------------------------------------------
inline auto FPickEdit::getPickButton() -> FButton&
{ return pick_button; }

}  // namespace finalcut

#endif  // FPICKEDIT_H
