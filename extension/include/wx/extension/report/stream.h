////////////////////////////////////////////////////////////////////////////////
// Name:      stream.h
// Purpose:   Declaration of class 'wxExStreamToListView'
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/extension/stream.h>

class wxExFrameWithHistory;
class wxExListView;

/// Offers a wxExStream with reporting to a listview.
class WXDLLIMPEXP_BASE wxExStreamToListView : public wxExStream
{
public:
  /// Constructor.
  wxExStreamToListView(
    const wxExPath& filename,
    const wxExTool& tool);

  /// Sets up the tool.
  static bool SetupTool(
    /// tool to use
    const wxExTool& tool, 
    /// frame
    wxExFrameWithHistory* frame,
    /// listview to which is reported, if nullptr,
    /// calls Activate on frame to find report
    wxExListView* report = nullptr);
private:
  /// The comment type.
  enum wxExCommentType
  {
    COMMENT_NONE = 0,  ///< no comment
    COMMENT_BEGIN,     ///< begin of comment
    COMMENT_END,       ///< end of comment
    COMMENT_BOTH,      ///< begin or end of comment
    COMMENT_INCOMPLETE ///< within a comment
  };

  /// The syntax type.
  enum wxExSyntaxType
  {
    SYNTAX_NONE = 0, ///< no syntax
    SYNTAX_ONE,      ///< syntax according to comment begin1 and end1
    SYNTAX_TWO       ///< syntax according to comment begin2 and end2
  };

  wxExCommentType CheckCommentSyntax(
    const std::string& syntax_begin,
    const std::string& syntax_end,
    const std::string& text) const;

  /// Returns the actual begin of comment, depending on the syntax type.
  const std::string CommentBegin() const {
    return (m_SyntaxType == SYNTAX_NONE || m_SyntaxType == SYNTAX_ONE) ?
      GetFileName().GetLexer().GetCommentBegin() :
      GetFileName().GetLexer().GetCommentBegin2();};

  /// Returns the last end of comment detected, depending on the last syntax type.
  const std::string CommentEnd() const {
    return (m_LastSyntaxType == SYNTAX_NONE || m_LastSyntaxType == SYNTAX_ONE) ?
      GetFileName().GetLexer().GetCommentEnd() :
      GetFileName().GetLexer().GetCommentEnd2();};

  /// Check whether specified text result in a comment.
  wxExCommentType CheckForComment(const std::string& text);
  void CommentStatementEnd();
  void CommentStatementStart();
  std::string Context(const std::string& line, int pos) const;
  
  // Implement interface from wxExStream.
  virtual bool Process(std::string& line, size_t line_no) override;
  virtual bool ProcessBegin() override;
  virtual void ProcessEnd() override;
  virtual void ProcessMatch(const std::string& line, size_t line_no, int pos) override;
  
  static wxExListView* m_Report;
  static wxExFrameWithHistory* m_Frame;

  bool m_IsCommentStatement = false;
  bool m_IsString = false;

  int m_ContextSize;
  
  wxExSyntaxType m_LastSyntaxType = SYNTAX_NONE;
  wxExSyntaxType m_SyntaxType = SYNTAX_NONE;
};
