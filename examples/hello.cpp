/************************************************************************
* hello.cpp - A simple hello world program                              *
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

#include <final/fapplication.h>
#include <final/fmessagebox.h>


int main (int argc, char* argv[])
{
  // Create the application object
  FApplication app(argc, argv);

  // Create a simple dialog box
  FMessageBox mbox(&app);
  mbox.setText("Hello world");

  // Start the application
  mbox.exec();
}
