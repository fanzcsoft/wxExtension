////////////////////////////////////////////////////////////////////////////////
// Name:      test-vcsentry.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/vcsentry.h>
#include <wx/extension/managedframe.h>
#include <wx/extension/defs.h>
#include "test.h"

TEST_CASE("wxExVCSEntry")
{
  REQUIRE( wxExVCSEntry().GetCommands().size() == 1); // the empty command
  
  wxExVCSEntry test("my-vcs", "./",
    {wxExVCSCommand("one", "main"), wxExVCSCommand("two", "main")},
    wxExVCSEntry::VCS_FLAGS_LOCATION_POSTFIX);
  
  REQUIRE( test.GetCommands().size() == 2);
  REQUIRE(!test.GetCommand().GetCommand().empty());
  REQUIRE(!test.AdminDirIsTopLevel());
  REQUIRE( test.GetAdminDir() == "./");
  REQUIRE( test.GetBranch().empty());
  REQUIRE( test.GetFlags().empty());
  REQUIRE( test.GetName() == "my-vcs");
  REQUIRE( test.GetStdOut().empty());
  
  REQUIRE( wxExVCSEntry().GetFlagsLocation() == wxExVCSEntry::VCS_FLAGS_LOCATION_POSTFIX);
  
  test.ShowOutput();
  
  wxExMenu menu;
  REQUIRE( test.BuildMenu(0, &menu) == 0);

#ifndef __WXMSW__
  // This should have no effect.  
  REQUIRE(!test.SetCommand(5));
  REQUIRE(!test.SetCommand(ID_EDIT_VCS_LOWEST));
  REQUIRE(!test.SetCommand(ID_VCS_LOWEST));
  
  REQUIRE( test.GetCommands().size() == 2);
  REQUIRE( test.GetFlags().empty());
  REQUIRE( test.GetName() == "my-vcs");
  REQUIRE( test.GetStdOut().empty());
  REQUIRE(!test.Execute());
  
  wxExVCSEntry git("git");
  REQUIRE( git.Execute()); // executes just git, shows help
  REQUIRE( git.GetStdOut().find("usage: ") != std::string::npos);
  git.ShowOutput();

  wxExVCSEntry* git_async = new wxExVCSEntry("git", std::string(), {wxExVCSCommand("status")});
  REQUIRE( git_async->Execute(std::string(), wxExLexer(), PROCESS_EXEC_WAIT));
  git_async->ShowOutput();
#endif
}
