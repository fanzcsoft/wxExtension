////////////////////////////////////////////////////////////////////////////////
// Name:      test-stream.cpp
// Purpose:   Implementation for wxExtension report unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/extension/frd.h>
#include <wx/extension/report/stream.h>
#include "test.h"

TEST_CASE("wxExStreamToListView")
{
  wxExTool tool(ID_TOOL_REPORT_FIND);

  wxExListView* report = new wxExListView(wxExListViewData().Type(LIST_FIND));
    
  AddPane(GetFrame(), report);

  wxExFindReplaceData::Get()->SetFindString("xx");
  
  REQUIRE(wxExStreamToListView::SetupTool(tool, GetFrame(), report));
  
  wxExStreamToListView textFile(GetTestPath("test.h"), tool);
  
  REQUIRE( textFile.RunTool());
  REQUIRE(!textFile.GetStatistics().GetElements().GetItems().empty());

  REQUIRE( textFile.RunTool()); // do the same test
  REQUIRE(!textFile.GetStatistics().GetElements().GetItems().empty());

  wxExStreamToListView textFile2(GetTestPath("test.h"), tool);
  REQUIRE( textFile2.RunTool());
  REQUIRE(!textFile2.GetStatistics().GetElements().GetItems().empty());
  
  wxExTool tool3(ID_TOOL_REPORT_KEYWORD);
  REQUIRE(wxExStreamToListView::SetupTool(tool3, GetFrame()));
  wxExStreamToListView textFile3(GetTestPath("test.h"), tool3);
  REQUIRE( textFile3.RunTool());
  REQUIRE(!textFile3.GetStatistics().GetElements().GetItems().empty());
}
