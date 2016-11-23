////////////////////////////////////////////////////////////////////////////////
// Name:      dir.cpp
// Purpose:   Implementation of class wxExDir and wxExDirOpenFile
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/dir.h>
#include <wx/extension/filename.h>
#include <wx/extension/frame.h>
#include <wx/extension/util.h>

class wxExDirTraverser: public wxDirTraverser
{
public:
  explicit wxExDirTraverser(wxExDir& dir)
    : m_Dir(dir){;}
  
  virtual wxDirTraverseResult OnDir(const wxString& dirname) override
  {
    if (m_Dir.Cancelled())
    {
      return wxDIR_STOP;
    }

    if (m_Dir.GetFlags() & wxDIR_DIRS)
    {
      if (!m_Dir.OnDir(dirname.ToStdString())) return wxDIR_STOP;
    }

    return wxDIR_CONTINUE;
  }

  virtual wxDirTraverseResult OnFile(const wxString& filename) override
  {
    if (m_Dir.Cancelled())
    {
      return wxDIR_STOP;
    }

    if (wxExMatchesOneOf(filename.ToStdString(), m_Dir.GetFileSpec()))
    {
      if (!m_Dir.OnFile(filename.ToStdString())) return wxDIR_STOP;
    }

    return wxDIR_CONTINUE;
  }
private:
  wxExDir& m_Dir;
};

wxExDir::wxExDir(const std::string& fullpath, const std::string& filespec, int flags)
  : wxDir(fullpath)
  , m_FileSpec(filespec)
  , m_Flags(flags)
{
}

int wxExDir::FindFiles()
{
  if (!IsOpened())
  {
    wxLogError("Could not open: " + GetName());
    return -1;
  }
  else if (Running())
  {
    wxLogStatus(_("Busy"));
    return -1;
  }

  Start();

  wxExDirTraverser traverser(*this);
  
  // Using m_FileSpec only works if it 
  // contains single spec (*.h), wxDir does not handle multi specs (*.cpp; *.h).
  const size_t retValue = Traverse(
    traverser, 
    (m_FileSpec.find(";") != std::string::npos ? std::string(): m_FileSpec), 
    m_Flags);

  Stop();

  return retValue;
}

#if wxUSE_GUI
wxExDirOpenFile::wxExDirOpenFile(wxExFrame* frame,
  const std::string& fullpath, 
  const std::string& filespec, 
  long file_flags,
  int dir_flags)
  : wxExDir(fullpath, filespec, dir_flags)
  , m_Frame(frame)
  , m_Flags(file_flags)
{
}

bool wxExDirOpenFile::OnFile(const std::string& file)
{
  m_Frame->OpenFile(file, 0, std::string(), m_Flags);
  return true;
}
#endif
