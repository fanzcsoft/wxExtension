////////////////////////////////////////////////////////////////////////////////
// Name:      file.h
// Purpose:   Declaration of class wxExFile
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <wx/file.h>
#include <wx/extension/filename.h>
#include <wx/extension/stat.h>

/// Adds several File* methods to wxFile. All the File* methods update
/// the wxExStat member. Also takes care of synchronization,
/// all you have to do is call CheckSync once in a while.
class WXDLLIMPEXP_BASE wxExFile
{
public:
  /// Default constructor.
  /// The open_file parameter specifies the behaviour of FileLoad and
  /// FileSave, if true, the file is opened before calling DoFileLoad
  /// and DoFileSave, if false the file is not opened.
  /// That might be useful if you do not use the wxFile for loading
  /// and saving (as with XML documents), but still want to use the
  /// virtual file interface.
  wxExFile(bool open_file = true);

  /// Opens a file with a filename.
  wxExFile(
    const wxFileName& filename,
    wxFile::OpenMode mode = wxFile::read,
    bool open_file = true);
  
  /// Copy constructor.
  wxExFile(const wxExFile& rhs);
  
  /// Assignment operator.
  wxExFile& operator=(const wxExFile& f);

  /// Destructor, closes file if it was opened.
  virtual ~wxExFile() {};

  /// Checks whether this file can be synced, and 
  /// syncs (invokes DoFileLoad) the file if so.
  /// Returns true if this file was synced.
  bool CheckSync();

  /// Closes file if it was opened.
  bool Close() {return IsOpened() && m_File->Close();};

  /// Sets the filename member, opens the file if asked for,
  /// invokes DoFileLoad, and closes the file again.
  bool FileLoad(const wxExFileName& filename);

  /// Sets the filename member and invokes DoFileNew.
  void FileNew(const wxExFileName& filename);

  /// Sets the filename member if filename is ok, opens the file if asked for,
  /// invokes DoFileSave, and closes the file again.
  bool FileSave(const wxExFileName& filename = wxExFileName());

  /// Returns whether contents have been changed.
  virtual bool GetContentsChanged() const {return false;};

  /// Returns the file name.
  const auto & GetFileName() const {return m_FileName;}

  /// Returns whether file is opened.
  bool IsOpened() const {return m_File->IsOpened();};

  /// Returns length.
  wxFileOffset Length() const {return m_File->Length();};
  
  /// Opens current filename.
  bool Open(wxFile::OpenMode mode = wxFile::read, int access = wxS_DEFAULT)
    {return m_File->Open(m_FileName.GetFullPath(), mode, access);};
  
  /// Opens specified file.
  bool Open(const wxString &filename, 
    wxFile::OpenMode mode = wxFile::read, int access = wxS_DEFAULT)
    {return m_File->Open(filename, mode, access);};
  
  /// Reads this file into a buffer.
  const wxCharBuffer* Read(wxFileOffset seek_position = 0);

  /// Resets contents changed.
  virtual void ResetContentsChanged() {;};
  
  /// Writes file from buffer.
  bool Write(const wxCharBuffer& buffer) {
    return m_File->Write(buffer.data(), buffer.length()) == buffer.length();};
  
  /// Writes file from string.
  bool Write(const wxString &s) {return m_File->Write(s);}; 
protected:
  /// Assigns the filename.
  /// Does not open the file.
  void Assign(const wxExFileName& filename) {
    m_FileName = filename;
    m_Stat = filename.GetFullPath();};

  /// Invoked by FileLoad, allows you to load the file.
  /// The file is already opened, so you can call Read.
  /// If synced is true, this call was a result of
  /// CheckSync and not of FileLoad.
  virtual bool DoFileLoad(bool synced = false) {return false;};

  /// Invoked by FileNew, allows you to make a new (empty) file.
  virtual void DoFileNew() {;};

  /// Invoked by FileSave, allows you to save the file.
  /// The file is already opened.
  virtual void DoFileSave(bool save_as = false) {;};
private:
  bool Get(bool synced);
  bool MakeAbsolute() {
    return 
      m_FileName.MakeAbsolute() &&
      m_FileName.m_Stat.Sync(m_FileName.GetFullPath()) &&
      m_Stat.Sync(m_FileName.GetFullPath());};

  bool m_IsLoaded;
  bool m_OpenFile;
  
  std::unique_ptr<wxCharBuffer> m_Buffer;
  std::unique_ptr<wxFile> m_File;

  wxExFileName m_FileName;
  wxExStat m_Stat; // used for syncing, no public access
};
