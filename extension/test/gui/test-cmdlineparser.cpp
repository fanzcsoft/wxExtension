////////////////////////////////////////////////////////////////////////////////
// Name:      test-cmdlineparser.cpp
// Purpose:   Implementation for wxExtension cpp unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/cmdline.h>
#include "test.h"

void fixture::testCmdLineParser()
{
  long a;
  double b;
  wxString c;
  wxDateTime d;
  bool s;
  bool t;
  bool u;
  wxString p,q,r;
  
  wxExCmdLineParser clp("-a 10 -b 10.1 -c test -d 01-jan-2000 -s -t- -u one two three", 
    wxExCmdLineParser::CmdSwitches { 
      {{"s", ""}, {wxCMD_LINE_SWITCH_NEGATABLE, [&](bool on){s = on;}}},
      {{"t", ""}, {wxCMD_LINE_SWITCH_NEGATABLE, [&](bool on){t = on;}}},
      {{"u", ""}, {0, [&](bool on){u = true;}}}},
    wxExCmdLineParser::CmdOptions {
      {{"a", ""}, {wxCMD_LINE_VAL_NUMBER, [&](wxAny any) {any.GetAs(&a);}}},
      {{"b", ""}, {wxCMD_LINE_VAL_DOUBLE, [&](wxAny any) {any.GetAs(&b);}}},
      {{"c", ""}, {wxCMD_LINE_VAL_STRING, [&](wxAny any) {any.GetAs(&c);}}},
      {{"d", ""}, {wxCMD_LINE_VAL_DATE, [&](wxAny any) {any.GetAs(&d);}}}},
    wxExCmdLineParser::CmdParams {
      {"p", {0, [&](std::vector<wxString> & v) {p = v[0];}}},
      {"q", {0, [&](std::vector<wxString> & v) {q = v[1];}}},
      {"r", {0, [&](std::vector<wxString> & v) {r = v[2];}}}});
    
  CPPUNIT_ASSERT( clp.Parse() == 0 );

  CPPUNIT_ASSERT( a == 10 );
  CPPUNIT_ASSERT( b == 10.1 );
  CPPUNIT_ASSERT( c == "test" );
  CPPUNIT_ASSERT( d.IsValid() );
  CPPUNIT_ASSERT( s );
  CPPUNIT_ASSERT(!t );
  CPPUNIT_ASSERT( u );
  CPPUNIT_ASSERT( p == "one" );
  CPPUNIT_ASSERT( q == "two" );
  CPPUNIT_ASSERT( r == "three" );
}
