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

#include <wx/tokenzr.h>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "frutils.h"
#include "core/FRError.h"
#include "core/ProcessableObject.h"
#include "core/ProgressIndicator.h"
#include "TemplateProcessor.h"

//-----------------------------------------------------------------------------
TemplateProcessor::TemplateProcessor(ProcessableObject* object, wxWindow* window)
    : objectM(object), windowM(window)
{
}
//-----------------------------------------------------------------------------
void TemplateProcessor::processCommand(wxString cmdName, TemplateCmdParams cmdParams,
    ProcessableObject* object, wxString& processedText)
{
    if (cmdName == wxT("--"))
        // Comment.
        ;
    
    // {%template_root%}
    // Expands to the full path of the folder containing the currently
    // processed template, including the final path separator.
    // May expand to a blank string if the template being processed does
    // not come from a text file.
    else if (cmdName == wxT("template_root"))
        processedText += getTemplatePath();

    // {%getvar:name%}
    // Expands to the value of the specified string variable, or a blank
    // string if the variable is not defined.
    else if (cmdName == wxT("getvar") && !cmdParams.IsEmpty())
        processedText += getVar(cmdParams.all());

    // {%setvar:name:value%}
    // Sets the value of the specified variable and expands to a blank string.
    // If the variable is already defined overwrites it.
    else if (cmdName == wxT("setvar") && !cmdParams.IsEmpty())
    {
        if (cmdParams.Count() == 1)
            clearVar(cmdParams[0]);
        else
            setVar(cmdParams[0], cmdParams[1]);
    }

    // {%clearvar:name%}
    // Sets the value of the specified variable to a blank string.
    // If the variable is not defined it does nothing.
    // Expands to a blank string.
    else if (cmdName == wxT("clearvar") && !cmdParams.IsEmpty())
        clearVar(cmdParams.all());

    // {%clearvars%}
    // Sets all defined variables to blank strings.
    // Expands to a blank string.
    else if (cmdName == wxT("clearvars"))
        clearVars();

    // {%getconf:key%}
    // Expands to the value of the specified local config key,
    // or a blank string if the key is not found.
    // The local config is associated to the template - this is
    // not one of FlameRobin's global config files.
    else if (cmdName == wxT("getconf") && !cmdParams.IsEmpty())
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams.all(), object);
        processedText += configM.get(text, wxString(wxT("")));
    }

    // {%setconf:key:value%}
    // Sets the value of the specified specified local config key.
    // If the key is already defined overwrites it.
    // Expands to a blank string.
    else if (cmdName == wxT("setconf") && !cmdParams.IsEmpty())
    {
        if (cmdParams.Count() == 1)
            configM.setValue(cmdParams[0], wxString(wxT("")));
        else
            configM.setValue(cmdParams[0], cmdParams[1]);
    }

    // {%getglobalconf:key%}
    // Expands to the value of the specified global config key,
    // or a blank string if the key is not found.
    else if (cmdName == wxT("getglobalconf") && !cmdParams.IsEmpty())
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams.all(), object);
        processedText += config().get(text, wxString(wxT("")));
    }

    // {%abort%}
    // Throws a silent exception, interrupting the processing.
    // Expands to a blank string.
    else if (cmdName == wxT("abort"))
        throw FRAbort();
    
    // {%parent_window%}
    // Expands to the current window's numeric memory address.
    // Used to call FR's commands through URIs.
    else if (cmdName == wxT("parent_window"))
        processedText += wxString::Format(wxT("%ld"), (uintptr_t)windowM);

    // {%ifeq:<left term>:<right term>:<true output>[:<false output>]%}
    // If <left term> equals <right term> expands to <true output>, otherwise
    // expands to <false output> (or an empty string).
    else if (cmdName == wxT("ifeq") && (cmdParams.Count() >= 3))
    {
        wxString val1;
        internalProcessTemplateText(val1, cmdParams[0], object);
        wxString val2;
        internalProcessTemplateText(val2, cmdParams[1], object);
        wxString trueText;
        internalProcessTemplateText(trueText, cmdParams[2], object);
        wxString falseText;
        if (cmdParams.Count() >= 4)
        {
            cmdParams.RemoveAt(0, 3);
            internalProcessTemplateText(falseText, cmdParams.all(), object);
        }
        if (val1 == val2)
            processedText += trueText;
        else
            processedText += falseText;
    }

    // {%ifcontains:<list>:<term>:<true output>[:<false output>]%}
    // If <list> contains <term> expands to <true output>, otherwise
    // expands to <false output> (or an empty string).
    // <list> is a list of comma-separated values.
    else if (cmdName == wxT("ifcontains") && (cmdParams.Count() >= 3))
    {
        wxString listStr;
        internalProcessTemplateText(listStr, cmdParams[0], object);
        wxArrayString list(wxStringTokenize(listStr, wxT(",")));
        for (wxString::size_type i = 0; i < list.Count(); i++)
            list[i] = list[i].Trim(true).Trim(false);
        wxString val2;
        internalProcessTemplateText(val2, cmdParams[1], object);
        wxString trueText;
        internalProcessTemplateText(trueText, cmdParams[2], object);
        wxString falseText;
        if (cmdParams.Count() >= 4)
        {
            cmdParams.RemoveAt(0, 3);
            internalProcessTemplateText(falseText, cmdParams.all(), object);
        }
        if (list.Index(val2) != wxNOT_FOUND)
            processedText += trueText;
        else
            processedText += falseText;
    }

    // {%forall:<list>:<separator>:<text>%}
    // Repeats <text> once for each string in <list>, pasting a <separator>
    // before each item except the first.
    // <list> is a list of comma-separated values.
    // Inside <text> use the placeholder %%current_value%% to
    // mean the current string in the list.
    else if (cmdName == wxT("forall") && (cmdParams.Count() >= 3))
    {
        wxString listStr;
        internalProcessTemplateText(listStr, cmdParams[0], object);
        wxArrayString list(wxStringTokenize(listStr, wxT(",")));
        for (wxString::size_type i = 0; i < list.Count(); i++)
            list[i] = list[i].Trim(true).Trim(false);

        wxString separator;
        internalProcessTemplateText(separator, cmdParams[1], object);

        bool firstItem = true;
        for (wxArrayString::iterator it = list.begin(); it != list.end(); ++it)
        {
            wxString newText;
            internalProcessTemplateText(newText, cmdParams.all(2), object);
            newText.Replace(wxT("%%current_value%%"), *(it));
            if ((!firstItem) && (!newText.IsEmpty()))
                processedText += escapeChars(separator);
            if (!newText.IsEmpty())
                firstItem = false;
            processedText += newText;
        }
    }

    // {%alternate:text1:text2%}
    // Alternates expanding to text1 and text2 at each call, starting with text1.
    // Used to alternate table row colours, for example.
    else if (cmdName == wxT("alternate") && (cmdParams.Count() >= 2))
    {
        static bool first = false;
        first = !first;
        if (first)
            processedText += cmdParams[0];
        else
            processedText += cmdParams[1];
    }

    // Only if no internal commands are recognized, call external command handlers.
    else
        getTemplateCmdHandlerRepository().handleTemplateCmd(this, cmdName, cmdParams,
            object, processedText);
}
//-----------------------------------------------------------------------------
void TemplateProcessor::internalProcessTemplateText(wxString& processedText, wxString inputText,
    ProcessableObject* object)
{
    if (object == 0)
        object = objectM;

    // parse commands
    wxString::size_type pos = 0, oldpos = 0, endpos = 0;
    while (true)
    {
        pos = inputText.find(wxT("{%"), pos);
        if (pos == wxString::npos)
        {
            processedText += inputText.substr(oldpos);
            break;
        }

        wxString::size_type check, startpos = pos;
        int cnt = 1;
        while (cnt > 0)
        {
            endpos = inputText.find(wxT("%}"), startpos+1);
            if (endpos == wxString::npos)
                break;

            check = inputText.find(wxT("{%"), startpos+1);
            if (check == wxString::npos)
                startpos = endpos;
            else
            {
                startpos = (check < endpos ? check : endpos);
                if (startpos == check)
                    cnt++;
            }
            if (startpos == endpos)
                cnt--;
            startpos++;
        }

        if (cnt > 0)    // no matching closing %}
            break;

        processedText += inputText.substr(oldpos, pos - oldpos);
        wxString cmd = inputText.substr(pos + 2, endpos - pos - 2); // 2 = start_marker_len = end_marker_len
        
        // parse command name and params.
        wxString cmdName;
        TemplateCmdParams cmdParams;
        
        enum TemplateCmdState
        {
            inText,
            inString1,
            inString2
        };
        TemplateCmdState state = inText;
        wxString buffer;
        unsigned int nestLevel = 0;
        for (wxString::size_type i = 0; i < cmd.Length(); i++)
        {
            wxChar c = cmd[i];
            
            if (c == wxT(':'))
            {
                if ((nestLevel == 0) && (state == inText))
                {
                    cmdParams.Add(buffer);
                    buffer.Clear();
                    continue;
                }
            }
            buffer += c;
            
            if ((c == wxT('{')) && (i < cmd.Length() - 1) && (cmd[i + 1] == wxT('%')))
                nestLevel++;
            else if ((c == wxT('}')) && (i > 0) && (cmd[i - 1] == wxT('%')))
                nestLevel--;
            else if (c == wxT('\''))
                state == inString1 ? state = inText : state = inString1;
            else if (c == wxT('"'))
                state == inString2 ? state = inText : state = inString2;
        }
        if (buffer.Length() > 0)
            cmdParams.Add(buffer);

        if (cmdParams.Count() > 0)
        {
            cmdName = cmdParams[0];
            cmdParams.RemoveAt(0);
            if (!cmdName.IsEmpty())
                processCommand(cmdName, cmdParams, object, processedText);
        }
        oldpos = pos = endpos + 2;
    }
}
//-----------------------------------------------------------------------------
void TemplateProcessor::processTemplateFile(wxString& processedText,
    const wxFileName&  inputFileName, ProcessableObject* object,
    ProgressIndicator* progressIndicator)
{
    fileNameM = inputFileName;
    // put settings file in user writable directory
    // FIXME:
    // actually this is just a short-cut, assuming that there won't be
    // template files with same names in different paths (which holds true
    // for now, all template files come from $(FR_HOME)/sql-templates)
    //
    // much better would be to strip $(FR_HOME) if it is the first part of
    // the file path, and add the remaining part to getUserHomePath()
    wxFileName confFileName(config().getUserHomePath(),
        inputFileName.GetName(), wxT("conf"));
    confFileName.AppendDir(wxT("template-data"));
    configM.setConfigFileName(confFileName);
    progressIndicatorM = progressIndicator;
    internalProcessTemplateText(processedText, loadEntireFile(fileNameM),
        object);
}
//-----------------------------------------------------------------------------
void TemplateProcessor::processTemplateText(wxString& processedText, wxString inputText,
    ProcessableObject* object, ProgressIndicator* progressIndicator)
{
    fileNameM.Clear();
    configM.setConfigFileName(wxFileName(wxEmptyString));
    progressIndicatorM = progressIndicator;
    internalProcessTemplateText(processedText, inputText, object);
}
//-----------------------------------------------------------------------------
void TemplateProcessor::setVar(wxString varName, wxString varValue)
{
    varsM[varName] = varValue;
}
//-----------------------------------------------------------------------------
wxString TemplateProcessor::getVar(wxString varName)
{
    return varsM[varName];
}
//-----------------------------------------------------------------------------
void TemplateProcessor::clearVar(wxString varName)
{
    varsM.erase(varName);
}
//-----------------------------------------------------------------------------
void TemplateProcessor::clearVars()
{
    varsM.clear();
}
//-----------------------------------------------------------------------------
wxString TemplateProcessor::getTemplatePath()
{
    return fileNameM.GetPathWithSep();
}
//-----------------------------------------------------------------------------
wxString TemplateCmdParams::all(size_t start) const
{
    if (start > Count() - 1)
        start = Count() - 1;
        
    wxString result;
    for (size_t i = start; i < Count(); i++)
    {
        if (i == start)
          result = Item(i);
        else
          result += wxT(':') + Item(i);
    }
    return result;
}
//-----------------------------------------------------------------------------
TemplateCmdHandlerRepository& getTemplateCmdHandlerRepository()
{
    static TemplateCmdHandlerRepository repository;
    return repository;
}
//-----------------------------------------------------------------------------
//! needed to disallow instantiation
TemplateCmdHandlerRepository::TemplateCmdHandlerRepository()
    : handlerListSortedM(false)
{
}
//-----------------------------------------------------------------------------
TemplateCmdHandlerRepository::~TemplateCmdHandlerRepository()
{
    while (!handlersM.empty())
        removeHandler(handlersM.front());
}
//-----------------------------------------------------------------------------
//! needed in checkHandlerListSorted() to sort on objects instead of pointers
bool templateCmdHandlerPointerLT(const TemplateCmdHandler* left, const TemplateCmdHandler* right)
{
    return *left < *right;
}
//-----------------------------------------------------------------------------
void TemplateCmdHandlerRepository::checkHandlerListSorted()
{
    if (!handlerListSortedM)
    {
        handlersM.sort(templateCmdHandlerPointerLT);
        handlerListSortedM = true;
    }
}
//-----------------------------------------------------------------------------
//! returns false if no suitable handler found
void TemplateCmdHandlerRepository::handleTemplateCmd(TemplateProcessor *tp,
    wxString cmdName, TemplateCmdParams cmdParams, ProcessableObject* object,
    wxString& processedText)
{
    checkHandlerListSorted();
    for (std::list<TemplateCmdHandler*>::iterator it = handlersM.begin(); it != handlersM.end(); ++it)
        (*it)->handleTemplateCmd(tp, cmdName, cmdParams, object, processedText);
}
//-----------------------------------------------------------------------------
void TemplateCmdHandlerRepository::addHandler(TemplateCmdHandler* handler)
{
    // can't do ordered insert here, since the getPosition() function that
    // serves TemplateCmdHandler::operator< is virtual, and this function (addHandler)
    // is called in the constructor of TemplateCmdHandler.
    // The list will be sorted on demand (see checkHandlerListSorted()).
    handlersM.push_back(handler);
    handler->setRepository(this);
    handlerListSortedM = false;
}
//-----------------------------------------------------------------------------
void TemplateCmdHandlerRepository::removeHandler(TemplateCmdHandler* handler)
{
    handlersM.erase(std::find(handlersM.begin(), handlersM.end(), handler));
    handler->setRepository(0);
}
//-----------------------------------------------------------------------------
TemplateCmdHandler::TemplateCmdHandler() :
    repositoryM(0)
{
    getTemplateCmdHandlerRepository().addHandler(this);
}
//-----------------------------------------------------------------------------
void TemplateCmdHandler::setRepository(TemplateCmdHandlerRepository* const repository)
{
    repositoryM = repository;
}
//-----------------------------------------------------------------------------
//! this is currently only called on program termination
TemplateCmdHandler::~TemplateCmdHandler()
{
    if (repositoryM)
        repositoryM->removeHandler(this);
}
//-----------------------------------------------------------------------------
