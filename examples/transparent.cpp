/************************************************************************
* transparent.cpp - Demonstrates transparent windows                    *
*                                                                       *
* This file is part of the Final Cut widget toolkit                     *
*                                                                       *
* Copyright 2016-2017 Markus Gans                                       *
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

#include <final/fapplication.h>
#include <final/fdialog.h>
#include <final/flabel.h>
#include <final/fmessagebox.h>
#include <final/fstatusbar.h>
#include <final/fstring.h>


//----------------------------------------------------------------------
// class Transparent
//----------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)

class Transparent : public FDialog
{
  public:
    // Typedef and Enumeration
    typedef enum ttype
    {
      transparent        = 0,
      shadow             = 1,
      inherit_background = 2
    } trans_type;

  public:
    // Constructor
    explicit Transparent (FWidget* = 0, trans_type = transparent);

    // Destructor
    ~Transparent();

  private:
    // Disable copy constructor
    Transparent (const Transparent&);

    // Disable assignment operator (=)
    Transparent& operator = (const Transparent&);

    // Method
    void draw();

    // Event handlers
    void onKeyPress (FKeyEvent* ev);

    // Data Members
    trans_type type;
};
#pragma pack(pop)

//----------------------------------------------------------------------
Transparent::Transparent (FWidget* parent, Transparent::trans_type tt)
  : FDialog(parent)
  , type(tt)
{
  setStatusbarMessage("Press Q to quit");
}

//----------------------------------------------------------------------
Transparent::~Transparent()
{ }

//----------------------------------------------------------------------
void Transparent::draw()
{
  FDialog::draw();

  if ( isMonochron() )
    setReverse(true);

  if ( type == shadow )
  {
    setColor(wc.shadow_bg, wc.shadow_fg);
    setTransShadow();
  }
  else if ( type == inherit_background )
  {
    if ( getMaxColor() > 8 )
      setColor(fc::Blue, fc::Black);
    else
      setColor(fc::Green, fc::Black);

    setInheritBackground();
  }
  else
    setTransparent();

  FString line(getClientWidth(), wchar_t('.'));

  for (int n = 1; n <= getClientHeight(); n++)
  {
    setPrintPos (2, 2 + n);
    print(line);
  }

  if ( type == shadow )
    unsetTransShadow();
  else if ( type == inherit_background )
    unsetInheritBackground();
  else
    unsetTransparent();

  if ( isMonochron() )
    setReverse(false);
}

//----------------------------------------------------------------------
void Transparent::onKeyPress (FKeyEvent* ev)
{
  if ( ! ev )
    return;

  if ( ev->key() == 'q' && getParentWidget() )
  {
    if ( getParentWidget()->close() )
      ev->accept();
    else
      ev->ignore();
  }
  else
    FDialog::onKeyPress(ev);
}


//----------------------------------------------------------------------
// class MainWindow
//----------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)

class MainWindow : public FDialog
{
  private:
    FString line1;
    FString line2;

  private:
    // Disable copy constructor
    MainWindow (const MainWindow&);
    // Disable assignment operator (=)
    MainWindow& operator = (const MainWindow&);

    void draw();

    // Event handlers
    void onClose (FCloseEvent*);
    void onShow  (FShowEvent*);
    void onTimer (FTimerEvent*);
    void onKeyPress (FKeyEvent* ev)
    {
      if ( ! ev )
        return;

      if ( ev->key() == 'q' )
      {
        close();
        ev->accept();
      }
      else
        FDialog::onKeyPress(ev);
    }

  public:
    // Constructor
    explicit MainWindow (FWidget* = 0);
    // Destructor
    ~MainWindow();
};
#pragma pack(pop)

//----------------------------------------------------------------------
MainWindow::MainWindow (FWidget* parent)
  : FDialog(parent)
  , line1()
  , line2()
{
  line1 = "     .-.     .-.     .-.";
  line2 = "`._.'   `._.'   `._.'   ";

  Transparent* transpwin = new Transparent(this);
  transpwin->setText("transparent");
  transpwin->setGeometry (6, 3, 29, 12);
  transpwin->unsetTransparentShadow();

  Transparent* shadowwin = new Transparent(this, Transparent::shadow);
  shadowwin->setText("shadow");
  shadowwin->setGeometry (46, 11, 29, 12);
  shadowwin->unsetTransparentShadow();

  Transparent* ibg = new Transparent(this, Transparent::inherit_background);
  ibg->setText("inherit background");
  ibg->setGeometry (42, 3, 29, 7);
  ibg->unsetTransparentShadow();

  // Statusbar at the bottom
  FStatusBar* statusbar = new FStatusBar (this);
  statusbar->setMessage("Press Q to quit");

  addAccelerator('q');
  unsetTransparentShadow();
  activateDialog();
}

//----------------------------------------------------------------------
MainWindow::~MainWindow()
{ }

//----------------------------------------------------------------------
void MainWindow::draw()
{
  FDialog::draw();

  if ( isMonochron() )
    setReverse(true);

  setColor();
  setPrintPos (2, 4);
  print(line1);
  setPrintPos (2, 5);
  print(line2);

  if ( isMonochron() )
    setReverse(false);

  updateTerminal();
}

//----------------------------------------------------------------------
void MainWindow::onClose (FCloseEvent* ev)
{
  int ret = FMessageBox::info ( this, "Quit"
                              , "Do you really want\n"
                                "to quit the program ?"
                              , FMessageBox::Yes
                              , FMessageBox::No );
  if ( ret == FMessageBox::Yes )
    ev->accept();
  else
    ev->ignore();
}

//----------------------------------------------------------------------
void MainWindow::onShow (FShowEvent*)
{
  addTimer(100);
}

//----------------------------------------------------------------------
void MainWindow::onTimer (FTimerEvent*)
{
  wchar_t first_Char[2];
  uInt length = line1.getLength();
  first_Char[0] = line1[0];
  first_Char[1] = line2[0];
  line1 = line1.right(length - 1) + first_Char[0];
  line2 = line2.right(length - 1) + first_Char[1];
  redraw();
  flush_out();
}

//----------------------------------------------------------------------
//                               main part
//----------------------------------------------------------------------

int main (int argc, char* argv[])
{
  // Create the application object
  FApplication app (argc, argv);

  // Create main dialog object
  MainWindow main_dlg (&app);
  main_dlg.setText ("non-transparent");
  main_dlg.setGeometry (8, 16, 26, 7);

  // Set dialog main_dlg as main widget
  app.setMainWidget (&main_dlg);

  // Show and start the application
  main_dlg.show();
  return app.exec();
}
