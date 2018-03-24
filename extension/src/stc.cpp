////////////////////////////////////////////////////////////////////////////////
// Name:      stc.cpp
// Purpose:   Implementation of class wxExSTC
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <wx/app.h>
#include <wx/config.h>
#include <wx/settings.h>
#include <wx/extension/stc.h>
#include <wx/extension/debug.h>
#include <wx/extension/defs.h>
#include <wx/extension/frd.h>
#include <wx/extension/indicator.h>
#include <wx/extension/lexers.h>
#include <wx/extension/managedframe.h>
#include <wx/extension/menu.h>
#include <wx/extension/path.h>
#include <wx/extension/printing.h>
#include <wx/extension/stcdlg.h>
#include <wx/extension/tokenizer.h>
#include <wx/extension/util.h>
#include <wx/extension/vcs.h>
#include <easylogging++.h>

#if wxUSE_GUI

int wxExSTC::m_Zoom = -1;

wxExSTC::wxExSTC(const std::string& text, const wxExSTCData& data)
  : wxExSTC(wxExPath(), data)
{
  if (!text.empty())
  {
    HexMode() ? m_HexMode.AppendText(text): SetText(text);
    GuessType();
  }
  
  m_Data.Inject();
}

wxExSTC::wxExSTC(const wxExPath& filename, const wxExSTCData& data)
  : wxStyledTextCtrl(
      data.Window().Parent(),
      data.Window().Id(), 
      data.Window().Pos(), 
      data.Window().Size(), 
      data.Window().Style(), 
      data.Window().Name())
  , m_Data(this, data)
  , m_AutoComplete(this)
  , m_vi(this)
  , m_File(this, data.Window().Name())
  , m_Link(this)
  , m_HexMode(wxExHexMode(this))
  , m_Frame(dynamic_cast<wxExManagedFrame*>(wxTheApp->GetTopWindow()))
  , m_Lexer(this)
{
  if (wxConfig::Get()->ReadBool("AllowSync", true)) Sync();
  
  if (!wxExLexers::Get()->GetLexers().empty())
  {
    m_DefaultFont = wxConfigBase::Get()->ReadObject(
      _("Default font"), wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));
  }
  
#ifdef __WXMSW__
  SetEOLMode(wxSTC_EOL_CRLF);
#elif __WXGTK__
  SetEOLMode(wxSTC_EOL_LF);
#else
  SetEOLMode(wxSTC_EOL_CR);
#endif

  SetAdditionalCaretsBlink(true);
  SetAdditionalCaretsVisible(true);
  SetAdditionalSelectionTyping(true);  
  SetBackSpaceUnIndents(true);
  SetMouseDwellTime(1000);

  SetMarginType(m_MarginLineNumber, wxSTC_MARGIN_NUMBER);
  SetMarginType(m_MarginDividerNumber, wxSTC_MARGIN_SYMBOL);
  SetMarginType(m_MarginFoldingNumber, wxSTC_MARGIN_SYMBOL);
  SetMarginType(m_MarginTextNumber, wxSTC_MARGIN_TEXT);

  SetMarginMask(m_MarginFoldingNumber, wxSTC_MASK_FOLDERS);

  SetMarginSensitive(m_MarginFoldingNumber, true);
  SetMarginSensitive(m_MarginTextNumber, true);

  SetMultiPaste(wxSTC_MULTIPASTE_EACH);
  SetMultipleSelection(true);
  
  if (m_Zoom == -1)
  {
    m_Zoom = GetZoom();
  }
  else
  {
    SetZoom(m_Zoom);
  }

  // we have our own popup
#if wxCHECK_VERSION(3,1,1)
  UsePopUp(wxSTC_POPUP_NEVER);
#else
  UsePopUp(false);
#endif

  BindAll();

  if (filename.GetStat().IsOk())
  {
    Open(filename, data);
  }
  else
  {
    m_File.FileLoad(filename);
    m_Lexer.Set(filename.GetLexer(), true); // allow fold
  }

  ConfigGet();
  Fold();
}

void wxExSTC::BuildPopupMenu(wxExMenu& menu)
{
  const std::string sel = GetSelectedText().ToStdString();

  if (GetCurrentLine() == 0 && !wxExLexers::Get()->GetLexers().empty())
  {
    menu.Append(ID_EDIT_SHOW_PROPERTIES, _("Properties"));
  }
    
  if (m_Data.Menu() & STC_MENU_OPEN_LINK)
  {
    std::string filename;

    if (LinkOpen(LINK_OPEN_BROWSER | LINK_CHECK))
    {
      menu.AppendSeparator();
      menu.Append(ID_EDIT_OPEN_BROWSER, _("&Open In Browser"));
    }
    else if (LinkOpen(LINK_OPEN | LINK_CHECK, &filename))
    {
      menu.AppendSeparator();
      menu.Append(ID_EDIT_OPEN_LINK, _("Open") + " " + filename);
    }
  }

#if wxCHECK_VERSION(3,1,0)
  if (GetEdgeMode() == wxSTC_EDGE_MULTILINE)
  {
    menu.AppendSeparator();
    menu.Append(ID_EDIT_EDGE_SET, _("Edge Column"));
    menu.Append(ID_EDIT_EDGE_CLEAR, _("Edge Column Reset"));
  }
#endif

  if (m_Data.Menu() & STC_MENU_OPEN_WWW && !sel.empty())
  {
    menu.AppendSeparator();
    menu.Append(ID_EDIT_OPEN_WWW, _("&Search"));
  }
  
  if (m_Data.Menu() & STC_MENU_DEBUG)
  {
    m_Frame->GetDebug()->AddMenu(&menu, true);
  }
  
  if ((m_Data.Menu() & STC_MENU_VCS) &&
       GetFileName().FileExists() && sel.empty() &&
       wxExVCS::DirExists(GetFileName()))
  {
    menu.AppendSeparator();
    menu.AppendVCS(GetFileName());
  }

  if (!m_vi.GetIsActive() && GetTextLength() > 0)
  {
    menu.AppendSeparator();
    menu.Append(wxID_FIND);

    if (!GetReadOnly())
    {
      menu.Append(wxID_REPLACE);
    }
  }

  menu.AppendSeparator();
  menu.AppendEdit();

  if (!GetReadOnly())
  {
    if (!sel.empty())
    {
      wxExMenu* menuSelection = new wxExMenu(menu.GetStyle());
      menuSelection->Append(ID_EDIT_UPPERCASE, _("&Uppercase\tF11"));
      menuSelection->Append(ID_EDIT_LOWERCASE, _("&Lowercase\tF12"));

      if (wxExGetNumberOfLines(sel) > 1)
      {
        wxExMenu* menuSort = new wxExMenu(menu.GetStyle());
        menuSort->Append(wxID_SORT_ASCENDING);
        menuSort->Append(wxID_SORT_DESCENDING);
        menuSelection->AppendSeparator();
        menuSelection->AppendSubMenu(menuSort, _("&Sort"));
      }

      menu.AppendSeparator();
      menu.AppendSubMenu(menuSelection, _("&Selection"));
    }
  }

  if (!GetReadOnly() && (CanUndo() || CanRedo()))
  {
    menu.AppendSeparator();
    if (CanUndo()) menu.Append(wxID_UNDO);
    if (CanRedo()) menu.Append(wxID_REDO);
  }

  // Folding if nothing selected, property is set,
  // and we have a lexer.
  if (
     sel.empty() && 
     GetProperty("fold") == "1" &&
     m_Lexer.IsOk() &&
    !m_Lexer.GetScintillaLexer().empty())
  {
    menu.AppendSeparator();
    menu.Append(ID_EDIT_TOGGLE_FOLD, _("&Toggle Fold\tCtrl+T"));
    menu.Append(ID_EDIT_FOLD_ALL, _("&Fold All Lines\tF9"));
    menu.Append(ID_EDIT_UNFOLD_ALL, _("&Unfold All Lines\tF10"));
  }
}

bool wxExSTC::CanCut() const
{
  return wxStyledTextCtrl::CanCut() && !GetReadOnly() && !HexMode();
}

bool wxExSTC::CanPaste() const
{
  return wxStyledTextCtrl::CanPaste() && !GetReadOnly() && !HexMode();
}


void wxExSTC::CheckBrace()
{
  if (HexMode())
  {
    m_HexMode.HighlightOther();
  }
  else if (!CheckBrace(GetCurrentPos()))
  {
    CheckBrace(GetCurrentPos() - 1);
  }
}

bool wxExSTC::CheckBrace(int pos)
{
  const int brace_match = BraceMatch(pos);

  if (brace_match != wxSTC_INVALID_POSITION)
  {
    BraceHighlight(pos, brace_match);
    return true;
  }
  else
  {
    BraceHighlight(wxSTC_INVALID_POSITION, wxSTC_INVALID_POSITION);
    return false;
  }
}

void wxExSTC::Clear()
{
  if (m_vi.GetIsActive() && GetSelectedText().empty())
  {
    (void)m_vi.Command(std::string(1, WXK_DELETE));
  }
  else
  {
    wxStyledTextCtrl::Clear();
  }
}

void wxExSTC::ClearDocument(bool set_savepoint)
{
  SetReadOnly(false);
  
  ClearAll();
  
  if (set_savepoint)
  {
    EmptyUndoBuffer();
    SetSavePoint();
  }
}

void wxExSTC::Copy()
{
  if (CanCopy()) 
  {
    wxStyledTextCtrl::Copy();
  }
}

void wxExSTC::Cut()
{
  if (CanCut()) 
  {
    if (m_vi.GetIsActive())
    {
      const wxCharBuffer b(GetSelectedTextRaw());
      m_vi.SetRegistersDelete(std::string(b.data(), b.length() - 1));
      m_vi.SetRegisterYank(std::string(b.data(), b.length() - 1));
    }
  
    wxStyledTextCtrl::Cut();
  }
}
  
bool wxExSTC::FileReadOnlyAttributeChanged()
{
  SetReadOnly(GetFileName().IsReadOnly()); // does not return anything
  wxLogStatus(_("Readonly attribute changed"));

  return true;
}

void wxExSTC::FileTypeMenu()
{
  wxMenu* menu = new wxMenu();

  // The order here should be the same as the defines for wxSTC_EOL_CRLF.
  // So the FindItemByPosition can work
  menu->AppendRadioItem(ID_EDIT_EOL_DOS, "&DOS");
  menu->AppendRadioItem(ID_EDIT_EOL_MAC, "&MAC");
  menu->AppendRadioItem(ID_EDIT_EOL_UNIX, "&UNIX");
  menu->AppendSeparator();
  wxMenuItem* hex = menu->AppendCheckItem(ID_EDIT_HEX, "&HEX");
  
  menu->FindItemByPosition(GetEOLMode())->Check();
  
  if (HexMode())
  {
    hex->Check();
  }

  PopupMenu(menu);
  
  delete menu;
}

void wxExSTC::Fold(bool foldall)
{
  if (
     GetProperty("fold") == "1" &&
     m_Lexer.IsOk() &&
    !m_Lexer.GetScintillaLexer().empty())
  {
    SetMarginWidth(m_MarginFoldingNumber, 
      wxConfigBase::Get()->ReadLong(_("Folding"), 0));
    SetFoldFlags(
      wxConfigBase::Get()->ReadLong(_("Fold flags"),
      wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | 
        wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED));
        
    if (
      foldall || 
      GetLineCount() > wxConfigBase::Get()->ReadLong(_("Auto fold"), 0))
    {
      FoldAll();
    }
  }
  else
  {
    SetMarginWidth(m_MarginFoldingNumber, 0);
  }
}
  
void wxExSTC::FoldAll()
{
  if (GetProperty("fold") != "1") return;

  const int current_line = GetCurrentLine();
  const bool xml = (m_Lexer.GetLanguage() == "xml");

  int line = 0;
  while (line < GetLineCount())
  {
    const int level = GetFoldLevel(line);
    const int last_child_line = GetLastChild(line, level);
    
    if (xml && (
        level == wxSTC_FOLDLEVELBASE + wxSTC_FOLDLEVELHEADERFLAG))
    {
      line++;
    }
    else if (last_child_line > line + 1)
    {
      if (GetFoldExpanded(line)) ToggleFold(line);
      line = last_child_line + 1;
    }
    else
    {
      line++;
    }
  }

  GotoLine(current_line);
}

const std::string wxExSTC::GetEOL() const
{
  switch (GetEOLMode())
  {
    case wxSTC_EOL_CR: return "\r";
    case wxSTC_EOL_CRLF: return "\r\n";
    case wxSTC_EOL_LF: return "\n";
    default: wxFAIL; break;
  }

  return "\r\n";
}

// Cannot be const because of GetSelectedText (not const in 2.9.4).
const std::string wxExSTC::GetFindString()
{
  const std::string selection = GetSelectedText().ToStdString();

  if (!selection.empty() && wxExGetNumberOfLines(selection) == 1)
  {
    bool alnum = true;
    
    // If regexp is true, then only use selected text if text does not
    // contain special regexp characters.
    if (GetSearchFlags() & wxSTC_FIND_REGEXP)
    {
      for (size_t i = 0; i < selection.size() && alnum; i++)
      {
        if (
          !isalnum(selection[i]) && 
           selection[i] != ' ' && 
           selection[i] != '.' && 
           selection[i] != '-' && 
           selection[i] != '_')
        {
          alnum = false;
        }
      }
    }

    if (alnum)
    {  
      wxExFindReplaceData::Get()->SetFindString(selection);
    }
  }

  return wxExFindReplaceData::Get()->GetFindString();
}


const std::string wxExSTC::GetWordAtPos(int pos) const
{
  const int word_start = 
    const_cast< wxExSTC * >( this )->WordStartPosition(pos, true);
  const int word_end = 
    const_cast< wxExSTC * >( this )->WordEndPosition(pos, true);

  if (word_start == word_end && word_start < GetTextLength())
  {
    const std::string word = 
      const_cast< wxExSTC * >( this )->GetTextRange(word_start, word_start + 1).ToStdString();

    return !isspace(word[0]) ? word: std::string();
  }
  else
  {
    return const_cast< wxExSTC * >( this )->GetTextRange(word_start, word_end).ToStdString();
  }
}

void wxExSTC::GuessType()
{
  // Get a small sample from this document to detect the file mode.
  const auto length = (!HexMode() ? GetTextLength(): m_HexMode.GetBuffer().size());
  const auto sample_size = (length > 255 ? 255: length);
  const std::string text((!HexMode() ? GetTextRange(0, sample_size).ToStdString(): 
    m_HexMode.GetBuffer().substr(0, sample_size)));
  const std::string text2((!HexMode() ? GetTextRange(length - sample_size, length).ToStdString(): 
    m_HexMode.GetBuffer().substr(length - sample_size, sample_size)));

  std::vector<std::string> v;  
  
  // If we have a modeline comment.
  if (
    m_vi.GetIsActive() && 
     (wxExMatch("vi: *(set [a-z0-9:= ]+)", text, v) > 0 ||
      wxExMatch("vi: *(set [a-z0-9:= ]+)", text2, v) > 0))
  {
    if (!m_vi.Command(":" + v[0] + "*")) // add * to indicate modelin
    {
      wxLogStatus("Could not apply vi settings");
    }
  }

  if      (text.find("\r\n") != std::string::npos) SetEOLMode(wxSTC_EOL_CRLF);
  else if (text.find("\n") != std::string::npos)   SetEOLMode(wxSTC_EOL_LF);
  else if (text.find("\r") != std::string::npos)   SetEOLMode(wxSTC_EOL_CR);
  else return; // do nothing

#if wxUSE_STATUSBAR
  wxExFrame::UpdateStatusBar(this, "PaneFileType");
#endif
}

bool wxExSTC::LinkOpen()
{
  return LinkOpen(LINK_OPEN | LINK_OPEN_BROWSER);
}

bool wxExSTC::LinkOpen(int mode, std::string* filename)
{
  const std::string sel = GetSelectedText().ToStdString();

  if (sel.size() > 200 || sel.find('\n') != std::string::npos)
  {
    return false;
  }

  const std::string text = (!sel.empty() ? sel: GetCurLine().ToStdString());

  if (mode & LINK_OPEN_BROWSER)
  {
    const wxExPath path(m_Link.GetPath(text, 
      wxExControlData().Line(sel.empty() ? -1 : -2)));
    
    if (!path.Path().string().empty()) 
    {
      if (!(mode & LINK_CHECK)) 
      {
        wxLaunchDefaultBrowser(path.Path().string());
      }

      return true;
    }
  }

  if (mode & LINK_OPEN)
  {
    wxExControlData data;

    const wxExPath path(m_Link.GetPath(text, data));
    
    if (!path.Path().string().empty()) 
    {
      if (filename != nullptr)
      {
        *filename = path.GetFullName();
      }
      else if (m_Frame != nullptr)
      {
        m_Frame->OpenFile(path, data);
      }
      else
      {
        Open(path, data);
      }

      return true;
    }
  }
  
  return false;
}

bool wxExSTC::MarkerDeleteAllChange()
{
  if (!wxExLexers::Get()->MarkerIsLoaded(m_MarkerChange))
  {
    return false;
  }
  
  MarkerDeleteAll(m_MarkerChange.GetNo());
  
  return true;
}
  
void wxExSTC::MarkModified(const wxStyledTextEvent& event)
{
  if (!wxExLexers::Get()->MarkerIsLoaded(m_MarkerChange))
  {
    return;
  }
  
  UseModificationMarkers(false);
  
  const int line = LineFromPosition(event.GetPosition());
  
  if (event.GetModificationType() & wxSTC_PERFORMED_UNDO)
  {
    if (event.GetLinesAdded() == 0)
    {
      MarkerDelete(line, m_MarkerChange.GetNo());
    }
    else
    {
      for (int i = 0; i < abs(event.GetLinesAdded()); i++)
      {
        MarkerDelete(line + 1, m_MarkerChange.GetNo());
      }
    }
      
    if (!IsModified())
    {
      MarkerDeleteAllChange();
    }
  }
  else if (
    (event.GetModificationType() & wxSTC_MOD_INSERTTEXT) ||
    (event.GetModificationType() & wxSTC_MOD_DELETETEXT))
  {
    if (event.GetLinesAdded() <= 0)
    {
      MarkerAdd(line, m_MarkerChange.GetNo());
    }
    else
    {
      for (int i = 0; i < event.GetLinesAdded(); i++)
      {
        MarkerAdd(line + i, m_MarkerChange.GetNo());
      }
    }
  }
    
  UseModificationMarkers(true);
}
  

void wxExSTC::OnIdle(wxIdleEvent& event)
{
  event.Skip();
  
  if (
    m_File.CheckSync() &&
    // the readonly flags bit of course can differ from file actual readonly mode,
    // therefore add this check
    !(m_Data.Flags() & STC_WIN_READ_ONLY) &&
      GetFileName().GetStat().IsReadOnly() != GetReadOnly())
  {
    FileReadOnlyAttributeChanged();
  }
}

void wxExSTC::OnStyledText(wxStyledTextEvent& event)
{
  MarkModified(event); 
  event.Skip();
}

bool wxExSTC::Open(const wxExPath& filename, const wxExSTCData& data)
{
  if (GetFileName() != filename && !m_File.FileLoad(filename))
  {
    return false;
  }

  m_Data = wxExSTCData(data).Window(wxExWindowData().Name(filename.Path().string()));
  m_Data.Inject();

  if (m_Frame != nullptr)
  {
    m_Frame->SetRecentFile(filename);
  }

  return true;
}

void wxExSTC::Paste()
{
  if (CanPaste())
  {
    wxStyledTextCtrl::Paste();
  }
}

bool wxExSTC::PositionRestore()
{
  if (m_vi.Mode().Visual())
  {
    SetCurrentPos(m_SavedPos);
  }
  else if (m_SavedSelectionStart != -1 && m_SavedSelectionEnd != -1)
  {
    SetSelection(m_SavedSelectionStart, m_SavedSelectionEnd);
    SetCurrentPos(m_SavedSelectionStart);
  }
  else if (m_SavedPos != -1)
  {
    SetSelection(m_SavedPos, m_SavedPos);
    SetCurrentPos(m_SavedPos);
  }
  else
  {
    return false;
  }
  
  EnsureCaretVisible();
  
  return true;
}
  
void wxExSTC::PositionSave()
{
  m_SavedPos = GetCurrentPos();

  if (!m_vi.Mode().Visual())
  {
    m_SavedSelectionStart = GetSelectionStart();  
    m_SavedSelectionEnd = GetSelectionEnd();
  }
}

#if wxUSE_PRINTING_ARCHITECTURE
void wxExSTC::Print(bool prompt)
{
  wxPrintData* data = wxExPrinting::Get()->GetHtmlPrinter()->GetPrintData();
  wxExPrinting::Get()->GetPrinter()->GetPrintDialogData().SetPrintData(*data);
  wxExPrinting::Get()->GetPrinter()->Print(this, new wxExPrintout(this), prompt);
}
#endif

#if wxUSE_PRINTING_ARCHITECTURE
void wxExSTC::PrintPreview(wxPreviewFrameModalityKind kind)
{
  wxPrintPreview* preview = new wxPrintPreview(
    new wxExPrintout(this), 
    new wxExPrintout(this));

  if (!preview->Ok())
  {
    delete preview;
    wxExSTCEntryDialog("There was a problem previewing.\n"
      "Perhaps your current printer is not set correctly?").ShowModal();
    return;
  }

  wxPreviewFrame* frame = new wxPreviewFrame(
    preview,
    this,
    wxExPrintCaption(GetName().ToStdString()));

  frame->InitializeWithModality(kind);
  frame->Show();
}
#endif

void wxExSTC::PropertiesMessage(long flags)
{
  wxExLogStatus(GetFileName(), flags);
  
#if wxUSE_STATUSBAR
  if (flags != STAT_SYNC)
  {
    wxExFrame::UpdateStatusBar(this, "PaneFileType");
    wxExFrame::UpdateStatusBar(this, "PaneLexer");
    wxExFrame::UpdateStatusBar(this, "PaneMode");
  }
  
  wxExFrame::UpdateStatusBar(this, "PaneInfo");
#endif

  if (!(flags & STAT_SYNC) && m_Frame != nullptr)
  {
    const wxString file = GetName() + 
      (GetReadOnly() ? " [" + _("Readonly") + "]": wxString());
    
    m_Frame->SetTitle(!file.empty() ? file: wxTheApp->GetAppName());
  }
}

int wxExSTC::ReplaceAll(
  const std::string& find_text,
  const std::string& replace_text)
{
  int selection_from_end = 0;

  if (SelectionIsRectangle() || wxExGetNumberOfLines(GetSelectedText().ToStdString()) > 1)
  {
    TargetFromSelection();
    selection_from_end = GetLength() - GetTargetEnd();
  }
  else
  {
    SetTargetStart(0);
    SetTargetEnd(GetLength());
  }

  int nr_replacements = 0;
  SetSearchFlags(-1);
  BeginUndoAction();

  while (SearchInTarget(find_text) != -1)
  {
    bool skip_replace = false;

    // Check that the target is within the rectangular selection.
    // If not just continue without replacing.
    if (SelectionIsRectangle())
    {
      const int line = LineFromPosition(GetTargetStart());
      const int start_pos = GetLineSelStartPosition(line);
      const int end_pos = GetLineSelEndPosition(line);
      const int length = GetTargetEnd() - GetTargetStart();

      if (start_pos == wxSTC_INVALID_POSITION ||
          end_pos == wxSTC_INVALID_POSITION ||
          GetTargetStart() < start_pos ||
          GetTargetStart() + length > end_pos)
      {
        skip_replace = true;
      }
    }

    if (!skip_replace)
    {
      if (HexMode())
      {
        m_HexMode.ReplaceTarget(replace_text);
      }
      else
      {
        wxExFindReplaceData::Get()->UseRegEx() ?
          ReplaceTargetRE(replace_text):
          ReplaceTarget(replace_text);
      }

      nr_replacements++;
    }

    SetTargetStart(GetTargetEnd());
    SetTargetEnd(GetLength() - selection_from_end);
    
    if (GetTargetStart() >= GetTargetEnd())
    {
      break;
    }
  }

  EndUndoAction();

  wxLogStatus(_("Replaced: %d occurrences of: %s"),
    nr_replacements, find_text.c_str());

  return nr_replacements;
}

bool wxExSTC::ReplaceNext(bool find_next)
{
  return ReplaceNext(
    wxExFindReplaceData::Get()->GetFindString(),
    wxExFindReplaceData::Get()->GetReplaceString(),
    -1,
    find_next);
}

bool wxExSTC::ReplaceNext(
  const std::string& find_text, 
  const std::string& replace_text,
  int find_flags,
  bool find_next)
{
  if (!GetSelectedText().empty())
  {
    TargetFromSelection();
  }
  else
  {
    SetTargetStart(GetCurrentPos());
    SetTargetEnd(GetLength());
    SetSearchFlags(find_flags);
    if (SearchInTarget(find_text) == -1) return false;
  }

  if (HexMode())
  {
    m_HexMode.ReplaceTarget(replace_text);
  }
  else
  {
    wxExFindReplaceData::Get()->UseRegEx() ?
      ReplaceTargetRE(replace_text):
      ReplaceTarget(replace_text);
  }

  FindNext(find_text, find_flags, find_next);
  
  return true;
}

 
void wxExSTC::ResetMargins(wxExSTCMarginFlags flags)
{
  if (flags & STC_MARGIN_FOLDING) SetMarginWidth(m_MarginFoldingNumber, 0);
  if (flags & STC_MARGIN_DIVIDER) SetMarginWidth(m_MarginLineNumber, 0);
  if (flags & STC_MARGIN_LINENUMBER) SetMarginWidth(m_MarginDividerNumber, 0);
  if (flags & STC_MARGIN_TEXT) SetMarginWidth(m_MarginTextNumber, 0);
}

void wxExSTC::SelectNone()
{
  if (SelectionIsRectangle())
  {
    // SetSelection does not work.
    CharRight();
    CharLeft();
  }
  else
  {
    // The base styledtextctrl version uses scintilla, sets caret at 0.
    SetSelection(GetCurrentPos(), GetCurrentPos());
  }
}

bool wxExSTC::SetIndicator(const wxExIndicator& indicator, int start, int end)
{
  if (!wxExLexers::Get()->IndicatorIsLoaded(indicator))
  {
    return false;
  }

  SetIndicatorCurrent(indicator.GetNo());
  IndicatorFillRange(start, end - start);
  
  return true;
}

void wxExSTC::SetSearchFlags(int flags)
{
  if (flags == -1)
  {
    flags = 0;
    
    wxExFindReplaceData* frd = wxExFindReplaceData::Get();
    if (frd->UseRegEx()) 
    {
      flags |= wxSTC_FIND_REGEXP;
#if wxCHECK_VERSION(3,1,1)
      flags |= wxSTC_FIND_CXX11REGEX;
#endif
    }
    if (frd->MatchWord()) flags |= wxSTC_FIND_WHOLEWORD;
    if (frd->MatchCase()) flags |= wxSTC_FIND_MATCHCASE;
  }

  wxStyledTextCtrl::SetSearchFlags(flags);
}

void wxExSTC::SetText(const std::string& value)
{
  ClearDocument();

  AddTextRaw((const char *)value.c_str(), value.length());

  DocumentStart();

  // Do not allow the text specified to be undone.
  EmptyUndoBuffer();
}

void wxExSTC::ShowLineNumbers(bool show)
{
  SetMarginWidth(m_MarginLineNumber, 
    show ? wxConfigBase::Get()->ReadLong(_("Line number"), 0): 0);
}

bool wxExSTC::ShowVCS(const wxExVCSEntry* vcs)
{
  if (vcs->GetMarginWidth() <= 0)
  {
    return false;
  }

  if (vcs->GetStdOut().empty())
  {
    return false;
  }

  wxExTokenizer tkz(vcs->GetStdOut(), "\r\n");

  int begin, end;
  bool number = true;
  bool found = false;

  try
  {
    begin = std::stoi(vcs->GetPosBegin());
    end = std::stoi(vcs->GetPosEnd());
  }
  catch (std::exception& e)
  {
    number = false;
  }

  int line = 0;
  while (tkz.HasMoreTokens())
  {
    const std::string text(tkz.GetNextToken());

    if (!number)
    {
      begin = text.find(vcs->GetPosBegin());
      end = text.find(vcs->GetPosEnd());
    }

    if (begin != std::string::npos && end != std::string::npos)
    {
      MarginSetText(line, text.substr(begin + 1, end - begin - 1));
      found = true;
    }
 
    line++;
  }

  if (found)
  {
    SetMarginWidth(m_MarginTextNumber, vcs->GetMarginWidth());
  }

  return found;
}

void wxExSTC::Sync(bool start)
{
  start ?
    Bind(wxEVT_IDLE, &wxExSTC::OnIdle, this):
    (void)Unbind(wxEVT_IDLE, &wxExSTC::OnIdle, this);
}

void wxExSTC::Undo()
{
  wxStyledTextCtrl::Undo();
  m_HexMode.Undo();
}

void wxExSTC::UseModificationMarkers(bool use)
{
  if (use)
    Bind(wxEVT_STC_MODIFIED, &wxExSTC::OnStyledText, this);
  else
    Unbind(wxEVT_STC_MODIFIED, &wxExSTC::OnStyledText, this);
}

void wxExSTC::WordLeftRectExtend() 
{
  const int repeat = GetCurrentPos() - WordStartPosition(GetCurrentPos(), false);
  
  for (int i = 0; i < repeat ; i++)
  {
    CharLeftRectExtend();
  }
}

void wxExSTC::WordRightRectExtend() 
{
  const int repeat = WordEndPosition(GetCurrentPos(), false) - GetCurrentPos();
  
  for (int i = 0; i < repeat; i++)
  {
    CharRightRectExtend();
  }
}
#endif // wxUSE_GUI
