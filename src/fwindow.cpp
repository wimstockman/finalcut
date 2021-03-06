/***********************************************************************
* fwindow.cpp - Intermediate base class for all window objects         *
*                                                                      *
* This file is part of the Final Cut widget toolkit                    *
*                                                                      *
* Copyright 2015-2020 Markus Gans                                      *
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

#include <utility>

#include "final/fapplication.h"
#include "final/fcombobox.h"
#include "final/fevent.h"
#include "final/fmenu.h"
#include "final/fmenubar.h"
#include "final/fstatusbar.h"
#include "final/fwindow.h"

namespace finalcut
{

// static attributes
FWindow* FWindow::previous_window{nullptr};


//----------------------------------------------------------------------
// class FWindow
//----------------------------------------------------------------------

// constructor and destructor
//----------------------------------------------------------------------
FWindow::FWindow(FWidget* parent)
  : FWidget(parent)
{
  setWindowWidget();
  FRect geometry (getTermGeometry());
  geometry.move(-1, -1);
  createArea (geometry, getShadow(), getVWin());
  addWindow (this);
}

//----------------------------------------------------------------------
FWindow::~FWindow()  // destructor
{
  const auto& fapp = FApplication::getApplicationObject();

  if ( previous_window == this )
    previous_window = nullptr;

  if ( isAlwaysOnTop() )
    deleteFromAlwaysOnTopList (this);

  // unset the global active window
  if ( this == FWindow::getActiveWindow() )
    unsetActiveWindow();

  delWindow (this);

  if ( ! fapp->isQuit() )
  {
    const auto& t_geometry = getTermGeometryWithShadow();
    restoreVTerm (t_geometry);
  }

  removeArea (getVWin());
}


// public methods of FWindow
//----------------------------------------------------------------------
FWidget* FWindow::getWindowFocusWidget() const
{
  // returns the focused widget of this window
  return win_focus_widget;
}

//----------------------------------------------------------------------
bool FWindow::setWindowWidget (bool enable)
{
  if ( isWindowWidget() == enable )
    return true;

  setFlags().window_widget = enable;

  if ( enable )
    setTermOffset();
  else
    setParentOffset();

  return enable;
}

//----------------------------------------------------------------------
void FWindow::setActiveWindow (FWindow* window)
{
  // activate FWindow object window

  if ( ! getWindowList() || getWindowList()->empty() )
    return;

  for (auto&& win : *getWindowList())
  {
    if ( win == window )
    {
      if ( ! window->isWindowActive() )
      {
        window->activateWindow();
        FEvent ev(fc::WindowActive_Event);
        FApplication::sendEvent(window, &ev);
      }
    }
    else
    {
      auto w = static_cast<FWindow*>(win);

      if ( w->isWindowActive() )
      {
        w->deactivateWindow();
        FEvent ev(fc::WindowInactive_Event);
        FApplication::sendEvent(win, &ev);
      }
    }
  }
}

//----------------------------------------------------------------------
void FWindow::setWindowFocusWidget (FWidget* obj)
{
  // set focus widget of this window
  win_focus_widget = obj;
}

//----------------------------------------------------------------------
bool FWindow::activateWindow (bool enable)
{
  // activate/deactivate this window
  if ( enable )
  {
    FWidget::setActiveWindow (this);
    setActiveArea (getVWin());
  }

  return (window_active = enable);
}

//----------------------------------------------------------------------
void FWindow::unsetActiveWindow()
{
  // unset the active FWindow object
  FWidget::setActiveWindow (nullptr);
}

//----------------------------------------------------------------------
bool FWindow::setResizeable (bool enable)
{
  return (setFlags().resizeable = enable);
}

//----------------------------------------------------------------------
bool FWindow::setTransparentShadow (bool enable)
{
  setFlags().shadow = setFlags().trans_shadow = enable;

  if ( enable )
    setShadowSize (FSize(2, 1));
  else
    setShadowSize (FSize(0, 0));

  return enable;
}

//----------------------------------------------------------------------
bool FWindow::setShadow (bool enable)
{
  if ( isMonochron() )
    return false;

  if ( enable )
  {
    setFlags().shadow = true;
    setFlags().trans_shadow = false;
    setShadowSize (FSize(1, 1));
  }
  else
  {
    setFlags().shadow = false;
    setFlags().trans_shadow = false;
    setShadowSize (FSize(0, 0));
  }

  return enable;
}

//----------------------------------------------------------------------
bool FWindow::setAlwaysOnTop (bool enable)
{
  if ( isAlwaysOnTop() == enable )
    return true;

  setFlags().always_on_top = enable;

  if ( enable )
  {
    if ( getAlwaysOnTopList() )
    {
      deleteFromAlwaysOnTopList (this);
      getAlwaysOnTopList()->push_back (this);
    }
  }
  else
    deleteFromAlwaysOnTopList (this);

  return enable;
}

//----------------------------------------------------------------------
bool FWindow::isWindowHidden() const
{
  // returns the window hidden state

  if ( isVirtualWindow() )
    return ! getVWin()->visible;
  else
    return false;
}

//----------------------------------------------------------------------
void FWindow::drawBorder()
{
  if ( isNewFont() )  // Draw a newfont outer frame
  {
    const FRect r(FPoint(1, 1), getSize());
    print() << r.getUpperLeftPos()
            << fc::NF_border_corner_upper_left                      // ⎡
            << FString(r.getWidth() - 2, fc::NF_border_line_upper)  // ¯
            << fc::NF_rev_border_corner_upper_right;                // ⎤

    for (int y = r.getY1() + 1; y < r.getY2(); y++)
    {
      print() << FPoint(r.getX1(), y)
              << fc::NF_border_line_left        // border left ⎸
              << FPoint(r.getX2(), y)
              << fc::NF_rev_border_line_right;  // border right⎹
    }

    print() << r.getLowerLeftPos()
            << fc::NF_border_corner_lower_left                       // ⎣
            << FString(r.getWidth() - 2, fc::NF_border_line_bottom)  // _
            << fc::NF_rev_border_corner_lower_right;                 // ⎦
  }
  else
  {
    FWidget::drawBorder();
  }
}

//----------------------------------------------------------------------
void FWindow::show()
{
  if ( isVirtualWindow() )
    getVWin()->visible = true;

  FWidget::show();
}

//----------------------------------------------------------------------
void FWindow::hide()
{
  if ( isVirtualWindow() )
    getVWin()->visible = false;

  FWidget::hide();
}

//----------------------------------------------------------------------
void FWindow::setX (int x, bool adjust)
{
  FWidget::setX (x, adjust);

  if ( isVirtualWindow() )
    getVWin()->offset_left = getTermX() - 1;
}

//----------------------------------------------------------------------
void FWindow::setY (int y, bool adjust)
{
  if ( y < 1 )
    y = 1;

  FWidget::setY (y, adjust);

  if ( isVirtualWindow() )
    getVWin()->offset_top = getTermY() - 1;
}

//----------------------------------------------------------------------
void FWindow::setPos (const FPoint& p, bool adjust)
{
  FPoint pos(p);

  if ( pos.getY() < 1 )
    pos.setY(1);

  FWidget::setPos (pos, adjust);

  if ( isVirtualWindow() )
  {
    auto virtual_win = getVWin();
    virtual_win->offset_left = getTermX() - 1;
    virtual_win->offset_top = getTermY() - 1;
  }
}

//----------------------------------------------------------------------
void FWindow::setWidth (std::size_t w, bool adjust)
{
  const std::size_t old_width = getWidth();
  FWidget::setWidth (w, adjust);

  if ( isVirtualWindow() && getWidth() != old_width )
  {
    FRect geometry (getTermGeometry());
    geometry.move(-1, -1);
    resizeArea (geometry, getShadow(), getVWin());
  }
}

//----------------------------------------------------------------------
void FWindow::setHeight (std::size_t h, bool adjust)
{
  const std::size_t old_height = getHeight();
  FWidget::setHeight (h, adjust);

  if ( isVirtualWindow() && getHeight() != old_height )
  {
    FRect geometry (getTermGeometry());
    geometry.move(-1, -1);
    resizeArea (geometry, getShadow(), getVWin());
  }
}

//----------------------------------------------------------------------
void FWindow::setSize (const FSize& size, bool adjust)
{
  const std::size_t old_width = getWidth();
  const std::size_t old_height = getHeight();
  FWidget::setSize (size, adjust);

  if ( isVirtualWindow()
    && (getWidth() != old_width || getHeight() != old_height) )
  {
    FRect geometry (getTermGeometry());
    geometry.move(-1, -1);
    resizeArea (geometry, getShadow(), getVWin());
  }
}

//----------------------------------------------------------------------
void FWindow::setGeometry ( const FPoint& p, const FSize& size, bool adjust)
{
  // Sets the geometry of the widget

  const int old_x = getX();
  const int old_y = getY();
  FPoint pos(p);
  const FSize old_size(getSize());

  if ( pos.getY() < 1 )
    pos.setY(1);

  FWidget::setGeometry (pos, size, adjust);

  if ( ! isVirtualWindow() )
    return;

  if ( getSize() != old_size )
  {
    FRect geometry (getTermGeometry());
    geometry.move(-1, -1);
    resizeArea (geometry, getShadow(), getVWin());
  }
  else
  {
    if ( getX() != old_x )
      getVWin()->offset_left = getTermX() - 1;

    if ( getY() != old_y )
      getVWin()->offset_top = getTermY() - 1;
  }
}

//----------------------------------------------------------------------
void FWindow::move (const FPoint& pos)
{
  FWidget::move(pos);

  if ( isVirtualWindow() )
  {
    auto virtual_win = getVWin();
    virtual_win->offset_left = getTermX() - 1;
    virtual_win->offset_top = getTermY() - 1;
  }
}

//----------------------------------------------------------------------
FWindow* FWindow::getWindowWidgetAt (int x, int y)
{
  // returns the window object to the corresponding coordinates
  if ( getWindowList() && ! getWindowList()->empty() )
  {
    auto iter = getWindowList()->end();
    const auto begin = getWindowList()->begin();

    do
    {
      --iter;
      if ( *iter )
      {
        auto w = static_cast<FWindow*>(*iter);

        if ( ! w->isWindowHidden()
          && w->getTermGeometry().contains(x, y) )
          return w;
      }
    }
    while ( iter != begin );
  }

  return 0;
}

//----------------------------------------------------------------------
void FWindow::addWindow (FWidget* obj)
{
  // add the window object obj to the window list
  if ( getWindowList() )
    getWindowList()->push_back(obj);

  processAlwaysOnTop();
}

//----------------------------------------------------------------------
void FWindow::delWindow (FWidget* obj)
{
  // delete the window object obj from the window list
  if ( ! getWindowList() || getWindowList()->empty() )
    return;

  auto iter = getWindowList()->begin();

  while ( iter != getWindowList()->end() )
  {
    if ( (*iter) == obj )
    {
      getWindowList()->erase(iter);
      return;
    }

    ++iter;
  }
}

//----------------------------------------------------------------------
FWindow* FWindow::getWindowWidget (const FWidget* obj)
{
  // returns the window object to the given widget obj
  auto p_obj = obj->getParentWidget();

  while ( ! obj->isWindowWidget() && p_obj )
  {
    obj = p_obj;
    p_obj = p_obj->getParentWidget();
  }

  if ( obj->isWindowWidget() )
    return const_cast<FWindow*>(reinterpret_cast<const FWindow*>(obj));
  else
    return 0;
}

//----------------------------------------------------------------------
int FWindow::getWindowLayer (const FWidget* obj)
{
  // returns the window layer from the widget obj

  const FWidget* window;

  if ( ! getWindowList() )
    return -1;

  if ( getWindowList()->empty() )
    return -1;

  if ( ! obj->isWindowWidget() )
  {
    if ( (window = getWindowWidget(obj)) == 0 )
      return -1;
  }
  else
    window = obj;

  auto iter = getWindowList()->begin();
  const auto end  = getWindowList()->end();

  while ( iter != end )
  {
    if ( *iter == window )
      break;

    ++iter;
  }

  return int(std::distance(getWindowList()->begin(), iter) + 1);
}

//----------------------------------------------------------------------
void FWindow::swapWindow (FWidget* obj1, FWidget* obj2)
{
  // swaps the window layer between obj1 and obj2

  if ( ! getWindowList() )
    return;

  if ( getWindowList()->empty() )
    return;

  if ( obj1->getFlags().modal )
    return;

  if ( obj2->getFlags().modal )
    return;

  auto iter = getWindowList()->begin();
  const auto end = getWindowList()->end();
  auto iter1 = end;
  auto iter2 = end;

  while ( iter != end )
  {
    if ( (*iter) == obj1 )
      iter1 = iter;
    else if ( (*iter) == obj2 )
      iter2 = iter;

    ++iter;
  }

  if ( iter1 != end && iter2 != end )
    std::swap (iter1, iter2);
}

//----------------------------------------------------------------------
bool FWindow::raiseWindow (FWidget* obj)
{
  // raises the window widget obj to the top

  if ( ! getWindowList() )
    return false;

  if ( getWindowList()->empty() )
    return false;

  if ( ! obj->isWindowWidget() )
    return false;

  if ( getWindowList()->back() == obj )
    return false;

  if ( getWindowList()->back()->getFlags().modal
    && ! obj->isMenuWidget() )
    return false;

  auto iter = getWindowList()->begin();

  while ( iter != getWindowList()->end() )
  {
    if ( *iter == obj )
    {
      getWindowList()->erase (iter);
      getWindowList()->push_back (obj);
      FEvent ev(fc::WindowRaised_Event);
      FApplication::sendEvent(obj, &ev);
      processAlwaysOnTop();
      return true;
    }

    ++iter;
  }

  return false;
}

//----------------------------------------------------------------------
bool FWindow::lowerWindow (FWidget* obj)
{
  // lowers the window widget obj to the bottom

  if ( ! getWindowList() )
    return false;

  if ( getWindowList()->empty() )
    return false;

  if ( ! obj->isWindowWidget() )
    return false;

  if ( getWindowList()->front() == obj )
    return false;

  if ( obj->getFlags().modal )
    return false;

  auto iter = getWindowList()->begin();

  while ( iter != getWindowList()->end() )
  {
    if ( *iter == obj )
    {
      getWindowList()->erase (iter);
      getWindowList()->insert (getWindowList()->begin(), obj);
      FEvent ev(fc::WindowLowered_Event);
      FApplication::sendEvent(obj, &ev);
      return true;
    }

    ++iter;
  }

  return false;
}

//----------------------------------------------------------------------
bool FWindow::zoomWindow()
{
  if ( zoomed )
  {
    zoomed = false;
    const FRect oldGeometry (getTermGeometryWithShadow());
    setGeometry (normalGeometry);
    restoreVTerm (oldGeometry);
    redraw();
  }
  else
  {
    zoomed = true;
    // save the current geometry
    normalGeometry = getGeometry();
    const FRect oldGeometry (getTermGeometryWithShadow());
    setGeometry (FPoint(1, 1), FSize(getMaxWidth(), getMaxHeight()));
    restoreVTerm (oldGeometry);
    redraw();
  }

  return zoomed;
}

//----------------------------------------------------------------------
void FWindow::switchToPrevWindow (FWidget* widget)
{
  // switch to previous window

  // Disable terminal updates to avoid flickering
  // when redrawing the focused widget
  if ( widget )
    widget->updateTerminal (FVTerm::stop_refresh);

  const bool is_activated = activatePrevWindow();
  auto active_win = static_cast<FWindow*>(getActiveWindow());

  if ( ! is_activated )
  {
    // no previous window -> looking for another window
    if ( getWindowList() && getWindowList()->size() > 1 )
    {
      auto iter = getWindowList()->end();
      const auto begin = getWindowList()->begin();

      do
      {
        --iter;
        auto w = static_cast<FWindow*>(*iter);

        if ( w
          && w != active_win
          && ! (w->isWindowHidden() || w->isWindowActive())
          && w != static_cast<FWindow*>(getStatusBar())
          && w != static_cast<FWindow*>(getMenuBar()) )
        {
          setActiveWindow(w);
          break;
        }
      }
      while ( iter != begin );
    }
  }

  if ( active_win )
  {
    auto focus = active_win->getWindowFocusWidget();

    if ( ! active_win->isWindowActive() )
      setActiveWindow(active_win);

    if ( focus )
    {
      focus->setFocus();

      if ( ! focus->isWindowWidget() )
        focus->redraw();
    }
  }

  // Enable terminal updates again
  if ( widget )
    widget->updateTerminal (FVTerm::continue_refresh);
}

//----------------------------------------------------------------------
bool FWindow::activatePrevWindow()
{
  // activate the previous window
  const auto& w = previous_window;

  if ( w )
  {
    if ( w->isWindowActive() )
      return true;

    if ( ! w->isWindowHidden() )
    {
      setActiveWindow(w);
      return true;
    }
  }

  return false;
}

//----------------------------------------------------------------------
void FWindow::setShadowSize (const FSize& size)
{
  const FSize& old_shadow = getShadow();
  FWidget::setShadowSize (size);
  const FSize& new_shadow = getShadow();

  if ( isVirtualWindow() && (new_shadow != old_shadow) )
  {
    auto geometry = getTermGeometry();
    geometry.move(-1, -1);
    resizeArea (geometry, getShadow(), getVWin());
  }
}


// protected methods of FWindow
//----------------------------------------------------------------------
void FWindow::adjustSize()
{
  const int old_x = getTermX();
  const int old_y = getTermY();
  FWidget::adjustSize();

  if ( zoomed )
    setGeometry (FPoint(1, 1), FSize(getMaxWidth(), getMaxHeight()), false);
  else if ( isVirtualWindow() )
  {
    if ( getTermX() != old_x )
      getVWin()->offset_left = getTermX() - 1;

    if ( getTermY() != old_y )
      getVWin()->offset_top = getTermY() - 1;
  }
}

//----------------------------------------------------------------------
bool FWindow::event (FEvent* ev)
{
  switch ( uInt(ev->type()) )
  {
    case fc::WindowActive_Event:
      onWindowActive (ev);
      break;

    case fc::WindowInactive_Event:
      onWindowInactive (ev);
      break;

    case fc::WindowRaised_Event:
      onWindowRaised (ev);
      break;

    case fc::WindowLowered_Event:
      onWindowLowered (ev);
      break;

    default:
      return FWidget::event(ev);
  }

  return true;
}

//----------------------------------------------------------------------
void FWindow::onWindowActive (FEvent*)
{ }

//----------------------------------------------------------------------
void FWindow::onWindowInactive (FEvent*)
{ }

//----------------------------------------------------------------------
void FWindow::onWindowRaised (FEvent*)
{ }

//----------------------------------------------------------------------
void FWindow::onWindowLowered (FEvent*)
{ }


// private methods of FWindow
//----------------------------------------------------------------------
void FWindow::deleteFromAlwaysOnTopList (FWidget* obj)
{
  // delete the window object obj from the always-on-top list
  if ( ! getAlwaysOnTopList() || getAlwaysOnTopList()->empty() )
    return;

  auto iter = getAlwaysOnTopList()->begin();

  while ( iter != getAlwaysOnTopList()->end() )
  {
    if ( *iter == obj )
    {
      getAlwaysOnTopList()->erase (iter);
      return;
    }

    ++iter;
  }
}

//----------------------------------------------------------------------
void FWindow::processAlwaysOnTop()
{
  // Raise all always-on-top windows
  if ( ! getAlwaysOnTopList() || getAlwaysOnTopList()->empty() )
    return;

  auto iter = getAlwaysOnTopList()->begin();

  while ( iter != getAlwaysOnTopList()->end() )
  {
    delWindow (*iter);

    if ( getWindowList() )
      getWindowList()->push_back(*iter);

    ++iter;
  }
}

// non-member functions
//----------------------------------------------------------------------
void closeDropDown (FWidget* widget, const FPoint& mouse_position)
{
  // Close the pop down windows

  bool is_dialog_menu{false};
  auto openmenu = FWidget::getOpenMenu();

  if ( ! openmenu )
   return;

  if ( openmenu->isInstanceOf("FMenu") )
  {
    bool contains_menu_structure;
    auto menu = static_cast<FMenu*>(openmenu);
    std::tie(contains_menu_structure, is_dialog_menu) = \
        closeOpenMenus (menu, mouse_position);

    if ( contains_menu_structure )
      return;
  }

  if ( openmenu->isInstanceOf("FDropDownListBox") )
  {
    auto drop_down = static_cast<FDropDownListBox*>(openmenu);

    if ( ! closeComboBox(drop_down, mouse_position) )
      return;
  }

  // No widget was been clicked and the menu is no dialog menu
  if ( ! (FWidget::getClickedWidget() || is_dialog_menu) )
    FWindow::switchToPrevWindow(widget);

  if ( FWidget::getStatusBar() )
    FWidget::getStatusBar()->drawMessage();

  widget->updateTerminal();
  FVTerm::flush();
}

}  // namespace finalcut
