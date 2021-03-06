////////////////////////////////////////////////////////////////////////////////
// Name:      support.cpp
// Purpose:   Implementation of DecoratedFrame class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/stockitem.h> // for wxGetStockLabel
#include <wx/extension/debug.h>
#include <wx/extension/filedlg.h>
#include <wx/extension/lexers.h>
#include <wx/extension/log.h>
#include <wx/extension/menu.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>
#include <wx/extension/vcs.h>
#include <wx/extension/vi-macros.h>
#include <wx/extension/report/listviewfile.h>
#ifndef __WXMSW__
#include "app.xpm"
#endif
#include "support.h"
#include "app.h"
#include "defs.h"

DecoratedFrame::DecoratedFrame(App* app)
  : wxExFrameWithHistory(
      25,  // maxFiles
      4,   // maxProjects
      wxExWindowData().Name("mainFrame").Style(wxDEFAULT_FRAME_STYLE))
  , m_App(app)
{
  SetIcon(wxICON(app));
  
  wxExProcess::PrepareOutput(this);

  const bool vi_mode = wxConfigBase::Get()->ReadBool(_("vi mode"), false);
  
#ifdef __WXMSW__
  const int lexer_size = 60;
#else
  const int lexer_size = 75;
#endif
  SetupStatusBar({
    {"PaneFileType", 50, _("File type").ToStdString()},
    {"PaneInfo", 100, _("Lines or items").ToStdString()},
    {"PaneLexer", lexer_size, _("Lexer").ToStdString()},
    {"PaneTheme", lexer_size, _("Theme").ToStdString()},
    {"PaneVCS", 150},
    {"PaneMacro", 75},
    {"PaneMode", 100}});
  
  if (wxExVCS vcs; vcs.Use() && wxExVCS::GetCount() > 0)
  {
    vcs.SetEntryFromBase();
    StatusText(!vcs.GetBranch().empty() ? vcs.GetBranch(): vcs.GetName(), "PaneVCS");
  }
  else
  {
    m_StatusBar->ShowField("PaneVCS", false);
  }
  
  if (wxExLexers::Get()->GetLexers().empty())
  {
    m_StatusBar->ShowField("PaneLexer", false);
    m_StatusBar->ShowField("PaneTheme", false);
  }
  
  m_StatusBar->ShowField("PaneMacro", vi_mode);
  m_StatusBar->ShowField("PaneMode", false);

  auto *menuFile = new wxExMenu();
  menuFile->Append(wxID_NEW,
    wxExEllipsed(wxGetStockLabel(wxID_NEW, wxSTOCK_NOFLAGS), "\tCtrl+N"));
  menuFile->Append(wxID_OPEN);
  GetFileHistory().UseMenu(ID_RECENT_FILE_MENU, menuFile);
  menuFile->AppendSeparator();
  menuFile->Append(wxID_CLOSE,
    wxGetStockLabel(wxID_CLOSE, wxSTOCK_NOFLAGS) + "\tCtrl+W");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_SAVE);
  menuFile->Append(wxID_SAVEAS);
  menuFile->Append(ID_ALL_SAVE,
    _("Save A&ll"), std::string(), wxART_FILE_SAVE);
  menuFile->AppendSeparator();
  menuFile->AppendPrint();
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  auto *menuEdit = new wxExMenu();
  menuEdit->Append(wxID_UNDO);
  menuEdit->Append(wxID_REDO);
  menuEdit->AppendSeparator();
  menuEdit->Append(wxID_CUT);
  menuEdit->Append(wxID_COPY);
  menuEdit->Append(wxID_PASTE);
  menuEdit->AppendSeparator();
  menuEdit->Append(wxID_JUMP_TO);
  menuEdit->AppendSeparator();
  auto* menuFind = new wxExMenu();
  
  if (vi_mode)
  {
    // No accelerators for vi mode, Ctrl F is page down.
    menuFind->Append(wxID_FIND, wxGetStockLabel(wxID_FIND, wxSTOCK_NOFLAGS));

    if (!(m_App->GetData().Flags() & STC_WIN_READ_ONLY))
    {
      menuFind->Append(wxID_REPLACE, wxGetStockLabel(wxID_REPLACE, wxSTOCK_NOFLAGS));
    }
  }
  else
  {
    menuFind->Append(wxID_FIND);

    if (!(m_App->GetData().Flags() & STC_WIN_READ_ONLY))
    {
      menuFind->Append(wxID_REPLACE);
    }
  }
  
  menuFind->Append(ID_TOOL_REPORT_FIND, wxExEllipsed(_("Find &in Files")));

  if (!(m_App->GetData().Flags() & STC_WIN_READ_ONLY))
  {
    menuFind->Append(ID_TOOL_REPLACE, wxExEllipsed(_("Replace in File&s")));
  }

  menuEdit->AppendSubMenu(menuFind, !(m_App->GetData().Flags() & STC_WIN_READ_ONLY) ?
    _("&Find and Replace"): _("&Find"));
  menuEdit->AppendSeparator();
  menuEdit->Append(
    ID_EDIT_CONTROL_CHAR, wxExEllipsed(_("&Control Char"), "Ctrl+K"));
  menuEdit->AppendSeparator();
  
  auto* menuMacro = new wxExMenu();
  menuMacro->Append(ID_EDIT_MACRO_START_RECORD, wxExEllipsed(_("Start Record")));
  menuMacro->Append(ID_EDIT_MACRO_STOP_RECORD, _("Stop Record"));
  menuMacro->AppendSeparator();
  menuMacro->Append(ID_EDIT_MACRO_PLAYBACK, wxExEllipsed(_("Playback"), "Ctrl+M"));
  
  if (wxExViMacros::GetFileName().FileExists())
  {
    menuMacro->AppendSeparator();
    menuMacro->Append(ID_EDIT_MACRO, wxGetStockLabel(wxID_EDIT));
  }
  
  menuEdit->AppendSubMenu(menuMacro, _("&Macro"), std::string(), ID_EDIT_MACRO_MENU);

  auto *menuView = new wxExMenu;
  AppendPanes(menuView);
  menuView->AppendSeparator();
  menuView->AppendCheckItem(ID_VIEW_FILES, _("&Files"));
  menuView->AppendCheckItem(ID_VIEW_PROJECTS, _("&Projects"));
  menuView->AppendCheckItem(ID_VIEW_DIRCTRL, _("&Explorer"));
  menuView->AppendCheckItem(ID_VIEW_HISTORY, _("&History"));
  menuView->AppendCheckItem(ID_VIEW_OUTPUT, _("&Output"));
  menuView->AppendSeparator();
  menuView->AppendCheckItem(ID_VIEW_ASCII_TABLE, _("&Ascii Table"));

  auto *menuProcess = new wxExMenu();
  menuProcess->Append(ID_PROCESS_SELECT, wxExEllipsed(_("&Select")));
  menuProcess->AppendSeparator();
  menuProcess->Append(wxID_EXECUTE);
  menuProcess->Append(wxID_STOP);

  auto *menuProject = new wxExMenu();
  menuProject->Append(
    ID_PROJECT_NEW, wxGetStockLabel(wxID_NEW), std::string(), wxART_NEW);
  menuProject->Append(
    ID_PROJECT_OPEN, wxGetStockLabel(wxID_OPEN), std::string(), wxART_FILE_OPEN);
  GetProjectHistory().UseMenu(ID_RECENT_PROJECT_MENU, menuProject);
  menuProject->Append(ID_PROJECT_OPENTEXT, _("&Open as Text"));
  menuProject->AppendSeparator();
  menuProject->Append(
    ID_PROJECT_CLOSE, wxGetStockLabel(wxID_CLOSE), std::string(), wxART_CLOSE);
  menuProject->AppendSeparator();
  menuProject->Append(
    ID_PROJECT_SAVE, wxGetStockLabel(wxID_SAVE), std::string(), wxART_FILE_SAVE);
  menuProject->Append(
    ID_PROJECT_SAVEAS,
    wxGetStockLabel(wxID_SAVEAS), std::string(), wxART_FILE_SAVE_AS);
  menuProject->AppendSeparator();
  menuProject->AppendCheckItem(ID_SORT_SYNC, _("&Auto Sort"));
  
  wxExMenu* menuDebug = nullptr;

  if (m_App->GetDebug())
  {
    menuDebug = new wxExMenu();

    if (GetDebug()->AddMenu(menuDebug) == 0)
    {
      delete menuDebug;
      menuDebug = nullptr;
      wxExLog() << "no debug menu present";
    }
  }

  wxMenu* menuOptions = new wxMenu();
  
  if (wxExVCS::GetCount() > 0)
  {
    menuOptions->Append(ID_OPTION_VCS, wxExEllipsed(_("Set &VCS")));
    menuOptions->AppendSeparator();
  }

#ifndef __WXOSX__
  menuOptions->Append(wxID_PREFERENCES, wxExEllipsed(_("Set &Editor Options")));
#else
  menuOptions->Append(wxID_PREFERENCES);
#endif  
  menuOptions->AppendSeparator();
  menuOptions->Append(ID_OPTION_LIST, wxExEllipsed(_("Set &List Options")));

  auto *menuHelp = new wxExMenu();
  menuHelp->Append(wxID_ABOUT);
  menuHelp->Append(wxID_HELP);

  wxMenuBar* menubar = new wxMenuBar();
  menubar->Append(menuFile, wxGetStockLabel(wxID_FILE));
  menubar->Append(menuEdit, wxGetStockLabel(wxID_EDIT));
  menubar->Append(menuView, _("&View"));
  menubar->Append(menuProcess, _("&Process"));
  menubar->Append(menuProject, _("&Project"));
  if (menuDebug != nullptr)
    menubar->Append(menuDebug, _("&Debug"));
  menubar->Append(menuOptions, _("&Options"));
  menubar->Append(menuHelp, wxGetStockLabel(wxID_HELP));
  
  SetMenuBar(menubar);
}

bool DecoratedFrame::AllowClose(wxWindowID id, wxWindow* page)
{
  switch (id)
  {
  case ID_NOTEBOOK_EDITORS:
    if (wxExFileDialog(
      &((wxExSTC*)page)->GetFile()).ShowModalIfChanged() == wxID_CANCEL)
    {
      return false;
    }
  break;
  case ID_NOTEBOOK_PROJECTS:
    if (wxExFileDialog(
       (wxExListViewFile*)page).ShowModalIfChanged() == wxID_CANCEL)
    {
      return false;
    }
  break;
  }
  
  return wxExFrameWithHistory::AllowClose(id, page);
}

void DecoratedFrame::OnNotebook(wxWindowID id, wxWindow* page)
{
  wxExFrameWithHistory::OnNotebook(id, page);
  
  switch (id)
  {
    case ID_NOTEBOOK_EDITORS:
      ((wxExSTC*)page)->PropertiesMessage();
    break;
    case ID_NOTEBOOK_LISTS:
    break;
    case ID_NOTEBOOK_PROJECTS:
      wxExLogStatus(((wxExListViewFile*)page)->GetFileName());
      UpdateStatusBar((wxExListViewFile*)page);
    break;
    default:
      wxFAIL;
  }
}
