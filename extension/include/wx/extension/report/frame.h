////////////////////////////////////////////////////////////////////////////////
// Name:      frame.h
// Purpose:   Include file for wxExFrameWithHistory class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <set>
#include <wx/bitmap.h> // for auibar
#include <wx/aui/auibar.h>
#include <wx/extension/filehistory.h>
#include <wx/extension/listview.h> // for wxExListViewFileName::wxExListType 
#include <wx/extension/managedframe.h>
#include <wx/extension/report/defs.h>

class wxExConfigDialog;
class wxExListView;
class wxExListViewFile;

/// Adds file and project history support to wxExManagedFrame.
/// It also sets a change indicator in the title of the frame if applicable.
/// Finally it adds find in files and selection dialogs.
class WXDLLIMPEXP_BASE wxExFrameWithHistory : public wxExManagedFrame
{
public:
  /// Extra open flags.
  enum
  {
    WIN_IS_PROJECT = 0x0100 ///< open as project
  };

  /// Constructor.
  /// Default it gives file history support to be used from the file menu.
  /// So you should call UseFileHistory somewhere to set it up.
  /// Default it does not use a recent project file.
  wxExFrameWithHistory(wxWindow* parent,
    wxWindowID id,
    const wxString& title,
    size_t maxFiles = 9,
    size_t maxProjects = 0,
    int style = wxDEFAULT_FRAME_STYLE);

  /// This method is called to activate a certain listview.
  /// Default it returns NULL.
  virtual wxExListViewFileName* Activate(
    wxExListViewFileName::wxExListType WXUNUSED(list_type), 
    const wxExLexer* WXUNUSED(lexer) = NULL) {
    return NULL;};
    
  /// Finds (or replaces) in specified files.
  /// Returns false if dialog was shown, and cancelled,
  /// or SetupTool returned false.
  bool FindInFiles(
    /// the files
    const std::vector< wxString > & files,
    /// ID_TOOL_REPORT_FIND or ID_TOOL_REPORT_REPLACE
    int id,
    /// Default shows a dialog.
    bool show_dialog = true,
    /// report for output
    wxExListView* report = NULL);

  /// Shows a modal find (or replace) in files dialog.
  /// Returns result from ShowModal.
  int FindInFilesDialog(
    /// ID_TOOL_REPORT_FIND or ID_TOOL_REPORT_REPLACE
    int id,
    /// add file types selection as well
    bool add_in_files = false);
  
  /// Returns caption for FindInFilesDialog.
  const wxString GetFindInCaption(int id) const;
  
  /// If there is a project somewhere, 
  /// your implementation should return that one.
  /// Default it returns NULL.
  virtual wxExListViewFile* GetProject() {return NULL;};

  /// Returns project history.
  wxExFileHistory& GetProjectHistory() {return m_ProjectHistory;};
  
  /// Greps for text.
  /// The base directory is the directory for the current stc
  /// component, if available.
  /// Returns false if line contains too few arguments, or 
  /// setup failed.
  bool Grep(
    /// text folder extension
    const wxString& line);
  
  /// Override OnCommandConfigDialog for add, find and replace in files.
  virtual void OnCommandConfigDialog(
    wxWindowID dialogid,
    int commandid = wxID_APPLY) override;
    
  /// Called if the notebook changed page.
  /// Adds page as recently used (file or project, depending on dynamic cast).
  virtual void OnNotebook(wxWindowID id, wxWindow* page);

  /// Updates file history.
  /// Returns true if history was updated.
  virtual bool SetRecentFile(const wxString& file) override;

  /// Updates project history.
  /// Returns true if history was updated.
  bool SetRecentProject(const wxString& project) {
    return m_ProjectHistory.SetRecentFile(project);};

  /// Uses specified history list, and adds all elements from file history
  /// to the list.
  void UseFileHistoryList(wxExListView* list);
protected:
  /// Access to file history list, 
  /// if you use this as a page in a notebook,
  /// you might want prevent closing it.
  wxExListView* GetFileHistoryList() {return m_FileHistoryList;};
  
  void OnIdle(wxIdleEvent& event);
private:
  void CreateDialogs();
  void FindInFiles(wxWindowID dialogid);
  const wxString GetFindReplaceInfoText(bool replace = false) const;

  wxExConfigDialog* m_FiFDialog;
  wxExConfigDialog* m_RiFDialog;
  wxExListView* m_FileHistoryList;
  wxExFileHistory m_ProjectHistory;

  const wxString m_TextInFiles;
  const wxString m_TextInFolder;
  const wxString m_TextRecursive;
  
  std::set < wxString > m_Info;
};
