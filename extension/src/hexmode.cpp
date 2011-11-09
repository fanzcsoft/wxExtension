////////////////////////////////////////////////////////////////////////////////
// Name:      hexmode.cpp
// Purpose:   Implementation of class wxExHexModeLine
// Author:    Anton van Wezenbeek
// Copyright: (c) 2011 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/hexmode.h>
#include <wx/extension/stc.h>

const wxFileOffset bytes_per_line = 16;
const wxFileOffset each_hex_field = 3;
const wxFileOffset space_between_fields = 1;
const wxFileOffset start_hex_field = 9;
const wxFileOffset start_ascii_field =
  start_hex_field + each_hex_field * bytes_per_line + space_between_fields;
  
wxExHexModeLine::wxExHexModeLine(wxExSTC* stc)
  : m_Line(stc->GetCurLine())
  , m_LineNo(stc->GetCurrentLine())
  , m_Index(stc->GetColumn(stc->GetCurrentPos()))
  , m_STC(stc)
{
}  

wxExHexModeLine::wxExHexModeLine(wxExSTC* stc, int line, int pos)
  : m_STC(stc)
{
  Set(line, pos); 
}

void wxExHexModeLine::AppendText(const wxCharBuffer& buffer)
{
  m_STC->m_HexBuffer += buffer;
  
  const wxFileOffset mid_in_hex_field = 7;

  wxString text;

  // Allocate space for the string.
  text.Alloc(
    // offset:      (9 + 1 + 1) * length / 16 bytes, 
    (start_hex_field + 1 + 1) * buffer.length() / bytes_per_line + 
    // hex field:   3 * length 
    each_hex_field * buffer.length() + 
    // ascii field: just the length
    buffer.length());

  wxFileOffset start = m_STC->GetLength();
  
  // Using wxString::Format here asserts (wxWidgets-2.9.1).
  char field_offset[9];
  
  for (
    wxFileOffset offset = 0; 
    offset < buffer.length(); 
    offset += bytes_per_line)
  {
    long count = buffer.length() - offset;
    count =
      (bytes_per_line < count ? bytes_per_line : count);

    wxString field_hex, field_ascii;

    for (register wxFileOffset byte = 0; byte < count; byte++)
    {
      const char c = buffer.data()[offset + byte];

      field_hex += wxString::Format("%02x ", (unsigned char)c);

      // Print an extra space.
      if (byte == mid_in_hex_field)
      {
        field_hex += ' ';
      }

      field_ascii += Printable(c);
    }

    // The extra space if we ended too soon.
    if (count <= mid_in_hex_field)
    {
      field_hex += ' ';
    }
    
    sprintf(field_offset, "%08lx ", (unsigned long)start + (unsigned long)offset);
    
    const wxString field_spaces = wxString(
      ' ', 
      (bytes_per_line - count)* each_hex_field);

    text +=  
      field_offset + 
      field_hex +
      field_spaces +
      field_ascii +
      m_STC->GetEOL();
  }

  m_STC->AppendText(text);
}

int wxExHexModeLine::Convert(int offset) const
{
  return atoi(m_Line.Mid(0, start_hex_field)) + offset;
}

int wxExHexModeLine::GetAsciiField() const
{
  if (m_Line.GetChar(m_Index) != ' ')
  {
    int space = 0;

    if (m_Index >= 
      start_hex_field + 
      space_between_fields + 
      (bytes_per_line * each_hex_field) / 2)
    {
      space++;
    }

    const int offset = (m_Index - (start_hex_field + space)) / each_hex_field;

    return start_ascii_field + offset;
  }
  
  return wxSTC_INVALID_POSITION;
}

int wxExHexModeLine::GetByte() const
{
  if (m_Index > start_ascii_field + bytes_per_line)
  {
    return wxSTC_INVALID_POSITION;
  }
  else if (m_Index >= start_ascii_field)
  {
    return Convert(m_Index - start_ascii_field);
  }
  else if (m_Index >= start_hex_field)
  {
    if (m_Line.GetChar(m_Index) != ' ')
    {
      int space = 0;

      if (m_Index >= 
        start_hex_field + 
        space_between_fields + 
        (bytes_per_line * each_hex_field) / 2)
      {
        space++;
      }

      return Convert((m_Index - (start_hex_field + space)) / each_hex_field);
    }
  }

  return wxSTC_INVALID_POSITION;
}
  
int wxExHexModeLine::GetHexField() const
{
  const int offset = m_Index - start_ascii_field;
  int space = 0;

  if (m_Index == start_ascii_field + bytes_per_line)
  {
    space--;
  }
  else if (m_Index >= start_ascii_field + bytes_per_line / 2)
  {
    space++;
  }

  return start_hex_field + each_hex_field * offset + space;
}

bool wxExHexModeLine::IsAsciiField() const
{
  return 
    m_Index >= start_ascii_field && 
    m_Index < start_ascii_field + bytes_per_line;
}

bool wxExHexModeLine::IsHexField() const
{
  return 
    m_Index >= start_hex_field &&
    m_Index < start_ascii_field;
}

bool wxExHexModeLine::IsReadOnly() const
{
  if (IsAsciiField())
  {
    return false;
  }
  else if (IsHexField())
  {
    if (m_Line.GetChar(m_Index) != ' ')
    {
      return false;
    }
  }
  
  return true;
}

int wxExHexModeLine::OtherField() const
{
  if (IsAsciiField())
  {
    return GetHexField();
  }
  else if (IsHexField())
  {
    return GetAsciiField();
  }
  else
  {
    return wxSTC_INVALID_POSITION;
  }
}

wxUniChar wxExHexModeLine::Printable(int c) const
{
  // We do not want the \n etc. to be printed,
  // as that disturbs the hex view field.
  if (c != 0 && c != '\r' && c != '\n' && c != '\t')
  {
    return c;
  }
  else
  {
    // Therefore print an ordinary ascii char.
    return '.';
  }
}
  
bool wxExHexModeLine::Replace(const wxUniChar& c)
{
  if (IsReadOnly())
  {
    return false;
  }
  
  const int pos = m_STC->PositionFromLine(m_LineNo);
  
  wxUniChar val = c;
  
  if (IsAsciiField())
  {
    // replace ascii field with value
    m_STC->wxStyledTextCtrl::Replace(
      pos + m_Index, 
      pos + m_Index + 1, 
      c);
      
    // replace hex field with code
    const wxString code = wxString::Format("%02x", c);
    
    m_STC->wxStyledTextCtrl::Replace(
      pos + OtherField(), 
      pos + OtherField() + 2, 
      code);
  }
  else if (IsHexField())
  {
    // hex text should be entered.
    if (!isxdigit(c))
    {
      return false;
    }
      
    // replace hex field with value
    m_STC->wxStyledTextCtrl::Replace(
      pos + m_Index, 
      pos + m_Index + 1,
      c);
        
    // replace ascii field with code
    char str[3];
    
    if (m_Line[m_Index + 1] == ' ')
    {
      str[0] = m_Line[m_Index - 1];
      str[1] = c;
    }
    else
    {
      str[0] = c;
      str[1] = m_Line[m_Index];
    }
    
    str[2] = '\0';
    
    int code;
    sscanf(str, "%x", &code);
    
    m_STC->wxStyledTextCtrl::Replace(
      pos + OtherField(), 
      pos + OtherField() + 1, 
      Printable(code));
      
    val = code;
  }
  else
  {
    return false;
  }
      
  m_STC->m_HexBuffer[GetByte()] = val;
  
  return true;
}

void wxExHexModeLine::Set(int line, int pos)
{
  m_Line = m_STC->GetLine(line);
  m_LineNo = line;
  m_Index = m_STC->GetColumn(pos);
}
