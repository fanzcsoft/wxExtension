////////////////////////////////////////////////////////////////////////////////
// Name:      configitem.cpp
// Purpose:   Implementation of wxExConfigItem class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/clrpicker.h> // for wxColourPickerWidget
#include <wx/checklst.h>
#include <wx/config.h>
#include <wx/filepicker.h>
#include <wx/fontpicker.h>
#include <wx/hyperlink.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>
#include <wx/window.h>
#include <wx/extension/configitem.h>
#include <wx/extension/frd.h>
#include <wx/extension/listview.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>

#if wxUSE_GUI

#define PERSISTENT(READ, TYPE, DEFAULT)                            \
{                                                                  \
  if (save)                                                        \
    wxConfigBase::Get()->Write(GetLabel(), GetValue().As<TYPE>()); \
  else                                                             \
    SetValue(wxConfigBase::Get()->READ(GetLabel(), DEFAULT));      \
}                                                                  \

bool Update(wxExFindReplaceData* frd, wxCheckListBox* clb, int item, bool save, bool value)
{
  const wxString field(clb->GetString(item));
  
  if (field == frd->GetTextMatchWholeWord())
  {
    !save ? clb->Check(item, frd->MatchWord()): frd->SetMatchWord(value);
  }
  else if (field == frd->GetTextMatchCase())
  {
    !save ? clb->Check(item, frd->MatchCase()): frd->SetMatchCase(value);
  }
  else if (field == frd->GetTextRegEx())
  {
    !save ? clb->Check(item, frd->UseRegEx()): frd->SetUseRegEx(value);
  }
  else if (field == frd->GetTextSearchDown())
  {
    frd->SetFlags(value ? wxFR_DOWN: ~wxFR_DOWN);
  }
  else
  {
    return false;
  }

  return true;
}

wxFlexGridSizer* wxExConfigItem::Layout(
  wxWindow* parent, wxSizer* sizer, bool readonly, wxFlexGridSizer* fgz)
{
  wxFlexGridSizer* ret = wxExItem::Layout(parent, sizer, readonly, fgz);
  
  ToConfig(false);
  
  return ret;
}
  
bool wxExConfigItem::ToConfig(bool save) const
{
  switch (GetType())
  {
    case ITEM_CHECKBOX:         PERSISTENT(ReadBool, bool, false); break;
    case ITEM_CHECKLISTBOX_BIT: PERSISTENT(ReadLong, long, 0); break;
    case ITEM_COLOUR:           PERSISTENT(ReadObject, wxColour, *wxWHITE); break;
    case ITEM_DIRPICKERCTRL:    PERSISTENT(Read, wxString, GetLabel()); break;
    case ITEM_FLOAT:            PERSISTENT(ReadDouble, double, 0); break;
    case ITEM_FONTPICKERCTRL:   PERSISTENT(ReadObject, wxFont, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)); break;
    case ITEM_INT:              PERSISTENT(ReadLong, long, 0); break;
    case ITEM_LISTVIEW_FOLDER:  PERSISTENT(Read, wxString, ""); break;
    case ITEM_SLIDER:           PERSISTENT(ReadLong, int, ((wxSlider* )GetWindow())->GetMin()); break;
    case ITEM_SPINCTRL: 
    case ITEM_SPINCTRL_HEX:     PERSISTENT(ReadLong, int, ((wxSpinCtrl* )GetWindow())->GetMin()); break;
    case ITEM_SPINCTRL_DOUBLE:  PERSISTENT(ReadDouble, double, ((wxSpinCtrlDouble* )GetWindow())->GetMin()); break;
    case ITEM_STC:              PERSISTENT(Read, wxString, ""); break;
    case ITEM_STRING:           PERSISTENT(Read, wxString, ""); break;
    case ITEM_TOGGLEBUTTON:     PERSISTENT(ReadBool, bool, false); break;

    case ITEM_CHECKLISTBOX_BOOL:
      {
      wxCheckListBox* clb = (wxCheckListBox*)GetWindow();

      for (size_t i = 0; i < clb->GetCount(); i++)
      {
        if (!Update(wxExFindReplaceData::Get(), clb, i, save, clb->IsChecked(i)))
        {
          if (save)
            wxConfigBase::Get()->Write(clb->GetString(i), clb->IsChecked(i));
          else
            clb->Check(i, wxConfigBase::Get()->ReadBool(clb->GetString(i), false));
        }
      }}
      break;

    case ITEM_COMBOBOX:
    case ITEM_COMBOBOXDIR:
      {
      wxComboBox* cb = (wxComboBox*)GetWindow();

      if (save)
      {
        const auto& l = wxExComboBoxToList(cb, m_MaxItems);

        if (GetLabel() == wxExFindReplaceData::Get()->GetTextFindWhat())
        {
          wxExFindReplaceData::Get()->SetFindStrings(l);
        }
        else if (GetLabel() == wxExFindReplaceData::Get()->GetTextReplaceWith())
        {
          wxExFindReplaceData::Get()->SetReplaceStrings(l);
        }
        else
        {
          wxExListToConfig(l, GetLabel());
        }
      }
      else
      {
        wxExComboBoxFromList(cb, wxExListFromConfig(GetLabel()));
      }
      }
      break;

    case ITEM_FILEPICKERCTRL:
      if (save)
      {
        wxConfigBase::Get()->Write(GetLabel(), GetValue().As<wxString>());
      }
      else
      {
        wxString initial;

#ifdef __WXGTK__
        initial = "/usr/bin/" + GetLabel();
        if (!wxFileExists(initial))
        {
          initial.clear();
        }
#endif
        SetValue(wxConfigBase::Get()->Read(GetLabel(), initial));
      }
      break;

    case ITEM_RADIOBOX:
      {
      wxRadioBox* rb = (wxRadioBox*)GetWindow();
      if (save)
      {
        for (const auto& b : GetChoices())
        {
          if (b.second == rb->GetStringSelection())
          {
            wxConfigBase::Get()->Write(GetLabel(), b.first);
          }
        }
      }
      else
      {
        const auto c = GetChoices().find(wxConfigBase::Get()->ReadLong(GetLabel(), 0));

        if (c != GetChoices().end())
        {
          rb->SetStringSelection(c->second);
        }
      }
      }
      break;

    case ITEM_USER:
      if (m_UserWindowToConfig != NULL)
      {
        return (m_UserWindowToConfig)(GetWindow(), save);
      }
      break;
      
    default:
      // the other types have no persistent info
      return false;
      break;
  }

  return true;
}
#endif // wxUSE_GUI
