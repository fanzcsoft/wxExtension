////////////////////////////////////////////////////////////////////////////////
// Name:      test-lexers.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/lexers.h>
#include <wx/extension/managedframe.h>
#include <wx/extension/path.h>
#include <wx/extension/stc.h>
#include "test.h"

TEST_CASE("wxExLexers")
{
  REQUIRE( wxExLexers::Get() != nullptr);
  REQUIRE(!wxExLexers::Get()->GetLexers().empty());
  
  // Test lexer and global macros.
  for (const auto& macro : std::vector<
    std::pair<
      std::pair<std::string,std::string>,
      std::string>> {
    {{"number","asm"},"2"},
    {{"number","cpp"},"4"},
    {{"XXX","global"},"XXX"},
    {{"mark_lcorner","global"},"10"},
    {{"mark_circle","global"},"0"},
    {{"iv_none","global"},"0"}})
  {
    REQUIRE( wxExLexers::Get()->ApplyMacro(
      macro.first.first, macro.first.second) == macro.second);
  }

  // At this moment we have no global properties.
  REQUIRE( wxExLexers::Get()->GetProperties().empty());
  
  wxExLexers::Get()->Apply(GetSTC());

  REQUIRE(!wxExLexers::Get()->GetLexers().empty());

  REQUIRE( wxExLexers::Get()->FindByFileName(
    GetTestPath("test.h").GetFullName()).GetScintillaLexer() == "cpp");
    
  REQUIRE( wxExLexers::Get()->FindByName(
    "xxx").GetScintillaLexer().empty());
    
  REQUIRE( wxExLexers::Get()->FindByName(
    "cpp").GetScintillaLexer() == "cpp");
    
  for (const auto& findby : std::vector<std::pair<
    std::string, 
    std::pair<std::string, std::string>>> {
    {"// this is a cpp comment text",{"cpp","cpp"}},
    {"#!/bin/bash",{"bash","bash"}},
    {"#!/bin/bash\n",{"bash","bash"}},
    {"#!/usr/bin/csh",{"bash","bash"}},
    {"#!/bin/csh",{"bash","csh"}},
    {"#!/bin/env python",{"python","python"}},
    {"#!/bin/sh",{"bash","sh"}},
    {"#!/bin/tcsh",{"bash","tcsh"}},
    {"<html>",{"hypertext","hypertext"}},
    {"<?xml",{"hypertext","xml"}}})
  {
    REQUIRE( wxExLexers::Get()->FindByText(
      findby.first).GetScintillaLexer() == findby.second.first);
    REQUIRE( wxExLexers::Get()->FindByText(
      findby.first).GetDisplayLexer() == findby.second.second);
  }
    
  REQUIRE(!wxExLexers::Get()->GetFileName().Path().empty());

  REQUIRE(!wxExLexers::Get()->GetMacros("global").empty());
  REQUIRE(!wxExLexers::Get()->GetMacros("cpp").empty());
  REQUIRE(!wxExLexers::Get()->GetMacros("pascal").empty());
  REQUIRE( wxExLexers::Get()->GetMacros("XXX").empty());
  
  REQUIRE(!wxExLexers::Get()->GetTheme().empty());
  REQUIRE( wxExLexers::Get()->GetThemeOk());
  REQUIRE(!wxExLexers::Get()->GetThemeMacros().empty());
  REQUIRE( wxExLexers::Get()->GetThemes() > 1);

  wxExLexers::Get()->SetThemeNone();
  REQUIRE( wxExLexers::Get()->GetTheme().empty());
  REQUIRE(!wxExLexers::Get()->GetThemeOk());
  wxExLexers::Get()->RestoreTheme();
  REQUIRE( wxExLexers::Get()->GetTheme() == "studio");
  REQUIRE( wxExLexers::Get()->GetThemeOk());
  
  REQUIRE(!wxExLexers::Get()->IndicatorIsLoaded(wxExIndicator(99)));
  REQUIRE( wxExLexers::Get()->IndicatorIsLoaded(wxExIndicator(0)));
  REQUIRE( wxExLexers::Get()->MarkerIsLoaded(wxExMarker(0)));
  REQUIRE( wxExLexers::Get()->GetIndicator(wxExIndicator(0)).IsOk());
  REQUIRE( wxExLexers::Get()->GetMarker(wxExMarker(0)).IsOk());
  
  wxString lexer("cpp");
  REQUIRE(!wxExLexers::Get()->GetKeywords("cpp").empty());
  REQUIRE(!wxExLexers::Get()->GetKeywords("csh").empty());
  REQUIRE( wxExLexers::Get()->GetKeywords("xxx").empty());
  REQUIRE( wxExLexers::Get()->GetKeywords(std::string()).empty());

  REQUIRE( wxExLexers::Get()->LoadDocument());
  
  wxExLexers::Get()->ApplyGlobalStyles(GetSTC());
  wxExLexers::Get()->Apply(GetSTC());
}
