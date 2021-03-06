////////////////////////////////////////////////////////////////////////////////
// Name:      test-statistics.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/statistics.h>
#include <wx/extension/managedframe.h>
#include "test.h"

TEST_CASE("wxExStatistics")
{
  wxExStatistics<int>* statistics1 = new wxExStatistics<int>;
  wxExStatistics<int>* statistics2 = new wxExStatistics<int>;
  
  REQUIRE(statistics1->Get().empty());
  REQUIRE(statistics1->GetItems().empty());
  REQUIRE(statistics1->Get("xx") == 0);
  
  REQUIRE(statistics2->Inc("xx") == 1);
  REQUIRE(statistics2->Set("xx", 3) == 3);
  REQUIRE(statistics2->Dec("xx") == 2);

  *statistics1 += *statistics2;
  
  REQUIRE(!statistics1->Get().empty());
  REQUIRE( statistics1->Get("xx") == 2);

  wxExGrid* grid1 = statistics1->Show(GetFrame());
  REQUIRE(grid1 != nullptr);
  REQUIRE(grid1 == statistics1->GetGrid());
  AddPane(GetFrame(), grid1);
  
  wxExGrid* grid2 = statistics2->Show(GetFrame());
  AddPane(GetFrame(), grid2);
  
  REQUIRE(statistics1->Show(GetFrame()) == grid1);
  REQUIRE(statistics2->Set("xx", 10) == 10);
  
  statistics2->Clear();
  REQUIRE(statistics2->Inc("xx") == 1);
  
  wxPostEvent(grid2, wxCommandEvent(wxEVT_MENU, wxID_CLEAR));
}
