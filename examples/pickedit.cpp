/***********************************************************************
* pickedit.cpp - Example for using a FPickEdit widget                  *
*                                                                      *
* This file is part of the FINAL CUT widget toolkit                    *
*                                                                      *
* FINAL CUT is free software; you can redistribute it and/or modify    *
* it under the terms of the GNU Lesser General Public License as       *
* published by the Free Software Foundation; either version 3 of       *
* the License, or (at your option) any later version.                  *
***********************************************************************/

#include <final/final.h>

using finalcut::FApplication;
using finalcut::FPickEdit;
using finalcut::FButton;
using finalcut::FDialog;
using finalcut::FLabel;
using finalcut::FPoint;
using finalcut::FSize;
using finalcut::FString;
using finalcut::FWidget;

//----------------------------------------------------------------------
// callback functions
//----------------------------------------------------------------------

// The "changed" callback of a FPickEdit: a value was picked from its list
void cb_changed (FLabel& result, FPickEdit& field)
{
  result.setText (FString{"Chosen: "} + field.getText());
  result.redraw();
}

void cb_quit (const FApplication& app)
{
  app.quit();
}

//----------------------------------------------------------------------
//                               main part
//----------------------------------------------------------------------
auto main (int argc, char* argv[]) -> int
{
  FApplication app{argc, argv};

  FDialog dgl{&app};
  dgl.setText ("FPickEdit demo");
  dgl.setGeometry (FPoint{4, 2}, FSize{60, 12});

  FLabel result{"Use the ... button to pick a value", &dgl};
  result.setGeometry (FPoint{2, 8}, FSize{50, 1});

  FPickEdit ticker{&dgl};
  ticker.setLabelText ("Ticker:");
  ticker.setGeometry (FPoint{12, 2}, FSize{20, 1});
  ticker.setChoices ({ {"AED", "AED"}, {"ASC", "ASC"},
                       {"BERLIN", "BERLIN"}, {"XIOR", "XIOR"} });
  ticker.addCallback ("changed", &cb_changed, std::ref(result),
                      std::ref(ticker));

  FPickEdit action{&dgl};
  action.setLabelText ("Action:");
  action.setGeometry (FPoint{12, 4}, FSize{20, 1});
  action.setChoices ({ {"1", "1 - Buy"}, {"2", "2 - Sell"} });
  action.addCallback ("changed", &cb_changed, std::ref(result),
                      std::ref(action));

  FButton quit{"&Quit", &dgl};
  quit.setGeometry (FPoint{2, 10}, FSize{10, 1});
  quit.addCallback ("clicked", &cb_quit, std::ref(app));

  FWidget::setMainWidget (&dgl);
  dgl.show();
  return app.exec();
}
