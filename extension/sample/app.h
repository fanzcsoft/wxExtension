////////////////////////////////////////////////////////////////////////////////
// Name:      app.h
// Purpose:   Declaration of sample classes for wxExtension
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016
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
  DECLARE_NO_COPY_CLASS(wxExSampleApp)
};

#if wxUSE_GRID
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
  /// Override the OnFile.
  virtual bool OnFile(const std::string& file) override;
  wxExGrid* m_Grid;
};
#endif

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
#if wxUSE_GRID
  wxExGrid* m_Grid;
#endif
  wxExListView* m_ListView;
  wxExNotebook* m_Notebook;
  wxExProcess* m_Process;
  wxExSTC* m_STC;
  wxExSTC* m_STCLexers;
  wxExShell* m_Shell;

  long m_FlagsSTC = 0;
  wxExStatistics <int> m_Statistics;

  DECLARE_NO_COPY_CLASS(wxExSampleFrame)
};
