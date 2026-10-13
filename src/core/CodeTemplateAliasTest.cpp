/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <wx/wx.h>
#include <wx/fileconf.h>
#include <wx/xml/xml.h>
#include "core/TemplateProcessor.h"
#include "core/StringUtils.h"
#include "config/Config.h"

namespace
{
bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << std::endl;
        return true;
    }
    std::cerr << "[FAIL] " << testName << std::endl;
    return false;
}

class TestTemplateProcessor: public TemplateProcessor
{
public:
    TestTemplateProcessor() : TemplateProcessor(0, 0) {}
    virtual wxString escapeChars(const wxString& input, bool) { return input; }
};

bool verifyConfdefHasAliasAndFocus(const wxString& filePath)
{
    wxXmlDocument doc;
    if (!doc.Load(filePath))
        return false;

    wxXmlNode* root = doc.GetRoot();
    if (!root)
        return false;

    bool foundAlias = false;
    bool foundFocus = false;

    for (wxXmlNode* n = root->GetChildren(); n; n = n->GetNext())
    {
        for (wxXmlNode* setting = n->GetChildren(); setting; setting = setting->GetNext())
        {
            if (setting->GetName() == "setting" && setting->GetAttribute("type") == "string")
            {
                bool hasAliasKey = false;
                bool hasFocusTag = false;
                for (wxXmlNode* prop = setting->GetChildren(); prop; prop = prop->GetNext())
                {
                    if (prop->GetName() == "key" && prop->GetNodeContent().Find("tableAlias") != wxNOT_FOUND)
                        hasAliasKey = true;
                    if (prop->GetName() == "focus" && prop->GetNodeContent() == "1")
                        hasFocusTag = true;
                }
                if (hasAliasKey && hasFocusTag)
                {
                    foundAlias = true;
                    foundFocus = true;
                }
            }
        }
    }
    return foundAlias && foundFocus;
}
}

int main(int argc, char** argv)
{
    wxInitializer initializer(argc, argv);
    if (!initializer.IsOk())
    {
        std::cerr << "Failed to initialize wxWidgets.\n";
        return 1;
    }

    wxString appConf = wxFileConfig::GetLocalFileName(wxEmptyString);
    if (wxFileExists(appConf))
        wxRemoveFile(appConf);

    bool ok = true;
    std::cout << "Starting CodeTemplateAlias tests..." << std::endl;

    wxFileName tmpConfig(wxFileName::CreateTempFileName("fr_alias_test"));
    TestTemplateProcessor tp;
    tp.getConfig().setConfigFileName(tmpConfig);

    // 1. Default alias generation using lowercase & substr
    {
        wxString result1;
        tp.processTemplateText(result1, "{%lowercase:{%substr:EMPLOYEES:0:1%}%}", 0, 0);
        ok = check(result1 == "e", "Default alias for EMPLOYEES is 'e'") && ok;

        wxString result2;
        tp.processTemplateText(result2, "{%lowercase:{%substr:CUSTOMER_ORDERS:0:1%}%}", 0, 0);
        ok = check(result2 == "c", "Default alias for CUSTOMER_ORDERS is 'c'") && ok;
    }

    // 2. Default alias (not set in config) -> expands to "a" fallback
    {
        wxString result;
        wxString tpl = "{%ifeq:{%getconf:unconfigured/tableAlias:a%}::: {%getconf:unconfigured/tableAlias:a%}%}";
        tp.processTemplateText(result, tpl, 0, 0);
        ok = check(result == " a", "Default table alias from clause has ' a'") && ok;

        wxString colResult;
        wxString colTpl = "{%ifeq:{%getconf:unconfigured/tableAlias:a%}:::{%getconf:unconfigured/tableAlias:a%}.%}COL1";
        tp.processTemplateText(colResult, colTpl, 0, 0);
        ok = check(colResult == "a.COL1", "Default column alias has 'a.COL1'") && ok;
    }

    // 3. Custom alias set in config -> expands to custom alias
    {
        wxString setResult;
        tp.processTemplateText(setResult, "{%setconf:custom/tableAlias:t%}", 0, 0);

        wxString result;
        wxString tpl = "{%ifeq:{%getconf:custom/tableAlias:a%}::: {%getconf:custom/tableAlias:a%}%}";
        tp.processTemplateText(result, tpl, 0, 0);
        ok = check(result == " t", "Custom table alias from clause has ' t'") && ok;

        wxString colResult;
        wxString colTpl = "{%ifeq:{%getconf:custom/tableAlias:a%}:::{%getconf:custom/tableAlias:a%}.%}COL1";
        tp.processTemplateText(colResult, colTpl, 0, 0);
        ok = check(colResult == "t.COL1", "Custom column alias has 't.COL1'") && ok;
    }

    // 4. Empty alias set in config -> expands to empty string (no alias)
    {
        wxString setResult;
        tp.processTemplateText(setResult, "{%setconf:empty/tableAlias:%}", 0, 0);

        wxString result;
        wxString tpl = "{%ifeq:{%getconf:empty/tableAlias:a%}::: {%getconf:empty/tableAlias:a%}%}";
        tp.processTemplateText(result, tpl, 0, 0);
        ok = check(result == "", "Empty table alias from clause has no prefix") && ok;

        wxString colResult;
        wxString colTpl = "{%ifeq:{%getconf:empty/tableAlias:a%}:::{%getconf:empty/tableAlias:a%}.%}COL1";
        tp.processTemplateText(colResult, colTpl, 0, 0);
        ok = check(colResult == "COL1", "Empty column alias has no prefix") && ok;
    }

    // 5. Confdef files verification for tableAlias and focus=1
    {
        const char* confdefFiles[] = {
            "code-templates/select.confdef",
            "code-templates/update.confdef",
            "code-templates/delete.confdef",
            "code-templates/create_selectable_procedure.confdef",
            "code-templates/create_selectable_execute_block.confdef"
        };
        for (const char* file : confdefFiles)
        {
            wxString path = wxString::FromUTF8(file);
            bool verified = verifyConfdefHasAliasAndFocus(path);
            if (!verified)
                verified = verifyConfdefHasAliasAndFocus("../" + path);
            wxString testName = wxString::Format("Confdef %s has tableAlias and focus", file);
            ok = check(verified, testName.mb_str()) && ok;
        }
    }

    // 6. Template files verification for tableAlias handling
    {
        const char* templateFiles[] = {
            "code-templates/select.template",
            "code-templates/update.template",
            "code-templates/delete.template",
            "code-templates/create_selectable_procedure.template",
            "code-templates/create_selectable_execute_block.template"
        };
        for (const char* file : templateFiles)
        {
            wxFileName fn(wxString::FromUTF8(file));
            if (!fn.FileExists())
                fn = wxFileName("../" + wxString::FromUTF8(file));
            wxString content = loadEntireFile(fn);
            bool hasTableAlias = (content.Find("tableAlias") != wxNOT_FOUND);
            wxString testName = wxString::Format("Template %s references tableAlias", file);
            ok = check(hasTableAlias, testName.mb_str()) && ok;
        }
    }

    wxRemoveFile(tmpConfig.GetFullPath());
    if (wxFileExists(appConf))
        wxRemoveFile(appConf);

    return ok ? 0 : 1;
}
