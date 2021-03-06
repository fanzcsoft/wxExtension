////////////////////////////////////////////////////////////////////////////////
// Name:      toolbar.h
// Purpose:   Declaration of wxExToolBar classes
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <wx/aui/auibar.h> 
#include <wx/extension/window-data.h> 

class wxExManagedFrame;

/// Offers a toolbar together with stock art.
/// Default no controls are added, you have to call AddControls to do that.
class WXDLLIMPEXP_BASE wxExToolBar : public wxAuiToolBar
{
public:
  /// Constructor.
  wxExToolBar(wxExManagedFrame* frame, 
    const wxExWindowData& data = wxExWindowData().Style(wxAUI_TB_DEFAULT_STYLE));

  /// Adds automatic naming (for stock menu id's) and 
  /// art id for toolbar normal items.
  wxAuiToolBarItem* AddTool(int toolId,
    const wxString& label = wxEmptyString,
    const wxBitmap& bitmap = wxNullBitmap,
    const wxString& shortHelp = wxEmptyString,
    wxItemKind kind = wxITEM_NORMAL);

  /// Adds the standard controls.
  /// This adds a file open, save and print and find control.
  void AddControls(bool realize = true);
  
  /// Returns the frame.
  wxExManagedFrame* GetFrame() {return m_Frame;};
private:
  wxExManagedFrame* m_Frame;
};

/// Offers a find toolbar, containing a find ctrl, up and down arrows
/// and checkboxes.
/// The find ctrl allows you to find in an wxExSTC
/// component on the specified wxExFrame.
class WXDLLIMPEXP_BASE wxExFindToolBar : public wxExToolBar
{
public:
  /// Constructor.
  wxExFindToolBar(wxExManagedFrame* frame, 
    const wxExWindowData& data = wxExWindowData().Style(wxAUI_TB_DEFAULT_STYLE));
};

/// Offers a options toolbar, containing checkboxes.
class WXDLLIMPEXP_BASE wxExOptionsToolBar : public wxExToolBar
{
public:
  /// Constructor.
  wxExOptionsToolBar(wxExManagedFrame* frame, 
    const wxExWindowData& data = wxExWindowData().Style(wxAUI_TB_DEFAULT_STYLE));
  
  /// Adds the standard checkboxes.
  /// This is a hex and a sync checkbox, and a process check box
  /// if you called PrepareOutput previously.
  void AddControls(bool realize = true);
  
  /// Updates checkbox state.
  /// Returns true if checkbox was found.
  bool Update(const wxString& name, bool show);
private:
  std::vector<wxCheckBox*> m_CheckBoxes;
};
