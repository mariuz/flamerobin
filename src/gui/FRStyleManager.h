/*
  Copyright (c) 2023-2023 The FlameRobin Development Team

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

#pragma once
#ifndef FR_STYLEMANAGER_H
#define FR_STYLEMANAGER_H

#include "config/Config.h"
#include "core/Subject.h"
#include "gui/FRStyle.h"

class FRStyleManager : public Subject
{
private:
    const wxString _PRYMARY = "StyleTheme";
    const wxString _SECONDARY = "StyleThemeSecondary";
    const wxString _STYLEACTIVE = "StyleActive";
    const wxString _default = "stylers";

    wxFileName fileNamePrimaryM;
    wxFileName fileNameSecondaryM;
    wxFileName fileNameM;
    int styleActiveM;

    FRStyles* globalStylerM;
    FRStylers stylersM;
    wxColor m_GCodecolor{ 255,130,0 };

protected:
    void loadLexerStyles(wxXmlNode* node);
    void loadGlobalStyles(wxXmlNode* node);
    void cleanStyles();

    void saveLexerStyles(wxXmlNode* node);
    void saveGlobalStyles(wxXmlNode* node);

    void assignWordStyle(wxStyledTextCtrl* text, FRStyle* style);
public:
    FRStyleManager();


    FRStyles* getGlobalStyler() { return globalStylerM; };
    FRStylers getLexerStylers() { return stylersM; };
    FRStyles* getStylerByName(wxString stylerName) { return stylerName == "Global Styles" ? getGlobalStyler() : getLexerStylers().getStylerByName(stylerName); };
    FRStyles* getStylerByDesc(wxString stylerDesc) { return stylerDesc == "Global Styles" ? getGlobalStyler() : getLexerStylers().getStylerByDesc(stylerDesc); };

    FRStyle* getGlobalStyle() { return globalStylerM->getStyleByName("Global override"); };
    FRStyle* getDefaultStyle() { return globalStylerM->getStyleByName("Default Style"); };
    FRStyle* getStyleByName(wxString styleName);

    wxFileName getFileName();
    void setFileName(wxFileName fileName);
    wxFileName getFileNamePrimary() { return fileNamePrimaryM; };
    void setFileNamePrimary(wxFileName fileName);
    wxFileName getFileNameSecondary() { return fileNameSecondaryM; };
    void setFileNameSecondary(wxFileName fileName);
    int getSytyleActive() { return styleActiveM; };
    void setStyleActive(int style) ;


    void assignGlobal(wxStyledTextCtrl* text);

    void assignLexer(wxStyledTextCtrl* text);

    void assignMargin(wxStyledTextCtrl* text);

    void loadConfig();
    void loadStyle();
    void saveStyle();

};

FRStyleManager& stylerManager();



#endif //FR_STYLEMANAGER_H
