////////////////////////////////////////////////////////////////////////////////
// Name:      vcscommand.cpp
// Purpose:   Implementation of wxExVCSCommand class
// Author:    Anton van Wezenbeek
// Created:   2010-08-27
// RCS-ID:    $Id$
// Copyright: (c) 2010 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/vcscommand.h>

int wxExVCSCommand::m_Instances = 0;

wxExVCSCommand::wxExVCSCommand()
  : m_Command()
  , m_SubMenu()
  , m_SubMenuIsCommand(false)
  , m_No(0)
  , m_Type(VCS_COMMAND_IS_BOTH) 
{
}

wxExVCSCommand::wxExVCSCommand(
  const wxString& command,
  const wxString& type,
  const wxString& submenu,
  const wxString& subcommand)
  : m_Command(command)
  , m_SubMenu(!submenu.empty() ? submenu: subcommand)
  , m_SubMenuIsCommand(!subcommand.empty())
  , m_No(m_Instances++)
  , m_Type(From(type))
{
}
  
int wxExVCSCommand::From(const wxString& type) const
{
  long command = VCS_COMMAND_IS_BOTH;
  
  if (type.Contains("popup"))
  {
    command = VCS_COMMAND_IS_POPUP;
  }
  else if (type.Contains("main"))
  {
    command = VCS_COMMAND_IS_MAIN;
  }
  
  long flags = (type.Contains("separator") ? VCS_COMMAND_SEPARATOR: 0);
  
  return command | flags;
}

const wxString wxExVCSCommand::GetCommand(
  bool include_subcommand,
  bool include_accelerators) const
{
  wxString command = m_Command;

  if (m_SubMenuIsCommand && include_subcommand)
  {
    command += " " + m_SubMenu;
  }

  if (command.Contains("&") && !include_accelerators)
  {
    command.Replace("&", wxEmptyString);
  }

  return command;
}

bool wxExVCSCommand::IsAdd() const
{
  return 
    GetCommand(false) == "add";
}

bool wxExVCSCommand::IsBlame() const
{
  return 
    GetCommand(false) == "blame" ||
    GetCommand(false) == "print";
}

bool wxExVCSCommand::IsCheckout() const
{
  return 
    GetCommand(false) == "checkout" ||
    GetCommand(false) == "co";
}

bool wxExVCSCommand::IsCommit() const
{
  return 
    GetCommand(false) == "commit" ||
    GetCommand(false) == "ci" ||
    GetCommand(false) == "delta";
}

bool wxExVCSCommand::IsDiff() const
{
  return 
    GetCommand(false).Contains("diff");
}

bool wxExVCSCommand::IsHelp() const
{
  return 
    GetCommand(false) == "help";
}

bool wxExVCSCommand::IsHistory() const
{
  return 
    GetCommand(false) == "log" ||
    GetCommand(false) == "prs" ||
    GetCommand(false) == "prt";
}

bool wxExVCSCommand::IsOpen() const
{
  return
    GetCommand(false) == "cat" ||
    GetCommand(false) == "get" ||
    IsBlame() ||
    IsDiff() ||
    IsHistory();
}

bool wxExVCSCommand::IsUpdate() const
{
  return 
    GetCommand(false) == "update" ||
    GetCommand(false) == "up";
}
