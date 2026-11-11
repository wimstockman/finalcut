/***********************************************************************
* fpickedit.cpp - Widget FPickEdit (a text field with a pick button)   *
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

#include <algorithm>

#include "final/dialog/fdialog.h"
#include "final/widget/fbutton.h"
#include "final/widget/flabel.h"
#include "final/widget/flineedit.h"
#include "final/widget/flistbox.h"
#include "final/widget/fpickedit.h"

namespace finalcut
{

// fpickedit.cpp - A text field with a "..." pick button at its right end.


namespace
{

//----------------------------------------------------------------------
// The list of a PickChoiceDialog. A left click emits "choose" and a right
// click emits "cancel", after the list has handled the click. The mouse
// wheel moves the selection one row.
//----------------------------------------------------------------------
class PickChoiceList final : public finalcut::FListBox
{
  public:
    explicit PickChoiceList (finalcut::FWidget* parent) : FListBox{parent} {}

  protected:
    void onMouseDown (finalcut::FMouseEvent* ev) override
    {
      FListBox::onMouseDown (ev);

      if (ev->getButton() == finalcut::MouseButton::Left)
        emitCallback ("choose");
      else if (ev->getButton() == finalcut::MouseButton::Right)
        emitCallback ("cancel");
    }

    // Enter chooses the selected row, Escape cancels the dialog
    void onKeyPress (finalcut::FKeyEvent* ev) override
    {
      const auto key = ev->key();

      if (key == finalcut::FKey::Enter || key == finalcut::FKey::Return)
      {
        ev->accept();
        emitCallback ("choose");
        return;
      }

      if (key == finalcut::FKey::Escape)
      {
        ev->accept();
        emitCallback ("cancel");
        return;
      }

      FListBox::onKeyPress (ev);
    }

    void onWheel (finalcut::FWheelEvent* ev) override
    {
      const auto direction = ev->getWheel();
      const std::size_t count = getCount();

      if (count == 0 || (direction != finalcut::MouseWheel::Up
                      && direction != finalcut::MouseWheel::Down))
      {
        FListBox::onWheel (ev);
        return;
      }

      const std::size_t current = currentItem();  // rows are numbered from 1
      std::size_t target = current;

      if (direction == finalcut::MouseWheel::Up)
        target = (current > 1) ? current - 1 : 1;
      else
        target = std::min (current + 1, count);

      setCurrentItem (target);
      redraw();
    }
};

//----------------------------------------------------------------------
// A dialog with the choices of a FPickEdit. value() is the chosen value.
//----------------------------------------------------------------------
class PickChoiceDialog final : public finalcut::FDialog
{
  public:
    PickChoiceDialog (finalcut::FWidget* parent, const std::string& title,
                      const std::vector<FPickEdit::Choice>& choices,
                      const std::string& current)
      : FDialog{finalcut::FString{title.c_str()}, parent}
      , choices_{choices}
    {
      setModal();

      for (const auto& choice : choices_)
        list.insert (finalcut::FString{choice.label.c_str()});

      // The rows of a FListBox are numbered from 1
      for (std::size_t i = 0; i < choices_.size(); ++i)
      {
        if (choices_[i].value == current)
        {
          list.setCurrentItem (i + 1);
          break;
        }
      }

      btnOk.addCallback ("clicked", this, &PickChoiceDialog::cb_ok);
      btnCancel.addCallback ("clicked", this, &PickChoiceDialog::cb_cancel);
      list.addCallback ("choose", this, &PickChoiceDialog::cb_ok);
      list.addCallback ("cancel", this, &PickChoiceDialog::cb_cancel);
    }

    std::string value() const { return chosen; }

  private:
    void initLayout() override
    {
      FDialog::initLayout();
      setGeometry (finalcut::FPoint{20, 6}, finalcut::FSize{40, 16});
      list.setGeometry (finalcut::FPoint{2, 2}, finalcut::FSize{36, 10});
      btnOk.setGeometry (finalcut::FPoint{8, 13}, finalcut::FSize{10, 1});
      btnCancel.setGeometry (finalcut::FPoint{22, 13}, finalcut::FSize{10, 1});
      list.setFocus();  // the list gets the keyboard and the mouse first
    }

    void cb_ok()
    {
      const std::size_t row = list.currentItem();  // 1-based

      if (row >= 1 && row <= choices_.size())
        chosen = choices_[row - 1].value;

      done (ResultCode::Accept);
    }

    void cb_cancel() { done (ResultCode::Reject); }

    std::vector<FPickEdit::Choice> choices_;
    std::string chosen{};
    PickChoiceList list{this};
    finalcut::FButton btnOk{"&OK", this};
    finalcut::FButton btnCancel{"&Cancel", this};
};

}  // namespace

//----------------------------------------------------------------------
FPickEdit::FPickEdit (finalcut::FWidget* parent)
  : finalcut::FWidget{parent}
  , edit{parent}
  , pick_button{"...", parent}
  , shadow_row{parent}
  , shadow_end{parent}
{
  edit.unsetShadow();
  pick_button.unsetShadow();
  pick_button.addCallback ("clicked", this, &FPickEdit::cb_pick);
  edit.addCallback ("activate", this, &FPickEdit::cb_activate);
  shadow_end.setText (finalcut::FString{"\u2584"});  // ▄
}

//----------------------------------------------------------------------
FPickEdit::~FPickEdit() noexcept = default;

//----------------------------------------------------------------------
void FPickEdit::onFocusIn (FFocusEvent* ev)
{
  FWidget::onFocusIn (ev);
  // The composite has nothing to type into: the focus goes to the text field
  edit.setFocus();
}

//----------------------------------------------------------------------
void FPickEdit::setGeometry (const finalcut::FPoint& pos,
                            const finalcut::FSize& size, bool adjust)
{
  finalcut::FWidget::setGeometry (pos, size, adjust);
  placeChildren();
}

//----------------------------------------------------------------------
void FPickEdit::setSize (const finalcut::FSize& size, bool adjust)
{
  finalcut::FWidget::setSize (size, adjust);
  placeChildren();
}

//----------------------------------------------------------------------
void FPickEdit::setLabelText (const finalcut::FString& text)
{
  edit.setLabelText (text);
}

//----------------------------------------------------------------------
void FPickEdit::setText (const finalcut::FString& text)
{
  edit.setText (text);
  edit.redraw();
}

//----------------------------------------------------------------------
void FPickEdit::setChoices (const std::vector<Choice>& new_choices)
{
  choices = new_choices;
}


//----------------------------------------------------------------------
void FPickEdit::onMouseDown (finalcut::FMouseEvent* ev)
{
  // The field and the button lie over this widget, so a click on them
  // arrives here. Pass it on to the field or the button.
  const int x = ev->getX();

  if (x >= int(getWidth()) - int(BUTTON_WIDTH))
    cb_pick();
  else
  {
    edit.setFocus();
    edit.redraw();
  }
}

//----------------------------------------------------------------------
void FPickEdit::placeChildren()
{
  // The field and the button are siblings: they are placed at the position
  // of this widget in the parent
  const int x = getX();
  const int y = getY();
  const int width = int(getWidth());
  edit.setGeometry ( finalcut::FPoint{x, y}
                   , finalcut::FSize{std::size_t(width) - BUTTON_WIDTH, 1} );
  pick_button.setGeometry ( finalcut::FPoint{x + width - int(BUTTON_WIDTH), y}
                          , finalcut::FSize{BUTTON_WIDTH, 1} );

  // The shadow row under the field and the button, and the shadow at the
  // right end of the button, as the shadows of a field
  std::string row;
  for (int col = 0; col < width; col++)
    row += "\u2580";  // ▀
  shadow_row.setText (finalcut::FString{row.c_str()});
  shadow_row.setGeometry ( finalcut::FPoint{x + 1, y + 1}
                         , finalcut::FSize{std::size_t(width), 1} );
  shadow_end.setGeometry ( finalcut::FPoint{x + width, y}
                         , finalcut::FSize{1, 1} );
}

//----------------------------------------------------------------------
void FPickEdit::cb_pick()
{
  if (choices.empty())
  {
    // No choices: the user of the widget handles the pick
    emitCallback ("pick");
    return;
  }

  auto* dialog = new PickChoiceDialog{ getParentWidget(), "Choose a value"
                                     , choices, getText().toString() };
  const bool accepted = dialog->exec() == finalcut::FDialog::ResultCode::Accept;
  const std::string value = dialog->value();
  delete dialog;

  if (accepted)
  {
    setText (finalcut::FString{value.c_str()});
    emitCallback ("changed");
  }
}

//----------------------------------------------------------------------
void FPickEdit::cb_activate()
{
  emitCallback ("activate");
}

}  // namespace finalcut
