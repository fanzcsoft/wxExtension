////////////////////////////////////////////////////////////////////////////////
// Name:      util.cpp
// Purpose:   Implementation of wxExtension utility methods
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <numeric>
#include <pugixml.hpp>
#include <regex>
#include <wx/app.h>
#include <wx/clipbrd.h>
#include <wx/config.h>
#include <wx/generic/dirctrlg.h> // for wxTheFileIconsTable
#include <wx/stdpaths.h>
#include <wx/wupdlock.h>
#include <wx/extension/util.h>
#include <wx/extension/dir.h>
#include <wx/extension/ex.h>
#include <wx/extension/filedlg.h>
#include <wx/extension/frame.h>
#include <wx/extension/frd.h>
#include <wx/extension/lexer.h>
#include <wx/extension/lexers.h>
#include <wx/extension/log.h>
#include <wx/extension/managedframe.h>
#include <wx/extension/path.h>
#include <wx/extension/process.h>
#include <wx/extension/stc.h>
#include <wx/extension/tokenizer.h>
#include <wx/extension/tostring.h>
#include <wx/extension/vcs.h>
#include <wx/extension/vi-macros.h>
#include <easylogging++.h>

const std::string wxExAfter(
  const std::string& text, char c, bool first)
{
  const auto pos = (first ? text.find(c): text.rfind(c));
  return
    (pos == std::string::npos ? text: text.substr(pos + 1));
}

const std::string wxExAlignText(
  const std::string_view& lines, const std::string_view& header,
  bool fill_out_with_space, bool fill_out, const wxExLexer& lexer)
{
  const auto line_length = lexer.UsableCharactersPerLine();

  // Use the header, with one space extra to separate, or no header at all.
  const auto header_with_spaces =
    (header.empty()) ? std::string() : std::string(header.size(), ' ');

  std::string in(lines);
  std::string line(header);

  bool at_begin = true;
  std::string out;

  while (!in.empty())
  {
    if (const auto word = wxExGetWord(in, false, false);
      line.size() + 1 + word.size() > line_length)
    {
      out += lexer.MakeSingleLineComment(line, fill_out_with_space, fill_out) + "\n";
      line = header_with_spaces + word;
    }
    else
    {
      line += (!line.empty() && !at_begin ? std::string(" "): std::string()) + word;
      at_begin = false;
    }
  }

  out += lexer.MakeSingleLineComment(line, fill_out_with_space, fill_out);

  return out;
}

std::tuple<bool, const std::string, const std::vector<std::string>> 
  wxExAutoCompleteFileName(const std::string& text)
{
  // E.g.:
  // 1) text: src/vi
  // -> should build vector with files in ./src starting with vi
  // path:   src
  // prefix: vi
  // 2) text: /usr/include/s
  // ->should build vector with files in /usr/include starting with s
  // path:   /usr/include
  // prefix: s
  // And text might be prefixed by a command, e.g.: e src/vi
  wxExPath path(wxExAfter(text, ' ', false));
  
  if (path.IsRelative())
  {
    path.MakeAbsolute();
  }

  const auto prefix(path.GetFullName());
  const std::vector <std::string > v(wxExGetAllFiles(path.GetPath(), prefix + "*"));

  if (v.empty())
  {
    return {false, std::string(), v};
  }

  auto rest_equal_size = std::string::npos;

  if (v.size() > 1)
  {
    bool all_ok = true;
    rest_equal_size = 0;
      
    for (auto i = prefix.length(); i < v[0].size() && all_ok; i++)
    {
      for (size_t j = 1; j < v.size() && all_ok; j++)
      {
        if (i < v[j].size() && v[0][i] != v[j][i])
        {
          all_ok = false;
        }
      }
    
      if (all_ok)
      {
        rest_equal_size++;
      }
    }
  }

  return {true, v[0].substr(prefix.length(), rest_equal_size), v};
}

bool wxExAutoCompleteText(const std::string& text, 
  const std::vector<std::string> & v, std::string& s)
{
  int matches = 0;
  
  for (const auto& it : v)
  {
    if (it.find(text) == 0)
    {
      s = it;
      matches++;
    }
  }

  return (matches == 1);
}

const std::string wxExBefore(
  const std::string& text, char c, bool first)
{
  if (const auto pos = (first ? text.find(c): text.rfind(c));
    pos != std::string::npos)
  {
    return text.substr(0, pos);
  }
  else
  {
    return text;
  }
}  

bool wxExBrowserSearch(const std::string& text)
{
  if (const auto search_engine(wxExConfigFirstOf(_("Search engine")));
    search_engine.empty())
  {
    return false;
  }
  else 
  {
    wxLaunchDefaultBrowser(search_engine + "?q=" + text);
    return true;
  }
}

bool wxExClipboardAdd(const std::string& text)
{
  if (wxClipboardLocker locker; !locker)
  {
    return false;
  }
  else
  {
    if (wxTheClipboard->AddData(new wxTextDataObject(text)))
    {
      // Take care that clipboard data remain after exiting
      // This is a boolean method as well, we don't check it, as
      // clipboard data is copied.
      // At least on Ubuntu 8.10 FLush returns false.
      wxTheClipboard->Flush();
    }
  }

  return true;
}

const std::string wxExClipboardGet()
{
  if (wxClipboardLocker locker; !locker)
  {
    return std::string();
  }
  else if (wxTheClipboard->IsSupported(wxDF_TEXT))
  {
    if (wxTextDataObject data; wxTheClipboard->GetData(data))
    {
      return data.GetText().ToStdString();
    }
  }

  return std::string();
}

void wxExComboBoxFromList(wxComboBox* cb, const std::list < std::string > & text)
{
  wxExComboBoxAs<const std::list < std::string >>(cb, text);
}

bool wxExCompareFile(const wxExPath& file1, const wxExPath& file2)
{
  if (wxConfigBase::Get()->Read(_("Comparator")).empty())
  {
    return false;
  }

  const auto arguments =
     (file1.GetStat().st_mtime < file2.GetStat().st_mtime) ?
       "\"" + file1.Path().string() + "\" \"" + file2.Path().string() + "\"":
       "\"" + file2.Path().string() + "\" \"" + file1.Path().string() + "\"";

  if (!wxExProcess().Execute(
    wxConfigBase::Get()->Read(_("Comparator")).ToStdString() + " " + arguments, 
    PROCESS_EXEC_WAIT))
  {
    return false;
  }

  wxLogStatus(_("Compared") + ": " + arguments);

  return true;
}

const std::string wxExConfigDir()
{
#ifdef __WXMSW__
  return wxPathOnly(wxStandardPaths::Get().GetExecutablePath()).ToStdString();
#else
  return wxExPath({
    wxGetHomeDir().ToStdString(), 
    ".config", 
    wxTheApp->GetAppName().Lower().ToStdString()}).Path().string();
#endif
}
  
const std::string wxExConfigFirstOf(const wxString& key)
{
  return 
    wxConfigBase::Get()->Read(key).BeforeFirst(wxExGetFieldSeparator()).ToStdString();
}

const std::string wxExConfigFirstOfWrite(const wxString& key, const wxString& value)
{
  std::vector<wxString> v{value};

  for (wxExTokenizer tkz(wxConfigBase::Get()->Read(key).ToStdString(), 
    std::string(1, wxExGetFieldSeparator())); tkz.HasMoreTokens(); )
  {
    if (const wxString val = tkz.GetNextToken(); val != value)
    {
      v.emplace_back(val);
    }
  }

  wxConfigBase::Get()->Write(key, std::accumulate(v.begin(), v.end(), wxString{}, 
    [&](const wxString& a, const wxString& b) {
      return a + b + wxExGetFieldSeparator();}));
  
  return value.ToStdString();
}
  
const std::string wxExEllipsed(
  const wxString& text, const std::string& control, bool ellipse)
{
  return text.ToStdString() + 
    (ellipse ? "...": std::string()) + 
    (!control.empty() ? "\t" + control: std::string());
}

const std::string wxExFirstOf(
  const std::string& text, 
  const std::string& chars, 
  size_t start_pos,
  long flags)
{
  const auto pos = !(flags & FIRST_OF_FROM_END) ? 
    text.find_first_of(chars, start_pos):
    text.find_last_of(chars, start_pos);

  if (!(flags & FIRST_OF_BEFORE))
  {
    return pos == std::string::npos ?
      std::string():
      text.substr(pos + 1);
  }
  else
  {
    return pos == std::string::npos ?
      text:
      text.substr(0, pos);
  }
}

const std::string wxExGetEndOfText(const std::string& text, size_t max_chars)
{
  auto text_out(text);

  if (text_out.length() > max_chars)
  {
    if (4 + text_out.length() - max_chars < text_out.length())
    {
      text_out = "..." + text_out.substr(4 + text_out.length() - max_chars);
    }
    else
    {
      text_out = text.substr(text.length() - max_chars);
    }
  }

  return text_out;
}

const std::string wxExGetFindResult(const std::string& find_text, 
  bool find_next, bool recursive)
{
  if (!recursive)
  {
    const auto where = (find_next) ? _("bottom").ToStdString(): _("top").ToStdString();
    return
      _("Searching for").ToStdString() + " " + 
      wxExQuoted(wxExSkipWhiteSpace(find_text)) + " " +
      _("hit").ToStdString() + " " + where;
  }
  else
  {
    if (wxConfigBase::Get()->ReadLong(_("Error bells"), 1))
    {
      wxBell();
    }
    return
      wxExQuoted(wxExSkipWhiteSpace(find_text)) + " " + _("not found").ToStdString();
  }
}

const char wxExGetFieldSeparator()
{
  return '\x0B';
}

int wxExGetIconID(const wxExPath& filename)
{
  return filename.FileExists() ? 
    wxFileIconsTable::file: (filename.DirExists() ? 
    wxFileIconsTable::folder: 
    wxFileIconsTable::computer);
}

int wxExGetNumberOfLines(const std::string& text, bool trim)
{
  if (text.empty())
  {
    return 0;
  }
  
  const auto trimmed = (trim ? wxExSkipWhiteSpace(text): text);
  
  if (const int c = std::count(trimmed.begin(), trimmed.end(), '\n') + 1; c != 1)
  {
    return c;
  }
  
  return std::count(trimmed.begin(), trimmed.end(), '\r') + 1;
}

const std::string wxExGetStringSet(
  const std::set<std::string>& kset, size_t min_size, const std::string& prefix)
{
  return std::accumulate(kset.begin(), kset.end(), std::string{}, 
    [&](const std::string& a, const std::string& b) {
      return (b.size() >= min_size && b.find(prefix) == 0) ? a + b + ' ': a;});
}

const std::string wxExGetWord(std::string& text,
  bool use_other_field_separators,
  bool use_path_separator)
{
  std::string field_separators = " \t";
  if (use_other_field_separators) field_separators += ":";
  if (use_path_separator) field_separators = wxFILE_SEP_PATH;
  std::string token;
  wxExTokenizer tkz(text, field_separators);
  if (tkz.HasMoreTokens()) token = tkz.GetNextToken();
  text = tkz.GetString();
  text = wxExSkipWhiteSpace(text, SKIP_LEFT);
  return token;
}

bool wxExIsBrace(int c) 
{
  return c == '[' || c == ']' ||
         c == '(' || c == ')' ||
         c == '{' || c == '}' ||
         c == '<' || c == '>';
}
         
bool wxExIsCodewordSeparator(int c) 
{
  return isspace(c) || wxExIsBrace(c) || 
         c == ',' || c == ';' || c == ':' || c == '@';
}

const std::list < std::string > wxExListFromConfig(const std::string& config)
{
  return wxExTokenizer(
    wxConfigBase::Get()->Read(config).ToStdString(), 
    std::string(1, wxExGetFieldSeparator())).Tokenize<std::list < std::string >>();
}

/// Saves entries from a list with strings to the config.
void wxExListToConfig(const std::list < std::string > & l, const std::string& config)
{
  if (l.empty()) return;

  std::string text;
  const int commandsSaveInConfig = 75;
  int items = 0;

  for (const auto& it : l)
  {
    if (items++ > commandsSaveInConfig) break;
    text += it + wxExGetFieldSeparator();
  }
  
  wxConfigBase::Get()->Write(config, text.c_str());
}

void wxExLogStatus(const wxExPath& fn, long flags)
{
  wxString text = ((flags & STAT_FULLPATH) ? 
    fn.Path().string(): fn.GetFullName());

  if (fn.GetStat().IsOk())
  {
    const wxString what = ((flags & STAT_SYNC) ? 
      _("Synchronized"):
      _("Modified"));
        
    text += " " + what + " " + fn.GetStat().GetModificationTime();
  }

  wxLogStatus(text);
}

void wxExLogStatus(const std::string& text)
{
  wxLogStatus(wxString(text));
}

long wxExMake(const wxExPath& makefile)
{
  wxExProcess* process = new wxExProcess;

  return process->Execute(
    wxConfigBase::Get()->Read("Make", "make").ToStdString() + " " +
      wxConfigBase::Get()->Read("MakeSwitch", "-f").ToStdString() + " " +
      makefile.Path().string(),
    PROCESS_EXEC_DEFAULT,
    makefile.GetPath());
}

bool wxExMarkerAndRegisterExpansion(wxExEx* ex, std::string& text)
{
  if (ex == nullptr) return false;

  for (wxExTokenizer tkz(text, "'" + std::string(1, WXK_CONTROL_R), false); tkz.HasMoreTokens(); )
  {
    tkz.GetNextToken();
    
    if (const auto rest(tkz.GetString()); !rest.empty())
    {
      // Replace marker.
      if (const char name(rest[0]); tkz.GetLastDelimiter() == '\'')
      {
        if (const auto line = ex->MarkerLine(name); line >= 0)
        {
          wxExReplaceAll(text, 
            tkz.GetLastDelimiter() + std::string(1, name), 
            std::to_string(line + 1));
        }
        else
        {
          return false;
        }
      }
      // Replace register.
      else
      {
        wxExReplaceAll(text,
          tkz.GetLastDelimiter() + std::string(1, name), 
          name == '%' ? ex->GetSTC()->GetFileName().GetFullName(): ex->GetMacros().GetRegister(name));
      }
    }
  }
  
  return true;
}
  
int wxExMatch(const std::string& reg, const std::string& text, 
  std::vector < std::string > & v)
{
  try 
  {
    if (std::match_results<std::string::const_iterator> m;
      !std::regex_search(text, m, std::regex(reg))) 
    {
      return -1;
    }
    else if (m.size() > 1)
    {
      v.clear();
      std::copy(++m.begin(), m.end(), std::back_inserter(v));
    }

    return v.size();
  }
  catch (std::regex_error& e) 
  {
    wxExLog(e) << reg << "code:" << e.code();
    return -1;
  }
}

bool wxExMatchesOneOf(const std::string& fullname, const std::string& pattern)
{
  if (pattern == "*") return true; // asterix matches always.

  // Make a regex of pattern matching chars.
  auto re(pattern); 
  wxExReplaceAll(re, ".", "\\.");
  wxExReplaceAll(re, "*", ".*");
  wxExReplaceAll(re, "?", ".?");
  
  for (wxExTokenizer tkz(re, ";"); tkz.HasMoreTokens(); )
  {
    if (std::regex_match(fullname, std::regex(tkz.GetNextToken()))) return true;
  }
  
  return false;
}

void wxExNodeProperties(const pugi::xml_node* node, std::vector<wxExProperty>& properties)
{
  for (const auto& child: node->children())
  {
    if (strcmp(child.name(), "property") == 0)
    {
      properties.emplace_back(child);
    }
  }
}

void wxExNodeStyles(const pugi::xml_node* node, const std::string& lexer,
  std::vector<wxExStyle>& styles)
{
  for (const auto& child: node->children())
  {
    if (strcmp(child.name(), "style") == 0)
    {
      styles.emplace_back(child, lexer);
    }
  }
}

bool wxExOneLetterAfter(const std::string& text, const std::string& letter)
{
  return std::regex_match(letter, std::regex("^" + text + "[a-zA-Z]$"));
}

int wxExOpenFiles(wxExFrame* frame, const std::vector< wxExPath > & files,
  const wxExSTCData& stc_data, int dir_flags)
{
  wxWindowUpdateLocker locker(frame);
  
  int count = 0;
  
  for (const auto& it : files)
  {
    if (
      it.Path().string().find("*") != std::string::npos || 
      it.Path().string().find("?") != std::string::npos)
    {
      count += wxExDirOpenFile(frame, 
        wxExPath::Current(),
        it.Path().string(), stc_data.Flags(), dir_flags).FindFiles();
    }
    else
    {
      wxExPath fn(it);
      wxExSTCData data(stc_data);

      if (!it.FileExists() && it.Path().string().find(":") != std::string::npos)
      {
        if (const wxExPath& val(wxExLink().GetPath(it.Path().string(), data.Control()));
          !val.Path().empty())
        {
          fn = val;
        }
      }

      if (!fn.FileExists())
      {
        fn.MakeAbsolute();
      }
       
      if (frame->OpenFile(fn, data) != nullptr)
      {
        count++;
      }
    }
  }
  
  return count;
}

void wxExOpenFilesDialog(wxExFrame* frame,
  long style, const wxString& wildcards, bool ask_for_continue,
  const wxExSTCData& data, int dir_flags)
{
  wxArrayString paths;
  const wxString caption(_("Select Files"));
      
  if (auto* stc = frame->GetSTC(); stc != nullptr)
  {
    wxExFileDialog dlg(
      &stc->GetFile(),
      wxExWindowData().Style(style).Title(caption.ToStdString()),
      wildcards);

    if (ask_for_continue)
    {
      if (dlg.ShowModalIfChanged(true) == wxID_CANCEL) return;
    }
    else
    {
      if (dlg.ShowModal() == wxID_CANCEL) return;
    }
      
    dlg.GetPaths(paths);
  }
  else
  {
    wxFileDialog dlg(frame,
      caption,
      wxEmptyString,
      wxEmptyString,
      wildcards,
      style);

    if (dlg.ShowModal() == wxID_CANCEL) return;

    dlg.GetPaths(paths);
  }

  wxExOpenFiles(frame, wxExToVectorPath(paths).Get(), data, dir_flags);
}

const std::string wxExPrintCaption(const wxExPath& filename)
{
  return filename.Path().string();
}

const std::string wxExPrintFooter()
{
  return _("Page @PAGENUM@ of @PAGESCNT@").ToStdString();
}

const std::string wxExPrintHeader(const wxExPath& filename)
{
  if (filename.FileExists())
  {
    return
      wxExGetEndOfText(
        filename.Path().string() + " " +
        wxDateTime(filename.GetStat().st_mtime).Format().ToStdString(), 
        filename.GetLexer().GetLineSize());
  }
  else
  {
    return _("Printed").ToStdString() + ": " + wxDateTime::Now().Format().ToStdString();
  }
}

const std::string wxExQuoted(const std::string& text)
{
  return "'" + text + "'";
}

bool wxExRegAfter(const std::string& text, const std::string& letter)
{
  return std::regex_match(letter, std::regex("^" + text + "[0-9=\"a-z%._]$"));
}

int wxExReplaceAll(std::string& text, 
  const std::string& search,
  const std::string& replace,
  int* match_pos) 
{
  int count = 0;
  bool update = false;

  for (size_t pos = 0; (pos = text.find(search, pos)) != std::string::npos; ) 
  {
    if (match_pos != nullptr && !update)
    {
      *match_pos = (int)pos;
      update = true;
    }

    text.replace(pos, search.length(), replace);
    pos += replace.length();

    count++;
  }
  
  return count;
}

template <typename InputIterator>
const std::string GetColumn(InputIterator first, InputIterator last)
{
  std::string text;
  
  for (InputIterator it = first; it != last; ++it) 
  {
    text += it->second;
  }

  return text;
}
    
template <typename InputIterator>
const std::string GetLines(std::vector<std::string> & lines,
  size_t pos, size_t len, InputIterator ii)
{
  std::string text;
  
  for (auto it : lines)
  {
    text += it.replace(pos, len, *ii);
    ++ii;
  }

  return text;
}
    
bool wxExShellExpansion(std::string& command)
{
  std::vector <std::string> v;
  const std::string re_str("`(.*?)`"); // non-greedy
  const std::regex re(re_str);
  
  while (wxExMatch(re_str, command, v) > 0)
  {
    wxExProcess process;
    if (!process.Execute(v[0], PROCESS_EXEC_WAIT)) return false;
    
    command = std::regex_replace(
      command, 
      re, 
      process.GetStdOut(), 
      std::regex_constants::format_sed);
  }
  
  return true;
}

const std::string wxExSkipWhiteSpace(
  const std::string& text, 
  size_t skip_type,
  const std::string& replace_with)
{
  auto output(text); 

  if (skip_type == SKIP_ALL)
  {
    output = std::regex_replace(output, 
      std::regex("[ \t\n\v\f\r]+"), replace_with, std::regex_constants::format_sed);
  }
  
  if (skip_type & SKIP_LEFT)
  {
    output = std::regex_replace(output, 
      std::regex("^[ \t\n\v\f\r]+"), "", std::regex_constants::format_sed);
  }

  if (skip_type & SKIP_RIGHT)
  {
    output = std::regex_replace(output, 
      std::regex("[ \t\n\v\f\r]+$"), "", std::regex_constants::format_sed);
  }
  
  return output;
}

const std::string wxExSort(const std::string& input, 
  size_t sort_type, size_t pos, const std::string& eol, size_t len)
{
  wxBusyCursor wait;

  // Empty lines are not kept after sorting, as they are used as separator.
  std::map<std::string, std::string> m;
  std::multimap<std::string, std::string> mm;
  std::multiset<std::string> ms;
  std::vector<std::string> lines;
  
  for (wxExTokenizer tkz(input, eol); tkz.HasMoreTokens(); )
  {
    const std::string line = tkz.GetNextToken() + eol;
    
    // Use an empty key if line is to short.
    std::string key;
    
    if (pos < line.length())
    {
      key = line.substr(pos, len);
    }
    
    if (len == std::string::npos)
    {
      if (sort_type & STRING_SORT_UNIQUE)
        m.insert({key, line});
      else
        mm.insert({key, line});
    }
    else
    {
      lines.emplace_back(line);
      ms.insert(key);
    }
  }

  std::string text;

  if (len == std::string::npos)
  {
    if (sort_type & STRING_SORT_DESCENDING)
    {
      text = ((sort_type & STRING_SORT_UNIQUE) ?
        GetColumn(m.rbegin(), m.rend()):
        GetColumn(mm.rbegin(), mm.rend()));
    }
    else
    {
      text = ((sort_type & STRING_SORT_UNIQUE) ?
        GetColumn(m.begin(), m.end()):
        GetColumn(mm.begin(), mm.end()));
    }
  }
  else
  {
    text = ((sort_type & STRING_SORT_DESCENDING) ? 
      GetLines(lines, pos, len, ms.rbegin()):
      GetLines(lines, pos, len, ms.begin()));
  }
  
  return text;
}

bool wxExSortSelection(wxExSTC* stc,
  size_t sort_type, size_t pos, size_t len)
{
  const auto start_pos = stc->GetSelectionStart();
  
  if (start_pos == -1 || 
    pos > (size_t)stc->GetSelectionEnd() || pos == std::string::npos)
  {
    return false;
  }
  
  bool error = false;
  stc->BeginUndoAction();
  
  try
  {
    if (stc->SelectionIsRectangle())
    {
      const auto start_pos_line = stc->PositionFromLine(stc->LineFromPosition(start_pos));
      const auto end_pos_line = stc->PositionFromLine(stc->LineFromPosition(stc->GetSelectionEnd()) + 1);
      const auto nr_lines = 
        stc->LineFromPosition(stc->GetSelectionEnd()) - 
        stc->LineFromPosition(start_pos);
        
      const auto sel = stc->GetTextRange(start_pos_line, end_pos_line); 
      stc->DeleteRange(start_pos_line, end_pos_line - start_pos_line);
      const auto text(wxExSort(sel.ToStdString(), sort_type, pos, stc->GetEOL(), len));
      stc->InsertText(start_pos_line, text);

      stc->SetCurrentPos(start_pos);
      stc->SelectNone();      
      for (size_t j = 0; j < len; j++)
      {
        stc->CharRightRectExtend();
      }
      for (int i = 0; i < nr_lines; i++)
      {
        stc->LineDownRectExtend();
      }
    }
    else
    {
      const auto text(wxExSort(stc->GetSelectedText().ToStdString(), sort_type, pos, stc->GetEOL(), len));
      stc->ReplaceSelection(text);
      stc->SetSelection(start_pos, start_pos + text.size());
    }
  }
  catch (std::exception& e)
  {
    wxExLog(e) << "during sort";
    error = true;
  }
  
  stc->EndUndoAction();
  
  return !error;
}
  
const std::string wxExTranslate(const std::string& text, 
  int pageNum, int numPages)
{
  auto translation(text);

  wxExReplaceAll(translation, "@PAGENUM@", std::to_string(pageNum));
  wxExReplaceAll(translation, "@PAGESCNT@", std::to_string(numPages));

  return translation;
}

void wxExVCSCommandOnSTC(const wxExVCSCommand& command, 
  const wxExLexer& lexer, wxExSTC* stc)
{
  if (command.IsBlame())
  {
    // Do not show an edge for blamed documents, they are too wide.
    stc->SetEdgeMode(wxSTC_EDGE_NONE);
  }
  
  if (command.IsDiff())
  {
    stc->GetLexer().Set("diff");
  }
  else if (command.IsHistory())
  {
    stc->GetLexer().Reset();
  }
  else if (command.IsOpen())
  {
    stc->GetLexer().Set(lexer, true); // fold
  }
  else
  {
    stc->GetLexer().Reset();
  }
}

void wxExVCSExecute(wxExFrame* frame, int id, const std::vector< wxExPath > & files)
{
  if (files.empty()) return;
  
  if (wxExVCS vcs(files, id); vcs.GetEntry().GetCommand().IsOpen())
  {
    if (vcs.ShowDialog() == wxID_OK)
    {
      for (const auto& it : files)
      {
        if (wxExVCS vcs({it}, id); vcs.Execute())
        {
          if (!vcs.GetEntry().GetStdOut().empty())
          {
            frame->OpenFile(it, vcs.GetEntry());
          }
          else if (!vcs.GetEntry().GetStdErr().empty())
          {
            wxExLog() << vcs.GetEntry().GetStdErr();
          }
          else
          {
            wxLogStatus("No difference");
            VLOG(9) << "no output from: " << vcs.GetEntry().GetExecuteCommand();
          }
        }
      }
    }
  }
  else
  {
    vcs.Request();
  }
}

void wxExXmlError(
  const wxExPath& filename, 
  const pugi::xml_parse_result* result,
  wxExSTC* stc)
{
  wxExLogStatus("xml error: " + std::string(result->description()));
  wxExLog(*result) << filename.GetName();

  // prevent recursion
  if (stc == nullptr && filename != wxExLexers::Get()->GetFileName())
  {
    if (auto* frame = wxDynamicCast(wxTheApp->GetTopWindow(), wxExManagedFrame);
      frame != nullptr)
    {
      stc = frame->OpenFile(filename);
    }
  }

  if (stc != nullptr && result->offset != 0)
  {
    stc->GetVi().Command("gg");
    stc->GetVi().Command(std::to_string(result->offset) + "|");
  }
}
