/******************************************************************************\
* File:          lexers.cpp
* Purpose:       Implementation of exLexers classes
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 2008-2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/stc/stc.h>
#include <wx/textfile.h>
#include <wx/extension/lexers.h>
#include <wx/extension/util.h> // for exMatchesOneOf

exLexers::exLexers(const wxFileName& filename)
  : m_FileName(filename)
{
}
  
const exLexer exLexers::FindByFileName(const wxFileName& filename) const
{
  if (!filename.IsOk() || m_Lexers.empty())
  {
    return exLexer();
  }

  for (
    std::vector<exLexer>::const_iterator it = m_Lexers.begin();
    it != m_Lexers.end();
    ++it)
  {
    if (exMatchesOneOf(filename, it->GetAssociations()))
    {
      return *it;
    }
  }

  return exLexer();
}

const exLexer exLexers::FindByName(const wxString& name) const
{
  if (!m_Lexers.empty())
  {
    for (
      std::vector<exLexer>::const_iterator it = m_Lexers.begin();
      it != m_Lexers.end();
      ++it)
    {
      if (name == it->GetScintillaLexer())
      {
        return *it;
      }
    }

    // We did not find a lexer, so give an error.
    // The same error is shown in exSTC::SetLexer as well.
    wxLogError("Lexer is not known: " + name);
  }

  return exLexer();
}

const exLexer exLexers::FindByText(const wxString& text) const
{
  // Add automatic lexers if text starts with some special tokens.
  const wxString text_lowercase = text.Lower();

  if (text_lowercase.StartsWith("#") ||
      // .po files that do not have comment headers, start with msgid, so set them
      text_lowercase.StartsWith("msgid"))
  {
    return FindByName("bash");
  }
  else if (text_lowercase.StartsWith("<html>") ||
           text_lowercase.StartsWith("<?php") ||
           text_lowercase.StartsWith("<?xml"))
  {
    return FindByName("hypertext");
  }
  // cpp files like #include <map> really do not have a .h extension (e.g. /usr/include/c++/3.3.5/map)
  // so add here.
  else if (text_lowercase.StartsWith("//"))
  {
    return FindByName("cpp");
  }
  
  return exLexer();
}

// TODO: Styles and Styles hex parse them here instead of at stc.
const wxString exLexers::ParseTagColourings(const wxXmlNode* node) const
{
  wxString text;

  wxXmlNode* child = node->GetChildren();

  while (child) 
  {
    if (child->GetName() == "colouring")
    {
      text += 
        child->GetAttribute("no", "0") + "=" + 
        child->GetNodeContent().Strip(wxString::both) + wxTextFile::GetEOL();
    }
    else if (child->GetName() == "comment")
    { 
      // Ignore comments.
    }
    else
    {
      wxLogError("Undefined colourings tag: %s on: %d", 
        child->GetName().c_str(), child->GetLineNumber());
    }
    
    child = child->GetNext();
  }
  
  return text;
}

void exLexers::ParseTagGlobal(const wxXmlNode* node)
{
  wxXmlNode* child = node->GetChildren();

  while (child) 
  {
    if (child->GetName() == "comment")
    { 
      // Ignore comments.
    }
    else if (child->GetName() == "hex")
    {
      m_StylesHex.push_back(
        child->GetAttribute("no", "0") + "=" + 
        child->GetNodeContent().Strip(wxString::both));
    }
    else if (child->GetName() == "indicator")
    {
      m_Indicators.insert(std::make_pair(
        atoi(child->GetAttribute("no", "0").c_str()),
        atoi(child->GetNodeContent().Strip(wxString::both).c_str())));
    }
    else if (child->GetName() == "marker")
    {
      const exMarker marker(ParseTagMarker(
        child->GetAttribute("no", "0"),
        child->GetNodeContent().Strip(wxString::both)));

      if (marker.GetMarkerNumber() < wxSTC_STYLE_MAX &&
          marker.GetMarkerSymbol() < wxSTC_STYLE_MAX)
      {
        m_Markers.push_back(marker);
      }
      else
      {
        wxLogError("Illegal marker number: %d or symbol: %d on: %d",
          marker.GetMarkerNumber(),
          marker.GetMarkerSymbol(),
          child->GetLineNumber());
      }
    }
    else if (child->GetName() == "style")
    {
      m_Styles.push_back(
        child->GetAttribute("no", "0") + "=" + 
        child->GetNodeContent().Strip(wxString::both));
    }
    else
    {
      wxLogError("Undefined global tag: %s on: %d", 
        child->GetName().c_str(), child->GetLineNumber());
    }
    
    child = child->GetNext();
  }
}

const exLexer exLexers::ParseTagLexer(const wxXmlNode* node) const
{
  exLexer lexer;
  lexer.m_ScintillaLexer = node->GetAttribute("name", "cpp");
  lexer.m_Associations = node->GetAttribute("extensions", "*.cpp");

  wxXmlNode *child = node->GetChildren();

  while (child) 
  {
    if (child->GetName() == "colourings")
    {
      lexer.m_Colourings = ParseTagColourings(child);
    }
    else if (child->GetName() == "keywords")
    {
      if (!lexer.SetKeywords(child->GetNodeContent().Strip(wxString::both)))
      {
        wxLogError("Keywords could not be set on: %d", child->GetLineNumber());
      }
    }
    else if (child->GetName() == "properties")
    {
      lexer.m_Properties = ParseTagProperties(child);
    }
    else if (child->GetName() == "comments")
    {
      lexer.m_CommentBegin = child->GetAttribute("begin1", "");
      lexer.m_CommentEnd = child->GetAttribute("end1", "");
      lexer.m_CommentBegin2 = child->GetAttribute("begin2", "");
      lexer.m_CommentEnd2 = child->GetAttribute("end2", "\n");
    }
    else if (child->GetName() == "comment")
    { 
      // Ignore comments.
    }
    else
    {
      wxLogError("Undefined lexer tag: %s on: %d", 
        child->GetName().c_str(), child->GetLineNumber());
    }
    
    child = child->GetNext();
  }

  return lexer;
}

const exMarker exLexers::ParseTagMarker(
  const wxString& number, 
  const wxString& props) const
{
  wxStringTokenizer prop_fields(props, ",");

  const wxString symbol = prop_fields.GetNextToken();
  
  wxColour foreground;
  wxColour background;

  if (prop_fields.HasMoreTokens())
  {
    foreground = prop_fields.GetNextToken();

    if (prop_fields.HasMoreTokens())
    {
      background = prop_fields.GetNextToken();
    }
  }

  const int no = atoi(number.c_str());
  const int symbol_no = atoi(symbol.c_str());

  if (no <= wxSTC_MARKER_MAX && symbol_no <= wxSTC_MARKER_MAX)
  {
    return exMarker(no, symbol_no, foreground, background);
  }
  else
  {
    wxLogError("Illegal marker number: %d or symbol: %d", no, symbol_no);
    return exMarker(0, 0, foreground, background);
  }
}

const wxString exLexers::ParseTagProperties(const wxXmlNode* node) const
{
  wxString text;
  
  wxXmlNode *child = node->GetChildren();

  while (child) 
  {
    if (child->GetName() == "property")
    {
      text += 
        child->GetAttribute("name", "0") + "=" + 
        child->GetNodeContent().Strip(wxString::both) + wxTextFile::GetEOL();
    }
    else if (child->GetName() == "comment")
    { 
      // Ignore comments.
    }
    else
    {
      wxLogError("Undefined properties tag: %s on %d", 
        child->GetName().c_str(), child->GetLineNumber());
    }
    
    child = child->GetNext();
  }
  
  return text;
}

bool exLexers::Read()
{
  if (!m_FileName.FileExists()) 
  {
    return false;
  }

  wxXmlDocument doc;
  
  if (!doc.Load(m_FileName.GetFullPath()))
  {
    return false;
  }
 
  // Initialize members.
  m_Indicators.clear();
  m_Lexers.clear();
  m_Markers.clear();
  m_Styles.clear();
  m_StylesHex.clear();

  wxXmlNode* child = doc.GetRoot()->GetChildren();

  while (child) 
  {
    if (child->GetName() == "global") 
    {
      ParseTagGlobal(child);
    }
    else if (child->GetName() == "lexer") 
    {
      const exLexer& lexer = ParseTagLexer(child);

      if (!lexer.GetScintillaLexer().empty())
      {
        m_Lexers.push_back(lexer);
      }
    } 

    child = child->GetNext();
  }

  return true; 
}
