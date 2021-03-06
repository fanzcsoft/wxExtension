////////////////////////////////////////////////////////////////////////////////
// Name:      test-dir.cpp
// Purpose:   Implementation for wxExtension report unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/extension/util.h>
#include <wx/extension/report/dir.h>
#include <wx/extension/report/listviewfile.h>
#include <wx/extension/report/stream.h>
#include "test.h"

TEST_CASE("wxExDirTool")
{
  const wxExTool tool = ID_TOOL_REPORT_FIND;

  wxExListView* report = new wxExListView(wxExListViewData().Type(LIST_FIND));
    
  if (!wxExStreamToListView::SetupTool(tool, GetFrame(), report))
  {
    return;
  }

  AddPane(GetFrame(), report);
  
  wxExDirTool dir(
    tool,
    "./",
    "*.cpp;*.h",
    DIR_FILES | DIR_HIDDEN | DIR_DIRS);

  dir.FindFiles();

  wxExLogStatus(tool.Info(&dir.GetStatistics().GetElements()));
}

TEST_CASE("wxExDirWithListView")
{
  wxExListViewFile* listView = new wxExListViewFile(GetProject());
  AddPane(GetFrame(), listView);
  wxExDirWithListView* dir = new wxExDirWithListView(listView, GetTestPath());
  REQUIRE(dir->FindFiles() == 0);
}
