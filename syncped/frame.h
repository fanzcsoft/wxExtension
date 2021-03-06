////////////////////////////////////////////////////////////////////////////////
// Name:      frame.h
// Purpose:   Declaration of class Frame
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/extension/notebook.h>
#include <wx/extension/process.h>
#include <wx/extension/report/dirctrl.h>
#include <wx/extension/report/listview.h>
#include "support.h"

class App;
class EditorsNotebook;

class Frame : public DecoratedFrame
{
public:
  explicit Frame(App* app);
protected:
  void OnCommand(wxCommandEvent& event);
  void OnUpdateUI(wxUpdateUIEvent& event);
private:
  virtual wxExListView* Activate(wxExListType type, const wxExLexer* lexer = nullptr) override;
  virtual bool ExecExCommand(wxExExCommand& command) override;
  virtual wxExListViewFile* GetProject() override;
  virtual bool IsOpen(const wxExPath& filename) override;
  virtual void OnCommandItemDialog(wxWindowID dialogid, const wxCommandEvent& event) override;
  virtual wxExSTC* OpenFile(
    const wxExPath& filename,
    const wxExSTCData& stc_data = wxExSTCData()) override;
  virtual wxExSTC* OpenFile(
    const wxExPath& filename, 
    const wxExVCSEntry& vcs, 
    const wxExSTCData& stc_data = wxExSTCData()) override;
  virtual wxExSTC* OpenFile(
    const wxExPath& filename, 
    const std::string& text, 
    const wxExSTCData& stc_data = wxExSTCData()) override;
  virtual void PrintEx(wxExEx* ex, const std::string& text) override;
  virtual wxExProcess* Process(const std::string& command) override;
  virtual wxExSTC* RestorePage(const std::string& key) override;
  virtual bool SaveCurrentPage(const std::string& key) override;
  virtual void StatusBarClicked(const std::string& pane) override;
  virtual void StatusBarClickedRight(const std::string& pane) override;
  virtual void SyncAll() override;
  virtual void SyncCloseAll(wxWindowID id) override;
  
  void AddPaneHistory();
  void AddPaneProcess();
  void AddPaneProjects();

  const long m_PaneFlag = 
    wxAUI_NB_DEFAULT_STYLE |
    wxAUI_NB_CLOSE_ON_ALL_TABS |
    wxAUI_NB_CLOSE_BUTTON |
    wxAUI_NB_WINDOWLIST_BUTTON |
    wxAUI_NB_SCROLL_BUTTONS;
  const wxString m_ProjectWildcard = 
    _("Project Files") + " (*.prj)|*.prj";

  bool m_Maximized {false};
  int m_NewProjectNo {1}, m_SplitId {1};

  EditorsNotebook *m_Editors {nullptr};

  wxCheckBox *m_CheckBoxDirCtrl {nullptr}, *m_CheckBoxHistory {nullptr};
  wxExGenericDirCtrl* m_DirCtrl {nullptr};
  wxExListViewWithFrame* m_History {nullptr};
  wxExNotebook *m_Lists {nullptr}, *m_Projects {nullptr};
  wxExProcess* m_Process {nullptr};
  wxExSTC* m_asciiTable {nullptr};
  std::string m_SavedPage;

  DECLARE_EVENT_TABLE()
};
