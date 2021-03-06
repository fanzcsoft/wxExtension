////////////////////////////////////////////////////////////////////////////////
// Name:      lexer.cpp
// Purpose:   Implementation of wxExLexer class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <functional>
#include <numeric>
#include <pugixml.hpp>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/lexer.h>
#include <wx/extension/frame.h>
#include <wx/extension/lexers.h>
#include <wx/extension/log.h>
#include <wx/extension/stc.h>
#include <wx/extension/tokenizer.h>
#include <wx/extension/util.h> // for wxExAlignText
#include <easylogging++.h>

wxExLexer& wxExLexer::operator=(const wxExLexer& l)
{
  if (this != &l)
  {
    m_CommentBegin = l.m_CommentBegin;
    m_CommentBegin2 = l.m_CommentBegin2;
    m_CommentEnd = l.m_CommentEnd;
    m_CommentEnd2 = l.m_CommentEnd2;
    m_DisplayLexer = l.m_DisplayLexer;
    m_EdgeColumns = l.m_EdgeColumns;
    m_EdgeMode = l.m_EdgeMode;
    m_Extensions = l.m_Extensions;
    m_IsOk = l.m_IsOk;
    m_Keywords = l.m_Keywords;
    m_KeywordsSet = l.m_KeywordsSet;
    m_Language = l.m_Language;
    m_LineSize = l.m_LineSize;
    m_Previewable = l.m_Previewable;
    m_Properties = l.m_Properties;
    m_ScintillaLexer = l.m_ScintillaLexer;
    m_Styles = l.m_Styles;
    
    if (m_STC != nullptr && l.m_STC != nullptr)
    {
      m_STC = l.m_STC;
    }
  }

  return *this;
}
  
// Adds the specified keywords to the keywords map and the keywords set.
// The text might contain the keyword set after a ':'.
// Returns false if specified set is illegal or value is empty.
bool wxExLexer::AddKeywords(const std::string& value, int setno)
{
  if (value.empty() || setno < 0 || setno >= wxSTC_KEYWORDSET_MAX)
  {
    return false;
  }
  
  std::set<std::string> keywords_set;

  for (wxExTokenizer tkz(value, "\r\n "); tkz.HasMoreTokens(); )
  {
    const auto line(tkz.GetNextToken());
    std::string keyword;

    if (wxExTokenizer fields(line, ":"); fields.CountTokens() > 1)
    {
      keyword = fields.GetNextToken();

      try
      {
        if (const auto new_setno = std::stoi(fields.GetNextToken());
          new_setno <= 0 || new_setno >= wxSTC_KEYWORDSET_MAX)
        {
          wxExLog("invalid keyword set") << new_setno;
          return false;
        }
        else if (new_setno != setno)
        {
          if (!keywords_set.empty())
          {
            m_KeywordsSet.insert({setno, keywords_set});
            keywords_set.clear();
          }

          setno = new_setno;
        }
      }
      catch (std::exception& e)
      {
        wxExLog(e) << "keyword:" << keyword;
        return false;
      }
    }
    else
    {
      keyword = line;
    }

    keywords_set.insert(keyword);
    m_Keywords.insert(keyword);
  }

  if (const auto& it = m_KeywordsSet.find(setno); it == m_KeywordsSet.end())
  {
    m_KeywordsSet.insert({setno, keywords_set});
  }
  else
  {
    it->second.insert(keywords_set.begin(), keywords_set.end());
  }

  return true;
}

bool wxExLexer::Apply() const
{
  if (m_STC == nullptr) return false;

  m_STC->ClearDocumentStyle();

  for (const auto& it : m_Properties)
  {
    it.ApplyReset(m_STC);
  }

  // Reset keywords, also if no lexer is available.
  for (size_t setno = 0; setno < wxSTC_KEYWORDSET_MAX; setno++)
  {
    m_STC->SetKeyWords(setno, std::string());
  }

  wxExLexers::Get()->ApplyGlobalStyles(m_STC);

  if (wxExLexers::Get()->GetThemeOk())
  {
    for (const auto& k : m_KeywordsSet)
    {
      m_STC->SetKeyWords(k.first, wxExGetStringSet(k.second));
    }

    wxExLexers::Get()->Apply(m_STC);

    for (const auto& p : m_Properties) p.Apply(m_STC);
    for (const auto& s : m_Styles) s.Apply(m_STC);
  }

  // And finally colour the entire document.
  if (const auto length = m_STC->GetLength(); length > 0)
  {
    m_STC->Colourise(0, length - 1);
  }
  
  return true;
}

void wxExLexer::AutoMatch(const std::string& lexer)
{
  if (const auto& l(wxExLexers::Get()->FindByName(lexer));
    l.GetScintillaLexer().empty())
  {
    for (const auto& it : wxExLexers::Get()->GetMacros(lexer))
    {
      // First try exact match.
      if (const auto& macro = wxExLexers::Get()->GetThemeMacros().find(it.first);
        macro != wxExLexers::Get()->GetThemeMacros().end())
      {
        m_Styles.emplace_back(it.second, macro->second);
      }
      else
      {
        // Then, a partial using find_if.
        if (const auto& style = std::find_if(wxExLexers::Get()->GetThemeMacros().begin(), wxExLexers::Get()->GetThemeMacros().end(), 
          [&](auto const& e) {return it.first.find(e.first) != std::string::npos;});
          style != wxExLexers::Get()->GetThemeMacros().end())
          m_Styles.emplace_back(it.second, style->second);
      }
    }
  }
  else
  {
    // Copy styles and properties, and not keywords,
    // so your derived display lexer can have it's own keywords.
    m_Styles = l.m_Styles;
    m_Properties = l.m_Properties;
    
    m_CommentBegin = l.m_CommentBegin;
    m_CommentBegin2 = l.m_CommentBegin2;
    m_CommentEnd = l.m_CommentEnd;
    m_CommentEnd2 = l.m_CommentEnd2;
  }
}

const std::string wxExLexer::CommentComplete(const std::string& comment) const
{
  if (m_CommentEnd.empty()) return std::string();
  
  // Fill out rest of comment with spaces, and comment end string.
  if (const int n = m_LineSize - comment.size() - m_CommentEnd.size(); n <= 0) 
  {
    return std::string();
  }
  else
  {
    const auto blanks = std::string(n, ' ');
    return blanks + m_CommentEnd;
  }
}

const std::string wxExLexer::GetFormattedText(
  const std::string& lines,
  const std::string& header,
  bool fill_out_with_space,
  bool fill_out) const
{
  std::string_view text = lines, header_to_use = header;
  std::string out;

  // Process text between the carriage return line feeds.
  for (size_t nCharIndex; (nCharIndex = text.find("\n")) != std::string::npos; )
  {
    out += wxExAlignText(
      text.substr(0, nCharIndex),
      header_to_use,
      fill_out_with_space,
      fill_out,
      *this);

    text = text.substr(nCharIndex + 1);
    header_to_use = std::string(header.size(), ' ');
  }

  if (!text.empty())
  {
    out += wxExAlignText(
      text,
      header_to_use,
      fill_out_with_space,
      fill_out,
      *this);
  }

  return out;
}

const std::string wxExLexer::GetKeywordsString(
  int keyword_set, size_t min_size, const std::string& prefix) const
{
  if (keyword_set == -1)
  {
    return wxExGetStringSet(m_Keywords, min_size, prefix);
  }
  else
  {
    if (const auto& it = m_KeywordsSet.find(keyword_set);
      it != m_KeywordsSet.end())
    {
      return wxExGetStringSet(it->second, min_size, prefix);
    }
  }

  return std::string();
}

bool wxExLexer::IsKeyword(const std::string& word) const
{
  return m_Keywords.find(word) != m_Keywords.end();
}

bool wxExLexer::KeywordStartsWith(const std::string& word) const
{
  const auto& it = m_Keywords.lower_bound(word);
  return 
    it != m_Keywords.end() &&
    it->find(word) == 0;
}

const std::string wxExLexer::MakeComment(
  const std::string& text,
  bool fill_out_with_space,
  bool fill_out) const
{
  std::string out;

  text.find("\n") != std::string::npos ?
    out += GetFormattedText(text, std::string(), fill_out_with_space, fill_out):
    out += wxExAlignText(text, std::string(), fill_out_with_space, fill_out, *this);

  return out;
}

const std::string wxExLexer::MakeComment(
  const std::string& prefix,
  const std::string& text) const
{
  std::string out;

  text.find("\n") != std::string::npos ?
    out += GetFormattedText(text, prefix, true, true):
    out += wxExAlignText(text, prefix, true, true, *this);

  return out;
}

const std::string wxExLexer::MakeSingleLineComment(
  const std::string_view& text,
  bool fill_out_with_space,
  bool fill_out) const
{
  if (m_CommentBegin.empty() && m_CommentEnd.empty())
  {
    return std::string(text);
  }

  // First set the fill_out_character.
  char fill_out_character;

  if (fill_out_with_space || m_ScintillaLexer == "hypertext")
  {
    fill_out_character = ' ';
  }
  else
  {
    if (text.empty())
    {
      if (m_CommentBegin == m_CommentEnd)
           fill_out_character = '-';
      else fill_out_character = m_CommentBegin[m_CommentBegin.size() - 1];
    }
    else   fill_out_character = ' ';
  }

  std::string out = m_CommentBegin + fill_out_character + std::string(text);

  // Fill out characters (prevent filling out spaces)
  if (fill_out && 
      (fill_out_character != ' ' || !m_CommentEnd.empty()))
  {
    if (const auto fill_chars = UsableCharactersPerLine() - text.size(); fill_chars > 0)
    {
      out += std::string(fill_chars, fill_out_character);
    }
  }

  if (!m_CommentEnd.empty()) out += fill_out_character + m_CommentEnd;

  return out;
}

void wxExLexer::Reset()
{
  m_CommentBegin.clear();
  m_CommentBegin2.clear();
  m_CommentEnd.clear();
  m_CommentEnd2.clear();
  m_DisplayLexer.clear();
  m_EdgeColumns.clear();
  m_Extensions.clear();
  m_Keywords.clear();
  m_KeywordsSet.clear();
  m_Language.clear();
  m_Properties.clear();
  m_ScintillaLexer.clear();
  m_Styles.clear();

  m_IsOk = false;
  m_Previewable = false;

  m_EdgeMode = wxExEdgeMode::ABSENT;
  
  if (m_STC != nullptr)
  {
    ((wxStyledTextCtrl *)m_STC)->SetLexer(wxSTC_LEX_NULL);
    Apply();
    wxExFrame::StatusText(GetDisplayLexer(), "PaneLexer");
    m_STC->ResetMargins(STC_MARGIN_FOLDING);
  }
}

void wxExLexer::Set(const pugi::xml_node* node)
{
  m_ScintillaLexer = node->attribute("name").value();
  m_IsOk = !m_ScintillaLexer.empty();

  if (!m_IsOk)
  {
    wxExLog("missing lexer") << *node;
  }
  else
  {
    m_DisplayLexer = (!node->attribute("display").empty() ?
      node->attribute("display").value():
      m_ScintillaLexer);
    m_Extensions = node->attribute("extensions").value();
    m_Language = node->attribute("language").value();
    m_Previewable = !node->attribute("preview").empty();

    if (const std::string em(node->attribute("edgemode").value()); !em.empty())
    {
      if (em == "none")
        m_EdgeMode = wxExEdgeMode::NONE;
      else if (em == "line")
        m_EdgeMode = wxExEdgeMode::LINE;
      else if (em == "background")
        m_EdgeMode = wxExEdgeMode::BACKGROUND;
      else
        wxExLog("unsupported edge mode") << em << *node;
    }

    if (const std::string ec(node->attribute("edgecolumns").value());
      !ec.empty())
    {
      try
      {
        m_EdgeColumns = wxExTokenizer(ec).Tokenize();
      }
      catch (std::exception& e)
      {
        wxExLog(e) << "edgecolumns:" << ec;
      }
    }
 
    AutoMatch((!node->attribute("macro").empty() ?
      node->attribute("macro").value():
      m_ScintillaLexer));

    if (m_ScintillaLexer == "hypertext")
    {
      // As our lexers.xml files cannot use xml comments,
      // add them here.
      m_CommentBegin = "<!--";
      m_CommentEnd = "-->";
    }

    for (const auto& child: node->children())
    {
      if (strcmp(child.name(), "styles") == 0)
      {
        wxExNodeStyles(&child, m_ScintillaLexer, m_Styles);
      }
      else if (strcmp(child.name(), "keywords") == 0)
      {
        // Add all direct keywords
        if (const std::string& direct(child.text().get());
          !direct.empty() && !AddKeywords(direct))
        {
          wxExLog("keywords could not be set") << child;
        }

        // Add all keywords that point to a keyword set.
        for (const auto& att: child.attributes())
        {
          std::string nm(att.name());
          const auto pos = nm.find("-");
          try
          {
            const auto setno = (pos == std::string::npos ? 0: std::stoi(nm.substr(pos + 1)));
            const auto keywords = wxExLexers::Get()->GetKeywords(att.value());

            if (keywords.empty())
            {
              wxExLog("empty keywords for") << att.value() << child;
            }

            if (!AddKeywords(keywords, setno))
            {
              wxExLog("keywords for") << att.value() << "could not be set" << child;
            }
          }
          catch (std::exception& e)
          {
            wxExLog(e) << "keyword:" << att.name();
          }
        }
      }
      else if (strcmp(child.name(), "properties") == 0)
      {
        if (!m_Properties.empty())
        {
          wxExLog("properties already available") << child;
        }

        wxExNodeProperties(&child, m_Properties);
      }
      else if (strcmp(child.name(), "comments") == 0)
      {
        m_CommentBegin = child.attribute("begin1").value();
        m_CommentEnd = child.attribute("end1").value();
        m_CommentBegin2 = child.attribute("begin2").value();
        m_CommentEnd2 = child.attribute("end2").value();
      }
    }
  }
}

bool wxExLexer::Set(const std::string& lexer, bool fold)
{
  if (
    !Set(wxExLexers::Get()->FindByName(lexer), fold) &&
    !wxExLexers::Get()->GetLexers().empty() &&
    !lexer.empty())
  {
    VLOG(9) << "lexer is not known: " << lexer;
  }
  
  return m_IsOk;
}

bool wxExLexer::Set(const wxExLexer& lexer, bool fold)
{
  (*this) = (lexer.GetScintillaLexer().empty() && m_STC != nullptr ?
     wxExLexers::Get()->FindByText(m_STC->GetLine(0).ToStdString()): lexer);

  if (m_STC == nullptr) return m_IsOk;

  m_STC->SetLexerLanguage(m_ScintillaLexer);

  const bool ok = (((wxStyledTextCtrl *)m_STC)->GetLexer()) != wxSTC_LEX_NULL;

  Apply();

  // Set edges only if lexer is set.
  if (ok)
  {
    switch (m_EdgeMode)
    {
      case wxExEdgeMode::ABSENT: break;
        
      case wxExEdgeMode::BACKGROUND:
        m_STC->SetEdgeMode(wxSTC_EDGE_BACKGROUND); 
        break;
        
      case wxExEdgeMode::LINE:
#if wxCHECK_VERSION(3,1,1)
        m_STC->SetEdgeMode(m_EdgeColumns.size() <= 1 ? 
          wxSTC_EDGE_LINE: wxSTC_EDGE_MULTILINE); 
#else
        m_STC->SetEdgeMode(wxSTC_EDGE_LINE);
#endif
        break;
        
      case wxExEdgeMode::NONE:
        m_STC->SetEdgeMode(wxSTC_EDGE_NONE); 
        break;
    }
        
    switch (m_EdgeColumns.size())
    {
      case 0: break;

      case 1:
        m_STC->SetEdgeColumn(m_EdgeColumns.front());
        break;

#if wxCHECK_VERSION(3,1,1)
      default:
        for (const auto& c : m_EdgeColumns)
        {
          m_STC->MultiEdgeAddLine(c, m_STC->GetEdgeColour());
        }
#endif
    }
  }

  wxExFrame::StatusText(GetDisplayLexer(), "PaneLexer");

  if (fold)
  {
    m_STC->Fold();
  }

  return m_ScintillaLexer.empty() || ok;
}

void wxExLexer::SetProperty(const std::string& name, const std::string& value)
{
  if (const auto& it = std::find_if(m_Properties.begin(), m_Properties.end(), 
    [name](auto const& e) {return e.GetName() == name;});
    it != m_Properties.end()) it->Set(value);
  else m_Properties.emplace_back(name, value);
}

size_t wxExLexer::UsableCharactersPerLine() const
{
  // We adjust this here for
  // the space the beginning and end of the comment characters occupy.
  return m_LineSize
    - ((m_CommentBegin.size() != 0) ? m_CommentBegin.size() + 1 : 0)
    - ((m_CommentEnd.size() != 0) ? m_CommentEnd.size() + 1 : 0);
}
