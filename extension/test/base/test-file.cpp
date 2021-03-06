////////////////////////////////////////////////////////////////////////////////
// Name:      test-file.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <wx/extension/file.h>
#include "../test.h"

TEST_CASE( "wxExFile" ) 
{
  SUBCASE( "basic" ) 
  {
    REQUIRE(!wxExFile("XXXXX").IsOpened());
    
    wxExFile file(GetTestPath("test.h"));
  
    REQUIRE(!file.CheckSync());
    REQUIRE(!file.GetContentsChanged());
    REQUIRE( file.IsOpened());
    
    file.ResetContentsChanged();

    REQUIRE(!file.FileSave());
    REQUIRE( file.GetFileName().GetStat().IsOk());
    // The fullpath should be normalized, test it.
    REQUIRE( file.GetFileName().Path().string() != "./test.h");
    REQUIRE(!file.GetFileName().GetStat().IsReadOnly());
    REQUIRE( file.FileLoad(GetTestPath("test.bin")));
    REQUIRE( file.Open(GetTestPath("test.bin").Path().string()));
    REQUIRE( file.IsOpened());

    const wxCharBuffer* buffer = file.Read();
    REQUIRE(buffer->length() == 40);
    
    file.FileNew("test-xxx");
    
    REQUIRE( file.Open(wxFile::write));
    REQUIRE( file.Write(*buffer));
    REQUIRE( file.Write(std::string("OK")));

    wxExFile create(std::string("test-create"), wxFile::write);
    REQUIRE( create.IsOpened());
    REQUIRE( create.Write(std::string("OK")));
  }

  // file should be closed before remove (at least for windows)
  SUBCASE( "remove")
  {
    REQUIRE( remove("test-create") == 0);
    REQUIRE( remove("test-xxx") == 0);
  }

  SUBCASE( "timing" ) 
  {
    wxExFile file(GetTestPath("test.h"));
  
    const int max = 10000;
    const auto ex_start = std::chrono::system_clock::now();
    
    for (int i = 0; i < max; i++)
    {
      REQUIRE(file.Read()->length() > 0);
    }

    const auto ex_milli = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ex_start);
    
    wxFile wxfile(GetTestPath("test.h").Path().string());
    const size_t l = wxfile.Length();
    const auto wx_start = std::chrono::system_clock::now();
    
    for (int j = 0; j < max; j++)
    {
      char* charbuffer = new char[l];
      wxfile.Read(charbuffer, l);
      REQUIRE(sizeof(charbuffer) > 0);
      delete[] charbuffer;
    }

    const auto wx_milli = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - wx_start);

    CHECK(ex_milli.count() < 2000);
    CHECK(wx_milli.count() < 2000);
  }
}
