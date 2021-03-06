////////////////////////////////////////////////////////////////////////////////
// Name:      tool.h
// Purpose:   Declaration of wxExTool classes
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <string>
#include <wx/extension/defs.h>

/// This class defines our tool info.
/// It is stored in the tool info map available from the wxExTool class.
class WXDLLIMPEXP_BASE wxExToolInfo
{
public:
  /// Default constructor.
  wxExToolInfo(
    const std::string& info = std::string(),
    const std::string& text = std::string(),
    const std::string& helptext = std::string())
    : m_Info(info)
    , m_HelpText(helptext)
    , m_Text(text){};

  /// Returns the helptext.
  const auto & GetHelpText() const {return m_HelpText;};

  /// Returns the info.
  const auto & GetInfo() const {return m_Info;};

  /// Returns the text.
  const auto & GetText() const {return m_Text;};
private:
  std::string m_HelpText;
  std::string m_Info;
  std::string m_Text;
};

template <class T> class wxExStatistics;

/// Offers tool methods and contains the tool info's.
/// A tool with non empty text is used by wxExMenu::AppendTools.
class WXDLLIMPEXP_BASE wxExTool
{
public:
  /// Default constructor, specify the wxExToolId to use.
  wxExTool(int id = -1);

  /// Adds your own info to the tool.
  /// If you use a %d in the info string, it is replaced by GetStatistics
  /// with the Actions Completed element.
  void AddInfo(
    int tool_id,
    const std::string& info,
    const std::string& text = std::string(),
    const std::string& helptext = std::string()) {
    m_ToolInfo[tool_id] = wxExToolInfo(info, text, helptext);};

  /// Returns the tool id.
  int GetId() const {return m_Id;};

  /// Returns all the tool info.
  const auto & GetToolInfo() const {return m_ToolInfo;};

  /// Returns info about current tool.
  const std::string Info() const;

  /// Returns info about current tool using specified statistics.
  const std::string Info(const wxExStatistics<int>* stat) const;
  
  /// Is this tool a find type.
  bool IsFindType() const {
    return m_Id == ID_TOOL_REPORT_FIND || m_Id == ID_TOOL_REPLACE;}

  /// Is this tool a report type.
  bool IsReportType() const {
    return m_Id > ID_TOOL_REPORT_FIRST && m_Id < ID_TOOL_REPORT_LAST;}
private:
  const int m_Id;
  static std::map < int, wxExToolInfo > m_ToolInfo;
};
