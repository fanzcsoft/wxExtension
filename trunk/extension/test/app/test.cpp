/******************************************************************************\
* File:          test.cpp
* Purpose:       Implementation for wxExtension cpp unit testing
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
* Created:       za 17 jan 2009 11:51:20 CET
*
* Copyright (c) 2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <TestCaller.h>
#include <wx/config.h>
#include <wx/extension/extension.h>
#include "test.h"

#define TEST_FILE "./test.h"
#define TEST_BIN "./test.bin"

void wxExAppTestFixture::testGlobal()
{
  CPPUNIT_ASSERT(wxExFindReplaceData::Get() != NULL);
  CPPUNIT_ASSERT(wxExLexers::Get() != NULL);
  CPPUNIT_ASSERT(wxExLog::Get() != NULL);
  CPPUNIT_ASSERT(wxExPrinting::Get() != NULL);
  CPPUNIT_ASSERT(wxExVCS::Get() != NULL);
  CPPUNIT_ASSERT(wxExTool::Get() != NULL);
}

void wxExAppTestFixture::testGrid()
{
  wxExGrid* grid = new wxExGrid(wxTheApp->GetTopWindow());
  CPPUNIT_ASSERT(grid->CreateGrid(5, 5));
  grid->SetGridCellValue(wxGridCellCoords(0, 0), "test");
  grid->SelectAll();
  CPPUNIT_ASSERT(!grid->GetSelectedCellsValue().empty());
  CPPUNIT_ASSERT(m_Grid->GetCellValue(0, 0) == "test");
  grid->SetCellsValue(wxGridCellCoords(0, 0), "test1\ttest2\ntest3\ttest4\n");
  CPPUNIT_ASSERT(grid->GetCellValue(0, 0) == "test1");
}

void wxExAppTestFixture::testListView()
{
  wxExListView* listView = new wxExListView(wxTheApp->GetTopWindow());
}

void wxExAppTestFixture::testNotebook()
{
  wxExNotebook* notebook = new wxExNotebook(wxTheApp->GetTopWindow(), NULL);
  // (parent should not be NULL)
  wxWindow* page1 = new wxWindow(wxTheApp->GetTopWindow(), wxID_ANY);
  wxWindow* page2 = new wxWindow(wxTheApp->GetTopWindow(), wxID_ANY);
  CPPUNIT_ASSERT(notebook->AddPage(page1, "key1") != NULL);
  CPPUNIT_ASSERT(notebook->AddPage(page2, "key2") != NULL);
  CPPUNIT_ASSERT(notebook->AddPage(page1, "key1") == NULL);
  CPPUNIT_ASSERT(notebook->GetKeyByPage(page1) == "key1");
  CPPUNIT_ASSERT(notebook->GetPageByKey("key1") == page1);
  CPPUNIT_ASSERT(notebook->SetPageText("key1", "keyx", "hello"));
  CPPUNIT_ASSERT(notebook->GetPageByKey("keyx") == page1);
  CPPUNIT_ASSERT(notebook->DeletePage("keyx"));
  CPPUNIT_ASSERT(notebook->GetPageByKey("keyx") == NULL);
}

void wxExAppTestFixture::testSTCFile()
{
  wxExSTCFile* stc = new wxExSTCFile(wxTheApp->GetTopWindow(), wxExFileName(TEST_FILE));
  // do the same test as with wxExFile in base for a binary file
  CPPUNIT_ASSERT(stc->Open(wxExFileName(TEST_BIN)));
  CPPUNIT_ASSERT(stc->GetFlags() == 0);
  CPPUNIT_ASSERT(stc->GetMenuFlags() == wxExSTCFile::STC_MENU_DEFAULT);
  const wxCharBuffer& buffer = stc->GetTextRaw();
  wxLogMessage(buffer.data());
  CPPUNIT_ASSERT(buffer.length() == 40);
  CPPUNIT_ASSERT(!stc->MacroIsRecording());
  CPPUNIT_ASSERT(!stc->MacroIsRecorded());
  stc->StartRecord();
  CPPUNIT_ASSERT( stc->MacroIsRecording());
  CPPUNIT_ASSERT(!stc->MacroIsRecorded());
  stc->StopRecord();
  CPPUNIT_ASSERT(!stc->MacroIsRecording());
  CPPUNIT_ASSERT(!stc->MacroIsRecorded()); // still no macro
}

void wxExAppTestFixture::testSTCShell()
{
  wxExSTCShell* shell = new wxExSTCShell(wxTheApp->GetTopWindow());
  shell->Prompt("test1");
  shell->Prompt("test2");
  shell->Prompt("test3");
  shell->Prompt("test4");
  // Prompting does not add a command to history.
  CPPUNIT_ASSERT(!shell->GetHistory().Contains("test4"));
  // Post 3 'a' chars to the shell, and check whether it comes in the history.
  wxKeyEvent event(wxEVT_CHAR);
  event.m_keyCode = 97; // one char 'a'
  wxPostEvent(shell, event);
  wxPostEvent(shell, event);
  wxPostEvent(shell, event);
  event.m_keyCode = WXK_RETURN;
  wxPostEvent(shell, event);
  // The event queue for shell is not yet processed, so next will assert anyway.
  //CPPUNIT_ASSERT(m_STCShell->GetHistory().Contains("aaa"));
}

void wxExAppTestFixture::testUtil()
{
  CPPUNIT_ASSERT(wxExClipboardAdd("test"));
  CPPUNIT_ASSERT(wxExClipboardGet() == "test");
  CPPUNIT_ASSERT(wxExGetNumberOfLines("test\ntest\n") == 3);
  CPPUNIT_ASSERT(wxExGetLineNumberFromText("test on line: 1200") == 1200);

  // Only usefull if the lexers file was present
  if (wxExLexers::Get()->Count() > 0)
  {
    wxConfigBase::Get()->Write(_("Purpose"), "hello test");
    const wxExFileName fn(TEST_FILE);
    const wxString header = wxExHeader().Get(&fn);
    CPPUNIT_ASSERT(header.Contains("hello test"));
  }
  else
  {
    wxLogMessage("No lexers available");
  }

  wxExLog::Get()->Log("hello from wxExtension test");
  CPPUNIT_ASSERT(!wxExMatchesOneOf(wxFileName("test.txt"), "*.cpp"));
  CPPUNIT_ASSERT(wxExMatchesOneOf(wxFileName("test.txt"), "*.cpp;*.txt"));
  CPPUNIT_ASSERT(wxExSkipWhiteSpace("\n\tt \n    es   t\n") == "t es t");
  CPPUNIT_ASSERT(!wxExTranslate("hello @PAGENUM@ from @PAGESCNT@", 1, 2).Contains("@"));
}

void wxExAppTestFixture::testVCS()
{
  wxExVCS vcs(wxExVCS::VCS_INFO, TEST_FILE);
  // There is a problem in wxExecute inside wxExVCS::Execute.
//  CPPUNIT_ASSERT(m_VCS->Execute() != -1);
//  CPPUNIT_ASSERT(!m_VCS->GetOutput().empty());
  CPPUNIT_ASSERT(vcs.DirExists(wxFileName(TEST_FILE)));
}

wxExTestSuite::wxExTestSuite()
  : CppUnit::TestSuite("wxExtension test suite")
{
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testGlobal",
    &wxExAppTestFixture::testGlobal));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testGrid",
    &wxExAppTestFixture::testGrid));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testListView",
    &wxExAppTestFixture::testListView));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testNotebook",
    &wxExAppTestFixture::testNotebook));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testMethods",
    &wxExAppTestFixture::testMethods));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testSTCFile",
    &wxExAppTestFixture::testSTCFile));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testSTCShell",
    &wxExAppTestFixture::testSTCShell));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testUtil",
    &wxExAppTestFixture::testUtil));
    
  addTest(new CppUnit::TestCaller<wxExAppTestFixture>(
    "testVCS",
    &wxExAppTestFixture::testVCS));
}
