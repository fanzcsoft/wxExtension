////////////////////////////////////////////////////////////////////////////////
// Name:      test-Item.cpp
// Purpose:   Implementation for wxExtension cpp unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/item.h>
#include <wx/extension/managedframe.h>
#include "test.h"

void fixture::testItem()
{
  wxExItem item("label1", "hello string", "info1", "page1", 1, ITEM_STRING,
    true, true, 2);
  
  CPPUNIT_ASSERT( item.GetColumns() == -1); // page cols
  CPPUNIT_ASSERT( item.GetIsRequired());
  CPPUNIT_ASSERT( item.GetLabel() == "label1");
  CPPUNIT_ASSERT( item.GetPage() == "page1");
  CPPUNIT_ASSERT( item.GetType() == ITEM_STRING);
  CPPUNIT_ASSERT( item.GetWindow() == NULL);
  CPPUNIT_ASSERT( item.GetValue().IsNull());
  CPPUNIT_ASSERT(!item.IsRowGrowable());
  
  // setting value if window is NULL should have no effect.
  CPPUNIT_ASSERT(!item.SetValue(wxString("test")));
  CPPUNIT_ASSERT( item.GetValue().IsNull());

  wxGridSizer sizer(3);

  CPPUNIT_ASSERT( item.Layout(m_Frame, &sizer) != NULL);
  CPPUNIT_ASSERT( item.GetWindow() != NULL);
  CPPUNIT_ASSERT( item.GetValue() == "hello string");
  CPPUNIT_ASSERT( item.GetWindow()->GetWindowStyleFlag() == 1);
  
  CPPUNIT_ASSERT( item.SetValue(wxString("value changed")));
  CPPUNIT_ASSERT( item.GetValue() == "value changed");
  
  item.SetRowGrowable(true);
  CPPUNIT_ASSERT( item.IsRowGrowable());
  
  wxExItem item_int("int", ITEM_INT, "100");
  CPPUNIT_ASSERT( item_int.GetType() == ITEM_INT);
  CPPUNIT_ASSERT( item_int.Layout(m_Frame, &sizer) != NULL);
  CPPUNIT_ASSERT( item_int.GetWindow() != NULL);
  CPPUNIT_ASSERT( item_int.GetValue() == 100);
  CPPUNIT_ASSERT( item_int.SetValue(300));
  CPPUNIT_ASSERT( item_int.GetValue() == 300);
  
  wxExItem item_picker("picker", ITEM_FILEPICKERCTRL, "/usr/bin/git");
  CPPUNIT_ASSERT( item_picker.Layout(m_Frame, &sizer) != NULL);
  CPPUNIT_ASSERT( item_picker.GetValue() == "/usr/bin/git");
}