////////////////////////////////////////////////////////////////////////////////
// Name:      ex.cpp
// Purpose:   Implementation of class wxExEx
//            http://pubs.opengroup.org/onlinepubs/9699919799/utilities/ex.html
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <regex>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/extension/ex.h>
#include <wx/extension/address.h>
#include <wx/extension/addressrange.h>
#include <wx/extension/cmdline.h>
#include <wx/extension/ctags.h>
#include <wx/extension/debug.h>
#include <wx/extension/defs.h>
#include <wx/extension/frd.h>
#include <wx/extension/lexers.h>
#include <wx/extension/log.h>
#include <wx/extension/managedframe.h>
#include <wx/extension/stc.h>
#include <wx/extension/stcdlg.h>
#include <wx/extension/tokenizer.h>
#include <wx/extension/type-to-value.h>
#include <wx/extension/util.h>
#include <wx/extension/version.h>
#include <wx/extension/vi-macros.h>
#include <easylogging++.h>
#include "eval.h"

#define POST_CLOSE( ID, VETO )                              \
{                                                           \
  wxCloseEvent event(ID);                                   \
  event.SetCanVeto(VETO);                                   \
  wxPostEvent(wxTheApp->GetTopWindow(), event);             \
};                                                          \

#define POST_COMMAND( ID )                                  \
{                                                           \
  wxCommandEvent event(                                     \
    wxEVT_COMMAND_MENU_SELECTED, ID);                       \
                                                            \
  if (command.find(" ") != std::string::npos)               \
  {                                                         \
    event.SetString(command.substr(command.find(" ") + 1)); \
  }                                                         \
                                                            \
  wxPostEvent(wxTheApp->GetTopWindow(), event);             \
};                                                          \

enum class wxExCommandArg
{
  INT,
  NONE,
  OTHER,
};

enum class wxExInfoMessage
{
  ADD,
  COPY,
  DEL,
};

wxExCommandArg ParseCommandWithArg(const std::string& command)
{
  if (const auto post(wxExAfter(command, ' ')); post == command)
  {
    return wxExCommandArg::NONE;
  }
  else if (atoi(post.c_str()) > 0)
  {
    return wxExCommandArg::INT;
  }
  else
  {
    return wxExCommandArg::OTHER;
  }
}

wxExEvaluator wxExEx::m_Evaluator;
wxExSTCEntryDialog* wxExEx::m_Dialog = nullptr;
wxExViMacros wxExEx::m_Macros;

wxExEx::wxExEx(wxExSTC* stc)
  : m_Command(wxExExCommand(stc))
  , m_CTags(new wxExCTags(this))
  , m_Frame(wxDynamicCast(wxTheApp->GetTopWindow(), wxExManagedFrame))
  , m_Commands {
    {":ab", [&](const std::string& command) {
      return HandleContainer<std::string, std::map<std::string, std::string>>(
        "Abbreviations", command, &m_Macros.GetAbbreviations(),
        [=](const std::string& name, const std::string& value) {
          m_Macros.SetAbbreviation(name, value);return true;});}},
#if wxCHECK_VERSION(3,1,0)
    {":ar", [&](const std::string& command) {
      wxString text;
      for (size_t i = 1; i < wxTheApp->argv.GetArguments().size(); i++)
      {
        text << wxTheApp->argv.GetArguments()[i] << "\n";
      }
      if (!text.empty()) ShowDialog("ar", text.ToStdString());
      return true;}},
#endif
    {":chd", [&](const std::string& command) {
      if (command.find(" ") == std::string::npos) return true;
      wxExPath::Current(wxExFirstOf(command, " ")); return true;}},
    {":close", [&](const std::string& command) {POST_COMMAND( wxID_CLOSE ) return true;}},
    {":de", [&](const std::string& command) {
      m_Frame->GetDebug()->Execute(wxExFirstOf(command, " "), m_Command.STC());
      return true;}},
    {":e", [&](const std::string& command) {POST_COMMAND( wxID_OPEN ) return true;}},
    {":f", [&](const std::string& command) {InfoMessage(); return true;}},
    {":grep", [&](const std::string& command) {POST_COMMAND( ID_TOOL_REPORT_FIND ) return true;}},
    {":gt", [&](const std::string& command) {return m_Command.STC()->LinkOpen();}},
    {":help", [&](const std::string& command) {POST_COMMAND( wxID_HELP ) return true;}},
    {":map", [&](const std::string& command) {
      switch (ParseCommandWithArg(command))
      {
        case wxExCommandArg::INT:
          // TODO: at this moment you cannot set KEY_CONTROL
          return HandleContainer<int, wxExViMacrosMapType>(
            "Map", command, nullptr,
            [=](const std::string& name, const std::string& value) {
              m_Macros.SetKeyMap(name, value);return true;}); 
        break;
        case wxExCommandArg::NONE: ShowDialog("Maps", 
            "[String map]\n" +
            ReportContainer<std::string, std::map<std::string, std::string>>(m_Macros.GetMap()) +
            "[Key map]\n" +
            ReportContainer<int, wxExViMacrosMapType>(m_Macros.GetKeysMap()) +
            "[Alt key map]\n" +
            ReportContainer<int, wxExViMacrosMapType>(m_Macros.GetKeysMap(KEY_ALT)) +
            "[Control key map]\n" +
            ReportContainer<int, wxExViMacrosMapType>(m_Macros.GetKeysMap(KEY_CONTROL)), 
            true);
          return true;
        break;
        case wxExCommandArg::OTHER:
          return HandleContainer<std::string, std::map<std::string, std::string>>(
            "Map", command, nullptr,
            [=](const std::string& name, const std::string& value) {
              m_Macros.SetMap(name, value);return true;});
      }
      return false;}},
    {":new", [&](const std::string& command) {POST_COMMAND( wxID_NEW ) return true;}},
    {":print", [&](const std::string& command) {m_Command.STC()->Print(command.find(" ") == std::string::npos); return true;}},
    {":pwd", [&](const std::string& command) {wxExLogStatus(wxExPath::Current()); return true;}},
    {":q!", [&](const std::string& command) {POST_CLOSE( wxEVT_CLOSE_WINDOW, false ) return true;}},
    {":q", [&](const std::string& command) {POST_CLOSE( wxEVT_CLOSE_WINDOW, true ) return true;}},
    {":reg", [&](const std::string& command) {
      ShowDialog("Registers", m_Evaluator.GetInfo(this), true);
      return true;}},
    {":sed", [&](const std::string& command) {POST_COMMAND( ID_TOOL_REPLACE ) return true;}},
    {":set", [&](const std::string& command) {
      if (command.find(" ") == std::string::npos)
      {
        POST_COMMAND( wxID_PREFERENCES )
      }
      else
      {
        const bool toggle = command.back() != '*'; // modeline does not toggle
        std::string text(command.substr(4, 
          command.back() == '*' ? command.size() - 5: std::string::npos));
        // Convert arguments (add -- to each group, remove all =).
        // ts=120 ac ic sy=cpp -> --ts 120 --ac --ic --sy cpp
        std::regex re("[0-9a-z=]+");
        text = std::regex_replace(text, re, "--&", std::regex_constants::format_sed);
        std::replace(text.begin(), text.end(), '=', ' ');
        wxExCmdLine(
          {{{"a", "ac", "Auto Complete"}, [](bool on){wxConfigBase::Get()->Write(_("Auto complete"), on);}},
           {{"b", "eb", "Error Bells"}, [](bool on){wxConfigBase::Get()->Write(_("Error bells"), on ? 2: 0);}},
           {{"C", "ic", "Ignore Case"}, [&](bool on){
             if (!on) m_SearchFlags |= wxSTC_FIND_MATCHCASE;
             else     m_SearchFlags &= ~wxSTC_FIND_MATCHCASE;
             wxExFindReplaceData::Get()->SetMatchCase(!on);}},
           {{"i", "ai", "Auto Indent"}, [](bool on){wxConfigBase::Get()->Write(_("Auto indent"), on ? 2: 0);}},
           {{"e", "re", "Regular Expression"}, [&](bool on){
             if (on) 
             {
               m_SearchFlags |= wxSTC_FIND_REGEXP;
#if wxCHECK_VERSION(3,1,1)
               m_SearchFlags |= wxSTC_FIND_CXX11REGEX;
#endif
             }
             else    
             {
               m_SearchFlags &= ~wxSTC_FIND_REGEXP;
#if wxCHECK_VERSION(3,1,1)
               m_SearchFlags &= ~wxSTC_FIND_CXX11REGEX;
#endif
             }
             wxExFindReplaceData::Get()->SetUseRegEx(on);}},
           {{"l", "el", "Edge Line"}, [&](bool on){
             m_Command.STC()->SetEdgeMode(on ? wxSTC_EDGE_LINE: wxSTC_EDGE_NONE);     
             wxConfigBase::Get()->Write(_("Edge line"), on ? wxSTC_EDGE_LINE: wxSTC_EDGE_NONE);}},
           {{"m", "sm", "Show Mode"}, [&](bool on){
             ((wxExStatusBar *)m_Frame->GetStatusBar())->ShowField("PaneMode", on);
             wxConfigBase::Get()->Write(_("Show mode"), on);}},
           {{"n", "nu", "show lineNUmbers"}, [&](bool on){
             m_Command.STC()->ShowLineNumbers(on);
             wxConfigBase::Get()->Write(_("Line numbers"), on);}},
           {{"s", "ws", "show WhiteSpace"}, [&](bool on){
             m_Command.STC()->SetViewEOL(on);
             m_Command.STC()->SetViewWhiteSpace(on ? wxSTC_WS_VISIBLEALWAYS: wxSTC_WS_INVISIBLE);
             wxConfigBase::Get()->Write(_("Whitespace"), on ? wxSTC_WS_VISIBLEALWAYS: wxSTC_WS_INVISIBLE);}},
           {{"u", "ut", "Use Tabs"}, [&](bool on){
             m_Command.STC()->SetUseTabs(on);
             wxConfigBase::Get()->Write(_("Use tabs"), on);}},
           {{"w", "mw", "Match Words"}, [&](bool on){
             if (on) m_SearchFlags |= wxSTC_FIND_WHOLEWORD;
             else    m_SearchFlags &= ~wxSTC_FIND_WHOLEWORD;
             wxExFindReplaceData::Get()->SetMatchWord(on);}},
           {{"W", "wl", "Wrap Line"}, [&](bool on){
             m_Command.STC()->SetWrapMode(on ? wxSTC_WRAP_CHAR: wxSTC_WRAP_NONE);
             wxConfigBase::Get()->Write(_("Wrap line"), on ? wxSTC_WRAP_CHAR: wxSTC_WRAP_NONE);}}},
          {{{"c", "ec", "Edge Column"}, {CMD_LINE_INT, [&](const std::any& val) {
             m_Command.STC()->SetEdgeColumn(std::any_cast<int>(val));
             wxConfigBase::Get()->Write(_("Edge column"), std::any_cast<int>(val));}}},
           {{"r", "rp", "reported lines"}, {CMD_LINE_INT, [&](const std::any& val) {
             wxConfigBase::Get()->Write("Reported lines",  std::any_cast<int>(val));}}},
           {{"S", "sw", "Shift Width"}, {CMD_LINE_INT, [&](const std::any& val) {
             m_Command.STC()->SetIndent(std::any_cast<int>(val));
             wxConfigBase::Get()->Write(_("Indent"), std::any_cast<int>(val));}}},
           {{"t", "ts", "Tab Stop"}, {CMD_LINE_INT, [&](const std::any& val) {
             m_Command.STC()->SetTabWidth(std::any_cast<int>(val));
             wxConfigBase::Get()->Write(_("Tab width"), std::any_cast<int>(val));}}},
           {{"y", "sy", "SYntax (lexer or 'off')"}, {CMD_LINE_STRING, [&](const std::any& val) {
             if (std::any_cast<std::string>(val) != "off") 
               m_Command.STC()->GetLexer().Set(std::any_cast<std::string>(val), true); // allow folding
             else              
               m_Command.STC()->GetLexer().Reset();}}}}).Parse(command.substr(0, 4) + text, toggle);
      }
      return true;}},
    {":so", [&](const std::string& command) {
      if (command.find(" ") == std::string::npos) return false;
      wxExPath path(wxExFirstOf(command, " "));
      if (path.IsRelative())
      {
        path.MakeAbsolute();
      }
      std::ifstream ifs(path.Path());
      if (!ifs.is_open()) return false;
      int i = 0;
      for (std::string line; std::getline(ifs, line); )
      {
        if (!line.empty())
        {
          if (line == command)
          {
            VLOG(9) << "recursive (line: " << i + 1 << ")";
            return false;
          }
          else if (!Command(line))
          {
            VLOG(9) << "command error (line: " << i + 1 << ")";
            return false;
          }
        }
        i++;
      }
      return true;}},
    {":syntax", [&](const std::string& command) {
      if (wxString(command).EndsWith("on"))
      {
        wxExLexers::Get()->RestoreTheme();
        m_Command.STC()->GetLexer().Set(m_Command.STC()->GetFileName().GetLexer().GetDisplayLexer(), true); // allow folding
      }
      else if (wxString(command).EndsWith("off"))
      {
        m_Command.STC()->GetLexer().Reset();
        wxExLexers::Get()->SetThemeNone();
      }
      else
      {
        return false;
      }
      m_Frame->StatusText(wxExLexers::Get()->GetTheme(), "PaneTheme");
      return true;}},
    {":ta", [&](const std::string& command) {
      m_CTags->Find(wxExFirstOf(command, " "));
      return true;}},
    {":una", [&](const std::string& command) {
      if (wxExTokenizer tkz(command); tkz.CountTokens() >= 1)
      {
        tkz.GetNextToken(); // skip :una
        m_Macros.SetAbbreviation(tkz.GetNextToken(), "");
      }
      return true;}},
    {":unm", [&](const std::string& command) {
      if (wxExTokenizer tkz(command); tkz.CountTokens() >= 1)
      {
        tkz.GetNextToken(); // skip :unm
        switch (ParseCommandWithArg(command))
        {
          case wxExCommandArg::INT: m_Macros.SetKeyMap(tkz.GetNextToken(), ""); break; 
          case wxExCommandArg::NONE: break;
          case wxExCommandArg::OTHER: m_Macros.SetMap(tkz.GetNextToken(), ""); break;
        }
      }
      return true;}},
    {":ve", [&](const std::string& command) {ShowDialog("Version", 
      wxExGetVersionInfo().GetVersionOnlyString().ToStdString()); return true;}},
    {":x", [&](const std::string& command) {
      if (command != ":x") return false;
      POST_COMMAND( wxID_SAVE )
      POST_CLOSE( wxEVT_CLOSE_WINDOW, true )
      return true;}}}
{
  wxASSERT(m_Frame != nullptr);
  ResetSearchFlags();
}

wxExEx::~wxExEx()
{
  delete m_CTags;
}
  
void wxExEx::AddText(const std::string& text)
{
  if (m_Register)
  {
    m_Macros.SetRegister(m_Register, text);
  }
  else
  {
    m_Command.STC()->AddTextRaw((const char *)text.c_str(), text.length());
  }

  InfoMessage(text, wxExInfoMessage::ADD);
}

std::tuple<double, int> wxExEx::Calculator(const std::string& text)
{
  const auto& [val, width, err] = m_Evaluator.Eval(this, text);

  if (!err.empty())
  {
    ShowDialog("Error", text + "\n" + err);
  }

  return {val, width};
}

bool wxExEx::Command(const std::string& cmd)
{
  auto command(cmd);

  if (!m_IsActive || command.empty() || command.front() != ':') return false;

  VLOG(9) << "ex command: " << cmd;

  const auto& it = m_Macros.GetMap().find(command);
  command = (it != m_Macros.GetMap().end() ? it->second: command);

  if (m_Frame->ExecExCommand(m_Command.Command(cmd)))
  {
    m_Command.clear();
    return true;
  }
  else if (command == ":" || command == ":'<,'>" || command == ":!")
  {
    m_Frame->GetExCommand(this, command);
    return true;
  }
  else if (
    !CommandHandle(command) &&
    !CommandAddress(command.substr(1)))
  {
    m_Command.clear();
    return false;
  }

  m_Macros.Record(command);
  m_Command.clear();

  return true;
}

bool wxExEx::CommandAddress(const std::string& command)
{
  auto rest(command);
  std::string range_str, cmd;
  bool addr1 = false; // single address

  if (rest.compare(0, 5, "'<,'>") == 0)
  {
    if (GetSelectedText().empty())
    {
      return false;
    }

    range_str = "'<,'>";
    cmd = rest.substr(5);
    rest = rest.substr(6);
  }
  else
  { 
    const std::string addr("[0-9\\.\\$\\+\\-]+"); // addr (normal)
    const std::string addrs("[\\?/].*?[\\?/]"); // addr search, non-greedy!
    const std::string addrm("'[a-z]"); // addr using marker
    const std::string cmd_group1("([aikrz=]|pu)(.*)"); // 1 addr command
    const std::string cmd_group2("([cdgjmpsStvywy<>\\!&~])(.*)"); // 2 addr command
    
    if (std::vector <std::string> v;
      // a % address range
      wxExMatch("^%" + cmd_group2, rest, v) == 2 ||
      // addr2 search
      wxExMatch("^(" + addrs + ")(," + addrs + ")" + cmd_group2, rest, v) == 4 ||
      // addr1 search
      wxExMatch("^(" + addrs + ")" + cmd_group1, rest, v) == 3 ||
      // addr2 markers
      wxExMatch("^(" + addrm + ")(," + addrm + ")" + cmd_group2, rest, v) == 4 ||
      wxExMatch("^(" + addr  + ")(," + addrm + ")" + cmd_group2, rest, v) == 4 ||
      wxExMatch("^(" + addrm + ")(," +  addr + ")" + cmd_group2, rest, v) == 4 ||
      // addr1 marker
      wxExMatch("^(" + addrm + ")" + cmd_group1, rest, v) == 3 ||
      // addr1
      wxExMatch("^(" + addr + ")?" + cmd_group1, rest, v) == 3 ||
      // addr2
      wxExMatch("^(" + addr + ")?(," + addr + ")?" + cmd_group2, rest, v) == 4)
    {
      switch (v.size())
      {
        case 2:
          range_str = "%";
          cmd = v[0];
          rest = v[1];
          break;
        case 3:
          addr1 = true;
          range_str = v[0];
          cmd = v[1];
          rest = wxExSkipWhiteSpace(v[2], SKIP_LEFT);
          break;
        case 4:
          range_str = v[0] + v[1];
          cmd = v[2];
          rest = v[3];
          break;
        default: wxFAIL; break;
      }

      if (!wxExMarkerAndRegisterExpansion(this, range_str))
      {
        return false;
      }
    }
    else 
    {
      const auto line(wxExAddress(this, rest).GetLine());
      if (line > 0) wxExSTCData(m_Command.STC()).Control(wxExControlData().Line(line)).Inject();
      return line > 0;
    }
    
    if (range_str.empty() && cmd != '!') 
    {
      range_str = (cmd == "g" || cmd == 'v' || cmd == 'w' ? "%": ".");
    }
  }
  
  if (addr1)
  {
    switch (const wxExAddress addr(this, range_str); (int)cmd[0])
    {
    case 0: return false;
    case 'a': return addr.Append(rest);
    case 'i': return addr.Insert(rest);
    case 'k': return !rest.empty() ? addr.MarkerAdd(rest[0]): false;
    case 'p': 
      if (cmd == "pu")
      { 
        return !rest.empty() ? addr.Put(rest[0]): addr.Put();
      }
      else
      {
        return false;
      }
      break;
    case 'r': return addr.Read(rest);
    case 'z': return addr.AdjustWindow(rest);
    case '=': return addr.WriteLineNumber();
    default:
      wxLogStatus("Unknown address command: %s", cmd);
      return false;
    }
  }
  else
  {
    switch (wxExAddressRange range(this, range_str); (int)cmd[0])
    {
    case 0: return false;
    case 'c': return range.Change(rest);
    case 'd': return range.Delete();
    case 'v':
    case 'g': return range.Global(rest, cmd[0] == 'v');
    case 'j': return range.Join();
    case 'm': return range.Move(wxExAddress(this, rest));
    case 'p': 
      if (m_Command.STC()->GetName() != "Print")
      {
        return range.Print(rest);
      }
      else
      {
        return false;
      }
    case 's':
    case '&':
    case '~': return range.Substitute(rest, cmd[0]);
    case 'S': return range.Sort(rest);
    case 't': return range.Copy(wxExAddress(this, rest));
    case 'w': 
      if (!rest.empty())
      {
        return range.Write(rest);
      }
      else
      {
        POST_COMMAND( wxID_SAVE )
        return true;
      }
      break;
    case 'y': return range.Yank(rest.empty() ? '0': (char)rest[0]);
    case '>': return range.Indent(true);
    case '<': return range.Indent(false);
    case '!': return range.Escape(rest);
    default:
      wxLogStatus("Unknown range command: %s", cmd);
      return false;
    }
  }
}

bool wxExEx::CommandHandle(const std::string& command) const
{
  const auto& it = std::find_if(m_Commands.begin(), m_Commands.end(), 
    [command](auto const& e) {return e.first == command.substr(0, e.first.size());});
  
  return it != m_Commands.end() && it->second(command);
}

void wxExEx::Copy(const wxExEx* ex)
{
  m_MarkerIdentifiers = ex->m_MarkerIdentifiers;
  m_Copy = true; // no char numbers for a copy
}

void wxExEx::Cut(bool show_message)
{
  const auto sel(GetSelectedText());
  
  Yank('0', false);

  m_Command.STC()->ReplaceSelection(wxEmptyString);
  
  SetRegistersDelete(sel);
  
  InfoMessage(sel, wxExInfoMessage::DEL);
}

const std::string wxExEx::GetRegisterInsert() const
{
  return m_Macros.GetRegister('.');
}

const std::string wxExEx::GetRegisterText() const
{
  return m_Register ? 
    m_Macros.GetRegister(m_Register):
    m_Macros.GetRegister('0');
}
  
const std::string wxExEx::GetSelectedText() const
{
  // This also supports rectangular text.
  if (m_Command.STC()->GetSelectedText().empty())
  {
    return std::string();
  }

  const wxCharBuffer b(m_Command.STC()->GetSelectedTextRaw());
  return std::string(b.data(), b.length() - 1);
}

template <typename S, typename T> 
bool wxExEx::HandleContainer(
  const std::string& kind,
  const std::string& command,
  const T * container,
  std::function<bool(const std::string&, const std::string&)> cb)
{
  if (wxExTokenizer tkz(command); tkz.CountTokens() >= 2)
  {
    tkz.GetNextToken(); // skip
    const auto name(tkz.GetNextToken());
    cb(name, tkz.GetString());
  }
  else if (container != nullptr)
  {
    ShowDialog(kind, ReportContainer<S, T>(*container), true);
  }

  return true;
}

void wxExEx::InfoMessage() const
{
  m_Frame->ShowExMessage(wxString::Format("%s line %d of %d --%d%%-- level %d", 
    m_Command.STC()->GetFileName().GetFullName().c_str(), 
    m_Command.STC()->GetCurrentLine() + 1,
    m_Command.STC()->GetLineCount(),
    100 * (m_Command.STC()->GetCurrentLine() + 1)/ m_Command.STC()->GetLineCount(),
    (m_Command.STC()->GetFoldLevel(m_Command.STC()->GetCurrentLine()) & wxSTC_FOLDLEVELNUMBERMASK)
     - wxSTC_FOLDLEVELBASE).ToStdString());
}

void wxExEx::InfoMessage(const std::string& text, wxExInfoMessage type) const
{
  if (const auto lines = wxExGetNumberOfLines(text);
    lines >= wxConfig::Get()->Read("Reported lines", 5))
  {
    wxString msg;

    switch (type)
    {
      case wxExInfoMessage::ADD: msg = _("%d lines added"); break;
      case wxExInfoMessage::COPY: msg = _("%d lines yanked"); break;
      case wxExInfoMessage::DEL: msg = _("%d fewer lines"); break;
    }

    m_Frame->ShowExMessage(wxString::Format(msg, lines - 1).ToStdString());
  }
}

bool wxExEx::MarkerAdd(char marker, int line)
{
  if (m_Copy) return false;

  const wxExMarker lm(wxExLexers::Get()->GetMarker(m_MarkerSymbol));

  if (!lm.IsOk())
  {
    wxExLog("could not find marker symbol") << m_MarkerSymbol.GetNo() << " in lexers";
    return false;
  }
  
  MarkerDelete(marker);

  int id;
  const int lin = (line == -1 ? m_Command.STC()->GetCurrentLine(): line);

  if (lm.GetSymbol() == wxSTC_MARK_CHARACTER)
  {
    if (const auto& it = m_MarkerNumbers.find(marker); it == m_MarkerNumbers.end())
    {
      // We have symbol:
      // 0: non-char ex marker
      // 1: change marker
      // 2: breakpoint marker
      // 3..: character markers (all markers in m_MarkerIdentifiers)
      const auto marker_offset = 3;
      const auto marker_number = m_MarkerIdentifiers.size() + marker_offset;

      m_Command.STC()->MarkerDefine(marker_number, 
        wxSTC_MARK_CHARACTER + marker, 
        wxString(lm.GetForegroundColour()), 
        wxString(lm.GetBackgroundColour()));

      id = m_Command.STC()->MarkerAdd(lin, marker_number);
      m_MarkerNumbers[marker] = marker_number;
    }
    else
    {
      id = m_Command.STC()->MarkerAdd(lin, it->second);
    }
  }
  else
  {
    id = m_Command.STC()->MarkerAdd(lin, m_MarkerSymbol.GetNo());
  }
    
  if (id == -1)
  {
    wxExLog("could not add marker") << marker  << "to line:" << lin;
    return false;  
  }
    
  m_MarkerIdentifiers[marker] = id;
  
  return true;
}  

bool wxExEx::MarkerDelete(char marker)
{
  if (const auto& it = m_MarkerIdentifiers.find(marker); it != m_MarkerIdentifiers.end())
  {
    m_Command.STC()->MarkerDeleteHandle(it->second);
    m_MarkerIdentifiers.erase(it);
    return true;
  }
  
  return false;
}

bool wxExEx::MarkerGoto(char marker)
{
  if (const auto line = MarkerLine(marker); line != -1)
  {
    wxExSTCData(m_Command.STC()).Control(wxExControlData().Line(line + 1)).Inject();
    return true;
  }
  
  return false;
}

int wxExEx::MarkerLine(char marker) const
{
  if (marker == '<')
  {
    if (!GetSelectedText().empty())
    {
      return m_Command.STC()->LineFromPosition(m_Command.STC()->GetSelectionStart());
    }
  }
  else if (marker == '>')
  {
    if (!GetSelectedText().empty())
    {
      return m_Command.STC()->LineFromPosition(m_Command.STC()->GetSelectionEnd());
    }
  }
  else
  {
    if (const auto& it = m_MarkerIdentifiers.find(marker); it != m_MarkerIdentifiers.end())
    {
      if (const auto line = m_Command.STC()->MarkerLineFromHandle(it->second); line == -1)
      {
        wxLogStatus("Handle for marker: %c invalid", marker);
      }
      else
      {
        return line;
      }
    }
    else
    {
      wxLogStatus(_("Undefined marker: %c"), marker);
    }
  }

  if (wxConfigBase::Get()->ReadLong(_("Error bells"), 1))
  {
    wxBell();
  }

  return -1;
}

void wxExEx::Print(const std::string& text)
{
  ShowDialog("Print", text);
}
  
template <typename S, typename T>
std::string wxExEx::ReportContainer(const T & container) const
{
  std::string output;

  for (const auto& it : container)
  {
    output += wxExTypeToValue<S>(it.first).getString() + "=" + it.second + "\n";
  }

  return output;
}

void wxExEx::ResetSearchFlags()
{
  m_SearchFlags = ((wxExFindReplaceData::Get()->MatchCase() ? wxSTC_FIND_MATCHCASE: 0) | 
    wxSTC_FIND_REGEXP
#if wxCHECK_VERSION(3,1,1)
    | wxSTC_FIND_CXX11REGEX
#endif
    );
}

void wxExEx::SetRegistersDelete(const std::string& value) const
{
  if (value.empty())
  {
    return;
  }
  
  for (int i = 9; i >= 2; i--)
  {
    if (const auto value(m_Macros.GetRegister(wxUniChar(48 + i - 1)));
      !value.empty())
    {
      m_Macros.SetRegister(wxUniChar(48 + i), value);
    }
  }
  
  m_Macros.SetRegister('1', value);
}
  
void wxExEx::SetRegisterInsert(const std::string& value) const
{
  if (value.empty())
  {
    return;
  }
  
  m_Macros.SetRegister('.', value);
}

void wxExEx::SetRegisterYank(const std::string& value) const
{
  if (value.empty())
  {
    return;
  }
  
  m_Macros.SetRegister('0', value);
}

void wxExEx::ShowDialog(
  const std::string& title, const std::string& text, bool prop_lexer)
{
  if (m_Dialog == nullptr)
  {
    m_Dialog = new wxExSTCEntryDialog(
      text,
      std::string(),
      wxExWindowData().Button(wxOK).Title(title));
  }
  else
  {
    if (title == "Print")
    { 
      if (title != m_Dialog->GetTitle())
      {
        m_Dialog->GetSTC()->SetText(text);
      }
      else
      {
        m_Dialog->GetSTC()->AppendText(text);
        m_Dialog->GetSTC()->DocumentEnd();
      }
    }
    else
    {
      m_Dialog->GetSTC()->SetText(text);
    }
    
    m_Dialog->SetTitle(title);
  }
  
  m_Dialog->GetSTC()->GetLexer().Set(
    prop_lexer ? wxExLexer("props"): m_Command.STC()->GetLexer());
  m_Dialog->Show();
}

bool wxExEx::Yank(const char name, bool show_message) const
{
  const auto range(GetSelectedText());
  
  if (range.empty())
  {
    return false;
  }
  else if (GetRegister())
  {
    m_Macros.SetRegister(GetRegister(), range);
  }
  else if (name != '0')
  {
    m_Macros.SetRegister(name, range);
  }
  else
  {
    SetRegisterYank(range);
  }

  InfoMessage(range, wxExInfoMessage::COPY);
  
  return true;
}
