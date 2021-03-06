////////////////////////////////////////////////////////////////////////////////
// Name:      dir.h
// Purpose:   Include file for wxExDirWithListView and wxExDirTool classes
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/extension/dir.h>
#include <wx/extension/stream-statistics.h>
#include <wx/extension/tool.h>

/// Offers a wxExDir with tool support.
/// RunTool is FindFiles invoked on all matching files.
class WXDLLIMPEXP_BASE wxExDirTool : public wxExDir
{
public:
  /// Constructor, provide your tool and a path.
  /// SetupTool should already be called.
  wxExDirTool(const wxExTool& tool,
    const wxExPath& fullpath,
    const std::string& filespec = std::string(),
    int flags = DIR_DEFAULT);
    
  /// Returns the statistics.
  auto & GetStatistics() {return m_Statistics;};
protected:  
  virtual bool OnFile(const wxExPath& file) override;
private:    
  wxExStreamStatistics m_Statistics;
  const wxExTool m_Tool;
};

class wxExListView;

/// Offers a wxExDir with reporting to a listview.
/// All matching files and folders are added as listitem to the listview.
class wxExDirWithListView : public wxExDir
{
public:
  /// Constructor, provide your listview and a path.
  wxExDirWithListView(wxExListView* listview,
    const wxExPath& fullpath,
    const std::string& filespec = std::string(),
    int flags = DIR_DEFAULT);
protected:
  virtual bool OnDir(const wxExPath& dir) override;
  virtual bool OnFile(const wxExPath& file) override;
private:
  wxExListView* m_ListView;
};
