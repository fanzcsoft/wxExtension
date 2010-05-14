/******************************************************************************\
* File:          test.h
* Purpose:       Declaration of classes for wxExtension cpp unit testing
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
* Created:       za 17 jan 2009 11:51:20 CET
*
* Copyright (c) 2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EXTESTUNIT_H
#define _EXTESTUNIT_H

#include <TestFixture.h>
#include <TestSuite.h>

/// CppUnit test suite.
class wxExTestSuite : public CppUnit::TestSuite
{
public:
  /// Default constructor.
  wxExTestSuite();
};

/// Derive your application from wxExApp.
class wxExTestApp: public wxExApp
{
public:
  /// Constructor.
  wxExTestApp() {}
private:
  /// Override the OnInit.
  virtual bool OnInit();
};

/// CppUnit app test fixture.
/// These classes require either an wxExApp object, or wx to be initialized.
class wxExAppTestFixture : public CppUnit::TestFixture
{
public:
  /// Default constructor.
  wxExAppTestFixture() : TestFixture() {}; 
  
  /// Destructor.
 ~wxExAppTestFixture() {};

  /// From TestFixture.
  /// Set up context before running a test.
  virtual void setUp() {};

  /// Clean up after the test run.
  virtual void tearDown() {};

  void testGlobal();
  void testGrid();
  void testListView();
  void testMenu();
  void testNotebook();
  void testSTCFile();
  void testSTCShell();
  void testUtil();
  void testVCS();
};
#endif
