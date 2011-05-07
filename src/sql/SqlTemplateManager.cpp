/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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


  $Id$

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/regex.h>

#include "config/Config.h"
#include "core/FRError.h"
#include "metadata/metadataitem.h"
#include "sql/SqlTemplateManager.h"

//-----------------------------------------------------------------------------
TemplateDescriptor::TemplateDescriptor(wxFileName templateFileName)
    : templateFileNameM(templateFileName), menuPositionM(0)
{
    loadDescriptionFromConfigFile();
}
//-----------------------------------------------------------------------------
void TemplateDescriptor::loadDescriptionFromConfigFile()
{
    wxFileName confFileName(templateFileNameM);
    confFileName.SetExt(wxT("conf"));
    if (confFileName.FileExists())
    {
        configM.setConfigFileName(confFileName);
        menuCaptionM = configM.get(wxT("templateInfo/menuCaption"), templateFileNameM.GetName());
        menuPositionM = configM.get(wxT("templateInfo/menuPosition"), 0);
        if (!configM.getValue(wxT("templateInfo/matchesType"), matchesTypeM))
            matchesTypeM = wxT(".*");
        if (!configM.getValue(wxT("templateInfo/matchesName"), matchesNameM))
            matchesNameM = wxT(".*");
    }
    else
    {
        menuCaptionM = templateFileNameM.GetName();
        menuPositionM = 0;
        matchesTypeM = wxT(".*");
        matchesNameM = wxT(".*");
    }
}
//-----------------------------------------------------------------------------
bool TemplateDescriptor::operator<(const TemplateDescriptor& right) const
{
    int p = getMenuPosition();
    int rp = right.getMenuPosition();
    if (p == rp)
        return (getMenuCaption().Cmp(right.getMenuCaption()) < 0);
    else
        return (p < rp);
}
//-----------------------------------------------------------------------------
wxString TemplateDescriptor::getMenuCaption() const
{
    if (!menuCaptionM.IsEmpty())
        return menuCaptionM;
    return templateFileNameM.GetName();
}
//-----------------------------------------------------------------------------
 bool TemplateDescriptor::matches(const MetadataItem& metadataItem) const
{
    wxRegEx typeRegEx(matchesTypeM, wxRE_ADVANCED);
    if (!typeRegEx.IsValid())
        throw FRError(_("Invalid regex"));
    if (typeRegEx.Matches(metadataItem.getTypeName()))
    {
        wxRegEx nameRegEx(matchesNameM, wxRE_ADVANCED);
        if (!nameRegEx.IsValid())
            throw FRError(_("Invalid regex"));
        if (nameRegEx.Matches(metadataItem.getName_()))
            return true;
    }
    return false;
}
//-----------------------------------------------------------------------------
//! needed in checkDescriptorsSorted() to sort on objects instead of pointers
bool templateDescriptorPointerLT(const TemplateDescriptorPtr left,
    const TemplateDescriptorPtr right)
{
    return *left < *right;
}
//-----------------------------------------------------------------------------
SqlTemplateManager::SqlTemplateManager(const MetadataItem& metadataItem)
    : metadataItemM(metadataItem)
{
    collectDescriptors();
}
//-----------------------------------------------------------------------------
void SqlTemplateManager::collectDescriptors()
{
    wxArrayString fileNames;
    // Collect predefined and user-defined template descriptors.
    // A user-defined descriptor will supercede a predefined one with the same
    // base name.
    // A user may also remove a predefined template by overriding it with one
    // that does not match any object.
    wxDir::GetAllFiles(config().getSqlTemplatesPath(), &fileNames,
        wxT("*.sql"), wxDIR_FILES);
    wxString userPath = config().getUserSqlTemplatesPath();
    if (wxDir::Exists(userPath))
        wxDir::GetAllFiles(userPath, &fileNames, wxT("*.sql"), wxDIR_FILES);
    descriptorsM.clear();
    for (wxString::size_type i = 0; i < fileNames.Count(); i++)
    {
        wxFileName fileName(fileNames[i]);
        TemplateDescriptor* td = findDescriptor(fileName.GetName());
        if (td)
        {
            // Present already - override it (and re-check matching).
            td->setTemplateFileName(fileName);
            if (!td->matches(metadataItemM))
                descriptorsM.remove(TemplateDescriptorPtr(td));
        }
        else
        {
            TemplateDescriptorPtr tdp(new TemplateDescriptor(fileName));
            if (tdp->matches(metadataItemM))
                descriptorsM.push_back(tdp);
        }
    }
    // Sort everything by menu position.
    descriptorsM.sort(templateDescriptorPointerLT);
}
//-----------------------------------------------------------------------------
TemplateDescriptor* SqlTemplateManager::findDescriptor(wxString baseFileName) const
{
    for (TemplateDescriptorList::const_iterator it = descriptorsBegin();
        it != descriptorsEnd(); ++it)
    {
        if ((*it)->getBaseFileName() == baseFileName)
            return it->get();
    }
    return 0;
}
//-----------------------------------------------------------------------------
TemplateDescriptorList::const_iterator SqlTemplateManager::descriptorsBegin() const
{
    return descriptorsM.begin();
}
//-----------------------------------------------------------------------------
TemplateDescriptorList::const_iterator SqlTemplateManager::descriptorsEnd() const
{
    return descriptorsM.end();
}
//-----------------------------------------------------------------------------
