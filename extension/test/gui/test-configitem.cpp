////////////////////////////////////////////////////////////////////////////////
// Name:      test-configitem.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/vscroll.h>
#include <wx/extension/item.h>
#include <wx/extension/managedframe.h>
#include "../test-configitem.h"
#include "test.h"

TEST_CASE("wxExConfigItem")
{
  wxScrolledWindow* panel = new wxScrolledWindow(GetFrame());
  AddPane(GetFrame(), panel);
  wxFlexGridSizer* sizer = new wxFlexGridSizer(4);
  panel->SetSizer(sizer);
  panel->SetScrollbars(20, 20, 50, 50);
  
  wxExItem::UseConfig(true);

  // Use specific constructors.
  const wxExItem ci_empty;
  const wxExItem ci_spacer(5);
  const wxExItem ci_cb("ci-cb", ITEM_COMBOBOX);
  const wxExItem ci_cb_dir("ci-cb-dir", ITEM_COMBOBOX_DIR);
  const wxExItem ci_sp("ci-sp", 1, 5);
  const wxExItem ci_sp_d("ci-sp-d", 1.0, 5.0);
  const wxExItem ci_sl("ci-sl", 1, 5, 2, ITEM_SLIDER);
  const wxExItem ci_vl(wxLI_HORIZONTAL);
  wxExItem ci_str("ci-string", std::string());
  const wxExItem ci_hl("ci-hyper", "www.wxwidgets.org", ITEM_HYPERLINKCTRL);
  wxExItem ci_st("ci-static", "HELLO", ITEM_STATICTEXT);
  const wxExItem ci_int("ci-int",ITEM_TEXTCTRL_INT);
  const wxExItem ci_rb("ci-rb", {
      {0, "Zero"},
      {1, "One"},
      {2, "Two"}},
    true);
  const wxExItem ci_bc("ci-cl", {
      {0, "Bit One"},
      {1, "Bit Two"},
      {2, "Bit Three"},
      {4, "Bit Four"}},
    false);
  const wxExItem ci_cl_n({"This","Or","Other"});
  const wxExItem ci_user("ci-usr", 
    new wxTextCtrl(), 
    [=](wxWindow* user, wxWindow* parent, bool readonly) {
     ((wxTextCtrl*)user)->Create(parent, 100);}, 
    [=](wxWindow* user, bool save) {
      if (save) wxConfigBase::Get()->Write("mytext", ((wxTextCtrl *)user)->GetValue());
      return true;},
    LABEL_LEFT,
    [=](wxWindow* user, const std::any& value, bool save) {
      wxLogStatus(((wxTextCtrl *)user)->GetValue());});
  
  REQUIRE(ci_empty.GetType() == ITEM_EMPTY);
  REQUIRE(!ci_empty.IsRowGrowable());
  REQUIRE(ci_cb.GetType() == ITEM_COMBOBOX);
  REQUIRE(ci_cb_dir.GetType() == ITEM_COMBOBOX_DIR);
  REQUIRE(ci_spacer.GetType() == ITEM_SPACER);
  REQUIRE(ci_sl.GetLabel() == "ci-sl");
  REQUIRE(ci_sl.GetType() == ITEM_SLIDER);
  REQUIRE(ci_vl.GetType() == ITEM_STATICLINE);
  REQUIRE(ci_sp.GetLabel() == "ci-sp");
  REQUIRE(ci_sp.GetType() == ITEM_SPINCTRL);
  REQUIRE(ci_sp_d.GetType() == ITEM_SPINCTRLDOUBLE);
  REQUIRE(ci_str.GetType() == ITEM_TEXTCTRL);
  REQUIRE(ci_hl.GetType() == ITEM_HYPERLINKCTRL);
  REQUIRE(ci_st.GetType() == ITEM_STATICTEXT);
  REQUIRE(ci_int.GetType() == ITEM_TEXTCTRL_INT);
  REQUIRE(ci_rb.GetType() == ITEM_RADIOBOX);
  REQUIRE(ci_bc.GetType() == ITEM_CHECKLISTBOX_BIT);
  REQUIRE(ci_cl_n.GetType() == ITEM_CHECKLISTBOX_BOOL);
  REQUIRE(ci_user.GetType() == ITEM_USER);

  std::vector <wxExItem> items {
    ci_empty, ci_spacer, ci_cb, ci_cb_dir, ci_sl, ci_vl, ci_sp, ci_sp_d,
    ci_str, ci_hl, ci_st, ci_int, ci_rb, ci_bc, ci_cl_n, ci_user};

  const auto more(TestConfigItems(0, 1));
  items.insert(items.end(), more.begin(), more.end());
  
  // Check members are initialized.
  for (auto& it : items)
  {
    REQUIRE( it.GetColumns() == 1);
    
    if (it.GetType() == ITEM_USER)
      REQUIRE( it.GetWindow() != nullptr);
    else 
      REQUIRE( it.GetWindow() == nullptr);
      
    if (
       it.GetType() != ITEM_STATICLINE &&
       it.GetType() != ITEM_SPACER &&
       it.GetType() != ITEM_EMPTY)
    {
      REQUIRE(!it.GetLabel().empty());
    }
    
    it.SetRowGrowable(true);
  }

  // Layout the items and check control is created.
  for (auto& it : items)
  {
    // Testing on not nullptr not possible,
    // not all items need a sizer.
    it.Layout(panel, sizer);
 
    if (it.GetType() != ITEM_EMPTY && it.GetType() != ITEM_SPACER)
    {
      REQUIRE( it.GetWindow() != nullptr);
      
      if (
          it.GetType() == ITEM_CHECKLISTBOX_BOOL ||
         (it.GetType() >= ITEM_NOTEBOOK && it.GetType() <= ITEM_NOTEBOOK_TREE) || 
          it.GetType() == ITEM_RADIOBOX ||
          it.GetType() == ITEM_STATICLINE ||
          it.GetType() == ITEM_USER ||
          it.GetType() == ITEM_STATICTEXT)
      {
        REQUIRE(!it.GetValue().has_value());
      }
      else
      {
        REQUIRE( it.GetValue().has_value());
      }
    }
  }

  REQUIRE(ci_user.Apply());

  // Now check ToConfig (after Layout).  
  REQUIRE( ci_str.Layout(panel, sizer) != nullptr);
  REQUIRE( ci_st.Layout(panel, sizer) != nullptr);
  REQUIRE( ci_str.ToConfig(true));
  REQUIRE( ci_str.ToConfig(false));
  REQUIRE(!ci_st.ToConfig(true));
  REQUIRE(!ci_st.ToConfig(false));
  REQUIRE( ci_user.ToConfig(true));
  REQUIRE( ci_user.ToConfig(false));
}

#if wxCHECK_VERSION(3,1,0)
TEST_CASE("wxExConfigDefaults")
{
  wxExConfigDefaults def({
    {"def-colour", ITEM_COLOURPICKERWIDGET, *wxWHITE},
    {"def-font", ITEM_FONTPICKERCTRL, wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT)},
    {"def-double", ITEM_TEXTCTRL_FLOAT, 8.8},
    {"def-string", ITEM_TEXTCTRL, std::string("a string")},
    {"def-int", ITEM_TEXTCTRL_INT, 10l}});
  
  REQUIRE( def.Get() != nullptr);
  REQUIRE( def.Get()->Exists("def-colour"));
}
#endif
