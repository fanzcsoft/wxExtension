////////////////////////////////////////////////////////////////////////////////
// Name:      test-ctags-entry`.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/ctags-entry.h>
#include "test.h"

TEST_CASE("wxExCTagsEntry")
{
  wxExCTagsEntry filter;

  REQUIRE(!filter.Active() );

  filter.Access("xx");
  REQUIRE( filter.Active() );
  REQUIRE( filter.Access() == "xx" );

  filter.Class("yy");
  REQUIRE( filter.Class() == "yy" );

  filter.Kind("f");
  REQUIRE( filter.Kind() == "f" );

  filter.Signature("zz");
  REQUIRE( filter.Signature() == "zz" );

  REQUIRE(!filter.Get().empty() );

  filter.Clear();
  REQUIRE(!filter.Active() );
  REQUIRE( filter.Get().empty() );
}
