/******************************************************************************\
* File:          support.h
* Purpose:       Declaration of support classes
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _SUPPORT_H
#define _SUPPORT_H

#include <wx/extension/report/frame.h>

/// Adds a menu and small things to base class.
class FrameWithHistory : public wxExFrameWithHistory
{
public:
  /// Constructor.
  FrameWithHistory();
protected:
  wxExMenu* GetVCSMenu() {return m_MenuVCS;};
private:
  // Interface from wxExFrame.
  virtual bool AllowClose(wxWindowID id, wxWindow* page);
  virtual void OnNotebook(wxWindowID id, wxWindow* page);

  wxExMenu* m_MenuVCS;
};
#endif
