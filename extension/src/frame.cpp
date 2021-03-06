////////////////////////////////////////////////////////////////////////////////
// Name:      frame.cpp
// Purpose:   Implementation of wxExFrame class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/fdrepdlg.h> // for wxFindDialogDialog and Event
#include <wx/persist/toplevel.h>
#include <wx/extension/frame.h>
#include <wx/extension/defs.h>
#include <wx/extension/frd.h>
#include <wx/extension/grid.h>
#include <wx/extension/lexers.h>
#include <wx/extension/listview.h>
#include <wx/extension/path.h>
#include <wx/extension/printing.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>
#include <wx/extension/tostring.h>
#include <wx/extension/vcsentry.h>

#define wxCAST_TO(classname)                                 \
  if (m_FindFocus != nullptr && m_FindFocus->IsShown())      \
  {                                                          \
    if (classname* win = dynamic_cast<classname*>(m_FindFocus); \
      win != nullptr)                                        \
    {                                                        \
      return win;                                            \
    }                                                        \
  }                                                          \
                                                             \
  wxWindow* win = wxWindow::FindFocus();                     \
  classname* cl = dynamic_cast<classname*>(win);             \
  return cl;
  
#define FIND_REPLACE( text, dlg)                             \
{                                                            \
  if (m_FindReplaceDialog != nullptr)                        \
  {                                                          \
    m_FindReplaceDialog->Destroy();                          \
  }                                                          \
                                                             \
  auto* win = wxWindow::FindFocus();                         \
                                                             \
  if (auto* cl = dynamic_cast<wxExSTC*>(win);                \
    cl != nullptr)                                           \
  {                                                          \
    m_FindFocus = cl;                                        \
  }                                                          \
  else                                                       \
  {                                                          \
    if (auto* cl = dynamic_cast<wxExListView*>(win);         \
      cl != nullptr)                                         \
    {                                                        \
      m_FindFocus = cl;                                      \
    }                                                        \
    else                                                     \
    {                                                        \
      if (auto* grid = dynamic_cast<wxExGrid*>(win);         \
        grid != nullptr)                                     \
      {                                                      \
        m_FindFocus = grid;                                  \
      }                                                      \
    }                                                        \
  }                                                          \
                                                             \
  if (auto* stc = GetSTC(); stc != nullptr)                  \
  {                                                          \
    stc->GetFindString();                                    \
  }                                                          \
                                                             \
  m_FindReplaceDialog = new wxFindReplaceDialog(             \
    this, &wxExFindReplaceData::Get()->GetFRD(), text, dlg); \
  m_FindReplaceDialog->Show();                               \
};                                                           \
  
#if wxUSE_DRAG_AND_DROP
class FileDropTarget : public wxFileDropTarget
{
public:
  explicit FileDropTarget(wxExFrame* frame) 
    : m_Frame(frame){;};

  virtual bool OnDropFiles(wxCoord x, wxCoord y, 
    const wxArrayString& filenames) override {
      wxExOpenFiles(m_Frame, wxExToVectorPath(filenames).Get());
      return true;}
private:
  wxExFrame* m_Frame;
};
#endif

wxExFrame::wxExFrame(const wxExWindowData& data)
  : wxFrame(
      data.Parent(), 
      data.Id(), 
      data.Title().empty() ? std::string(wxTheApp->GetAppDisplayName()): data.Title(), 
      data.Pos(), data.Size(), 
      data.Style() == DATA_NUMBER_NOT_SET ? 
        wxDEFAULT_FRAME_STYLE: data.Style(), 
      data.Name().empty() ? "wxExFrame": data.Name())
{
#if wxUSE_DRAG_AND_DROP
  SetDropTarget(new FileDropTarget(this));
#endif

  wxAcceleratorEntry entries[4];
  entries[0].Set(wxACCEL_NORMAL, WXK_F5, wxID_FIND);
  entries[1].Set(wxACCEL_NORMAL, WXK_F6, wxID_REPLACE);
  entries[2].Set(wxACCEL_CTRL, (int)'I', ID_VIEW_MENUBAR);
  entries[3].Set(wxACCEL_CTRL, (int)'T', ID_VIEW_TITLEBAR);

  wxAcceleratorTable accel(WXSIZEOF(entries), entries);
  SetAcceleratorTable(accel);

  wxPersistentRegisterAndRestore(this);
  
#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE
  wxExPrinting::Get()->GetHtmlPrinter()->SetParentWindow(this);
#endif

  Bind(wxEVT_FIND, [=](wxFindDialogEvent& event) {
    if (m_FindFocus != nullptr) wxPostEvent(m_FindFocus, event);});
  Bind(wxEVT_FIND_NEXT, [=](wxFindDialogEvent& event) {
    if (m_FindFocus != nullptr) wxPostEvent(m_FindFocus, event);});
  Bind(wxEVT_FIND_REPLACE, [=](wxFindDialogEvent& event) {
    if (m_FindFocus != nullptr) wxPostEvent(m_FindFocus, event);});
  Bind(wxEVT_FIND_REPLACE_ALL, [=](wxFindDialogEvent& event) {
  if (m_FindFocus != nullptr) wxPostEvent(m_FindFocus, event);});

  Bind(wxEVT_FIND_CLOSE, [=](wxFindDialogEvent& event) {
    wxASSERT(m_FindReplaceDialog != nullptr);
    // Hiding instead of destroying, does not 
    // show the dialog next time.
    m_FindReplaceDialog->Destroy();
    m_FindReplaceDialog = nullptr;});

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (GetStatusBar() != nullptr)
    {
      GetStatusBar()->Show(!GetStatusBar()->IsShown());
      SendSizeEvent();
    }}, ID_VIEW_STATUSBAR);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    (GetStatusBar() != nullptr ? event.Check(GetStatusBar()->IsShown()): event.Check(false));},
    ID_VIEW_STATUSBAR);

  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    if (auto* lv = GetListView(); lv != nullptr && lv->HasFocus())
    {
      UpdateStatusBar(lv);
    }}, ID_UPDATE_STATUS_BAR);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {FIND_REPLACE(_("Find"), 0 );}, wxID_FIND);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {FIND_REPLACE( _("Replace") , wxFR_REPLACEDIALOG );}, wxID_REPLACE);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_IsCommand = true;
    if (!event.GetString().empty())
    {
      std::string text(event.GetString());
      if (auto* stc = GetSTC(); stc != nullptr)
      {
        wxExPath::Current(stc->GetFileName().GetPath());
        if (!wxExMarkerAndRegisterExpansion(&stc->GetVi(), text)) return;
      }
      if (!wxExShellExpansion(text)) return;
      std::string cmd;
      if (std::vector <std::string> v; wxExMatch("\\+([^ \t]+)* *(.*)", text, v) > 1)
      {
        cmd = v[0];
        text = v[1];
      }
      wxExOpenFiles(this, wxExToVectorPath(text).Get(), wxExControlData().Command(cmd));
    }
    else
    {
      wxExOpenFilesDialog(this);
    }}, wxID_OPEN);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    SetMenuBar(GetMenuBar() != nullptr ? nullptr: m_MenuBar);}, ID_VIEW_MENUBAR);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    SetWindowStyleFlag(!(GetWindowStyleFlag() & wxCAPTION) ? 
      wxDEFAULT_FRAME_STYLE:
      GetWindowStyleFlag() & ~wxCAPTION);
    Refresh();}, ID_VIEW_TITLEBAR);

  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    (GetMenuBar() != nullptr ? event.Check(GetMenuBar()->IsShown()): event.Check(false));},
    ID_VIEW_MENUBAR);
  
  Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& event) {
    m_IsClosing = true;
	event.Skip(); });
}

wxExFrame::~wxExFrame()
{
  if (m_FindReplaceDialog != nullptr)
  {
    m_FindReplaceDialog->Destroy();
  }
  
  wxConfigBase::Get()->Write("ShowMenuBar", 
    GetMenuBar() != nullptr && GetMenuBar()->IsShown());
}

wxExGrid* wxExFrame::GetGrid()
{
  wxCAST_TO(wxExGrid);
}

wxExListView* wxExFrame::GetListView()
{
  wxCAST_TO(wxExListView);
}

std::string wxExFrame::GetStatusText(const std::string& pane)
{
  return (m_StatusBar == nullptr ? std::string(): m_StatusBar->GetStatusText(pane));
}

wxExSTC* wxExFrame::GetSTC()
{
  wxCAST_TO(wxExSTC);
}
  
bool wxExFrame::IsOpen(const wxExPath& filename)
{
  if (auto* stc = GetSTC(); stc != nullptr)
  {
    return stc->GetFileName() == filename;
  }
  
  return false;
}
  
wxStatusBar* wxExFrame::OnCreateStatusBar(
  int number,
  long style,
  wxWindowID id,
  const wxString& name)
{
  m_StatusBar = new wxExStatusBar(this, 
    wxExWindowData().Id(id).Style(style).Name(name.ToStdString()));
  m_StatusBar->SetFieldsCount(number);
  return m_StatusBar;
}

wxExSTC* wxExFrame::OpenFile(
  const wxExPath& filename,
  const wxExSTCData& data)
{
  if (auto* stc = GetSTC(); stc != nullptr)
  {
    stc->Open(filename, data);
    return stc;
  }

  return nullptr;
}

wxExSTC* wxExFrame::OpenFile(
  const wxExPath& filename,
  const wxExVCSEntry& vcs,
  const wxExSTCData& data)
{
  if (auto* stc = GetSTC(); stc != nullptr)
  {
    stc->SetText(vcs.GetStdOut());
    wxExVCSCommandOnSTC(vcs.GetCommand(), filename.GetLexer(), stc);
    return stc;
  }

  return nullptr;
}

wxExSTC* wxExFrame::OpenFile(
  const wxExPath& filename,
  const std::string& text,
  const wxExSTCData& data)
{
  if (auto* stc = GetSTC(); stc != nullptr)
  {
    stc->SetText(text);
    return stc;
  }

  return nullptr;
}
    
void wxExFrame::SetMenuBar(wxMenuBar* bar)
{
  if (bar != nullptr)
  {
    m_MenuBar = bar;
  }
  
  wxFrame::SetMenuBar(
   !m_IsCommand && !wxConfigBase::Get()->ReadBool("ShowMenuBar", true) ? 
      nullptr: 
      bar);
}

void wxExFrame::StatusBarClicked(const std::string& pane)
{
  if (auto* stc = GetSTC(); pane == "PaneInfo")
  {
    if (stc != nullptr) 
    {
      wxPostEvent(stc, wxCommandEvent(wxEVT_MENU, wxID_JUMP_TO));
    }
    else
    {
      if (auto* lv = GetListView();
        lv != nullptr) wxPostEvent(lv, wxCommandEvent(wxEVT_MENU, wxID_JUMP_TO));
    }
  }
  else if (pane == "PaneLexer")
  {
    if (stc != nullptr) wxExLexers::Get()->ShowDialog(stc);
  }
  else if (pane == "PaneFileType")
  {
    if (stc != nullptr) stc->FileTypeMenu();
  }
  else
  {
    // Clicking on another field, do nothing. 
  }
}

bool wxExFrame::StatusText(const std::string& text, const std::string& pane)
{
  return (m_IsClosing || m_StatusBar == nullptr ? 
    false: 
    m_StatusBar->SetStatusText(text, pane));
}

bool wxExFrame::UpdateStatusBar(const wxListView* lv)
{
  if (!m_IsClosing && lv->IsShown())
  {
    const auto text = std::to_string(lv->GetItemCount()) + 
      (lv->GetSelectedItemCount() > 0 ? "," + std::to_string(lv->GetSelectedItemCount()):
       std::string());
      
    return StatusText(text, "PaneInfo");
  }
  
  return false;
}

// Do not make it const, too many const_casts needed,
bool wxExFrame::UpdateStatusBar(wxExSTC* stc, const std::string& pane)
{
  if (stc == nullptr || m_IsClosing)
  {
    return false;
  }
  
  std::string text;

  if (pane == "PaneInfo")
  {
    if (stc->GetCurrentPos() == 0)
    {
      text = std::to_string(stc->GetLineCount());
    }
    else
    {
      int start;
      int end;
      const auto line = stc->GetCurrentLine() + 1;
      const auto pos = stc->GetCurrentPos() + 1 - stc->PositionFromLine(line - 1);
      stc->GetSelection(&start, &end);

      if (const int len  = end - start; len == 0) 
      {
        text = wxString::Format("%d,%d", line, pos);
      }
      else
      {
        if (stc->SelectionIsRectangle())
        {
          text = wxString::Format("%d,%d,%d", line, pos, (int)stc->GetSelectedText().length());
        }
        else
        {
          if (const auto number_of_lines = 
            // There might be null's inside selection.
            // So use the GetSelectedTextRaw variant.
            wxExGetNumberOfLines(wxString(stc->GetSelectedTextRaw()).ToStdString());
              number_of_lines <= 1) 
            text = wxString::Format("%d,%d,%d", line, pos, len);
          else
            text = wxString::Format("%d,%d,%d", line, number_of_lines, len);
        }
      }
    }
  }
  else if (pane == "PaneLexer")
  {
    if (wxExLexers::Get()->GetThemeOk())
    {
      text = stc->GetLexer().GetDisplayLexer();
    }
  }
  else if (pane == "PaneFileType")
  {
    switch (stc->GetEOLMode())
    {
    case wxSTC_EOL_CRLF: text = "DOS"; break;
    case wxSTC_EOL_CR: text = "MAC"; break;
    case wxSTC_EOL_LF: text = "UNIX"; break;
    default: text = "UNKNOWN";
    }
  }
  else if (pane == "PaneMode")
  {
    text = stc->GetVi().Mode().String();
  }
  else
  {
    return false;
  }

  return StatusText(text, pane);
}
