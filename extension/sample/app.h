////////////////////////////////////////////////////////////////////////////////
// Name:      app.h
// Purpose:   Declaration of sample classes for wxExtension
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018
////////////////////////////////////////////////////////////////////////////////

#include <wx/extension/app.h>
#include <wx/extension/dir.h>
#include <wx/extension/grid.h>
#include <wx/extension/listview.h>
#include <wx/extension/managedframe.h>
#include <wx/extension/notebook.h>
#include <wx/extension/process.h>
#include <wx/extension/shell.h>
#include <wx/extension/statistics.h>
#include <wx/extension/stc.h>

/// Derive your application from wxExApp.
class wxExSampleApp: public wxExApp
{
public:
  /// Constructor.
  wxExSampleApp() {}
private:
  virtual bool OnInit() override;
};

/// Use wxExDir.
class wxExSampleDir: public wxExDir
{
public:
  /// Constructor.
  wxExSampleDir(
    const std::string& fullpath, 
    const std::string& findfiles, 
    wxExGrid* grid);
private:
  virtual bool OnFile(const wxExPath& file) override;
  wxExGrid* m_Grid;
};

/// Use wxExManagedFrame.
class wxExSampleFrame: public wxExManagedFrame
{
public:
  /// Constructor.
  wxExSampleFrame();
  virtual wxExListView* GetListView() override {return m_ListView;};
  virtual void OnCommandItemDialog(
    wxWindowID id, 
    const wxCommandEvent& event) override;
protected:
  void OnCommand(wxCommandEvent& event);
private:
  wxExGrid* m_Grid;
  wxExListView* m_ListView;
  wxExNotebook* m_Notebook;
  wxExProcess* m_Process;
  wxExSTC* m_STC;
  wxExSTC* m_STCLexers;
  wxExShell* m_Shell;

  long m_FlagsSTC = 0;
  wxExStatistics <int> m_Statistics;
};
