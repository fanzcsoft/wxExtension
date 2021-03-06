////////////////////////////////////////////////////////////////////////////////
// Name:      stcfile.cpp
// Purpose:   Implementation of class wxExSTCFile
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <pugixml.hpp>
#include <wx/extension/stcfile.h>
#include <wx/extension/filedlg.h>
#include <wx/extension/lexers.h>
#include <wx/extension/log.h>
#include <wx/extension/path.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h> // for STAT_ etc.
#include <easylogging++.h>

void CheckWellFormed(wxExSTC* stc, const wxExPath& fn)
{
  if (fn.GetLexer().GetLanguage() == "xml")
  {
    if (const pugi::xml_parse_result result = 
      pugi::xml_document().load_file(fn.Path().string().c_str());
      !result)
    {
      wxExXmlError(fn, &result, stc);
    }
  }
}

wxExSTCFile::wxExSTCFile(wxExSTC* stc, const std::string& filename)
  : m_STC(stc)
  , m_PreviousLength(0)
{
  if (!filename.empty())
  {
    Assign(wxExPath(filename));
  }
}

bool wxExSTCFile::DoFileLoad(bool synced)
{
  if (
   GetContentsChanged() &&
   wxExFileDialog(this).ShowModalIfChanged() == wxID_CANCEL)
  {
    return false;
  }

  // Synchronizing by appending only new data only works for log files.
  // Other kind of files might get new data anywhere inside the file,
  // we cannot sync that by keeping pos. 
  // Also only do it for reasonably large files.
  const bool isLog = (GetFileName().GetExtension().find(".log") == 0);
  
  m_STC->UseModificationMarkers(false);

  ReadFromFile(
    synced &&
    isLog &&
    m_STC->GetTextLength() > 1024);

  if (!synced)
  {
    // ReadFromFile might already have set the lexer using a modeline.
    if (m_STC->GetLexer().GetScintillaLexer().empty())
    {
      m_STC->GetLexer().Set(GetFileName().GetLexer(), true);
    }

    wxLogStatus(_("Opened") + ": " + GetFileName().Path().string());
    VLOG(1) << "opened: " << GetFileName().Path().string();
  }
  
  m_STC->PropertiesMessage(synced ? STAT_SYNC: STAT_DEFAULT);
  m_STC->UseModificationMarkers(true);
  
  CheckWellFormed(m_STC, GetFileName());
  
  return true;
}

void wxExSTCFile::DoFileNew()
{
  m_STC->SetName(GetFileName().Path().string());
  m_STC->PropertiesMessage();
  m_STC->ClearDocument();
  m_STC->GetLexer().Set(GetFileName().GetLexer(), true); // allow fold
}

void wxExSTCFile::DoFileSave(bool save_as)
{
  if (m_STC->GetHexMode().Active())
  {
    Write(m_STC->GetHexMode().GetBuffer());
  }
  else
  {
    Write(m_STC->GetTextRaw());
  }
  
  if (save_as)
  {
    m_STC->SetReadOnly(GetFileName().IsReadOnly());
    m_STC->GetLexer().Set(GetFileName().GetLexer());
    m_STC->SetName(GetFileName().Path().string());
  }
  
  m_STC->MarkerDeleteAllChange();
  
  wxLogStatus(_("Saved") + ": " + GetFileName().Path().string());
  VLOG(1) << "saved: " << GetFileName().Path().string();
  
  CheckWellFormed(m_STC, GetFileName());
}

bool wxExSTCFile::GetContentsChanged() const 
{
  return m_STC->GetModify();
}

void wxExSTCFile::ReadFromFile(bool get_only_new_data)
{
  const bool pos_at_end = (m_STC->GetCurrentPos() >= m_STC->GetTextLength() - 1);

  int startPos, endPos;
  m_STC->GetSelection(&startPos, &endPos);

  wxFileOffset offset = 0;

  if (m_PreviousLength < Length() && get_only_new_data)
  {
    offset = m_PreviousLength;
  }

  if (offset == 0)
  {
    m_STC->ClearDocument();
  }

  m_PreviousLength = Length();

  if (const auto buffer = Read(offset); !m_STC->GetHexMode().Active())
  {
    m_STC->Allocate(buffer->length());
    
    get_only_new_data ? 
      m_STC->AppendTextRaw((const char *)buffer->data(), buffer->length()):
      m_STC->AddTextRaw((const char *)buffer->data(), buffer->length());
  }
  else
  {
    m_STC->GetHexMode().AppendText(std::string(buffer->data(), buffer->length()));
  }

  if (get_only_new_data)
  {
    if (pos_at_end)
    {
      m_STC->DocumentEnd();
    }
  }
  else
  {
    m_STC->GuessType();
    m_STC->DocumentStart();
  }

  if (startPos != endPos)
  {
    m_STC->SetSelection(startPos, endPos);
  }
  
  m_STC->EmptyUndoBuffer();
}

void wxExSTCFile::ResetContentsChanged()
{
  m_STC->SetSavePoint();
}
