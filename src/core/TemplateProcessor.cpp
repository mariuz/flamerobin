/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

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

#include <algorithm>

TemplateProcessor::TemplateProcessor(ProcessableObject* object, wxWindow* window)
    : objectM(object), windowM(window)
{
}

void TemplateProcessor::processCommand(const wxString& cmdName,
    const TemplateCmdParams& cmdParams, ProcessableObject* object,
    wxString& processedText)
{
    if (cmdName == "--")
        // Comment.
        ;

    // {%template_root%}
    // Expands to the full path of the folder containing the currently
    // processed template, including the final path separator.
    // May expand to a blank string if the template being processed does
    // not come from a text file.
    else if (cmdName == "template_root")
        processedText += getTemplatePath();

    // {%getvar:name%}
    // Expands to the value of the specified string variable, or a blank
    // string if the variable is not defined.
    else if (cmdName == "getvar" && !cmdParams.IsEmpty())
        processedText += getVar(cmdParams.all());

    // {%setvar:name:value%}
    // Sets the value of the specified variable and expands to a blank string.
    // If the variable is already defined overwrites it.
    else if (cmdName == "setvar" && !cmdParams.IsEmpty())
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
    else if (cmdName == "clearvar" && !cmdParams.IsEmpty())
        clearVar(cmdParams.all());

    // {%clearvars%}
    // Sets all defined variables to blank strings.
    // Expands to a blank string.
    else if (cmdName == "clearvars")
        clearVars();

    // {%getconf:key:default%}
    // Expands to the value of the specified local config key,
    // or a blank string if the key is not found.
    // The local config is associated to the template - this is
    // not one of FlameRobin's global config files.
    else if (cmdName == "getconf" && !cmdParams.IsEmpty())
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams[0], object);
        wxString defValue;
        if (cmdParams.Count() > 1)
            defValue = cmdParams.from(1);
        processedText += configM.get(text, defValue);
    }

    // {%setconf:key:value%}
    // Sets the value of the specified local config key.
    // If the key is already defined overwrites it.
    // Expands to a blank string.
    else if (cmdName == "setconf" && !cmdParams.IsEmpty())
    {
        if (cmdParams.Count() == 1)
            configM.setValue(cmdParams[0], wxString(""));
        else
            configM.setValue(cmdParams[0], cmdParams[1]);
    }

    // {%getglobalconf:key%}
    // Expands to the value of the specified global config key,
    // or a blank string if the key is not found.
    else if (cmdName == "getglobalconf" && !cmdParams.IsEmpty())
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams.all(), object);
        processedText += config().get(text, wxString(""));
    }

    // {%abort%}
    // Throws a silent exception, interrupting the processing.
    // Expands to a blank string.
    else if (cmdName == "abort")
        throw FRAbort();

    // {%parent_window%}
    // Expands to the current window's numeric memory address.
    // Used to call FR's commands through URIs.
    else if (cmdName == "parent_window")
        processedText += wxString::Format("%p", windowM);

    // {%colon%}
    else if (cmdName == "colon")
        processedText += ":";

    // {%if:<term>:<true output>[:<false output>]%}
    // If <term> equals true expands to <true output>, otherwise
    // expands to <false output> (or an empty string).
    else if (cmdName == "if" && (cmdParams.Count() >= 2))
    {
        wxString val;
        internalProcessTemplateText(val, cmdParams[0], object);

        wxString trueText;
        internalProcessTemplateText(trueText, cmdParams[1], object);
        wxString falseText;
        if (cmdParams.Count() >= 3)
            internalProcessTemplateText(falseText, cmdParams.from(2), object);
        if (getStringAsBoolean(val))
            processedText += trueText;
        else
            processedText += falseText;
    }

    // {%ifeq:<left term>:<right term>:<true output>[:<false output>]%}
    // If <left term> equals <right term> expands to <true output>, otherwise
    // expands to <false output> (or an empty string).
    else if (cmdName == "ifeq" && (cmdParams.Count() >= 3))
    {
        wxString val1;
        internalProcessTemplateText(val1, cmdParams[0], object);
        wxString val2;
        internalProcessTemplateText(val2, cmdParams[1], object);
        wxString trueText;
        internalProcessTemplateText(trueText, cmdParams[2], object);
        wxString falseText;
        if (cmdParams.Count() >= 4)
            internalProcessTemplateText(falseText, cmdParams.from(3), object);
        if (val1 == val2)
            processedText += trueText;
        else
            processedText += falseText;
    }

    // {%!:<input>%} or {%not:<input>%}
    // If <input> equals "true" returns "false";
    // if <input> equals "false" returns "true";
    // otherwise returns <input>.
    // Always expands the argument before evaluating it.
    else if ((cmdName == "!" || (cmdName == "not")) && (cmdParams.Count()))
    {
        wxString input;
        internalProcessTemplateText(input, cmdParams[0], object);
        if (input == getBooleanAsString(true))
            processedText += getBooleanAsString(false);
        else if (input == getBooleanAsString(false))
            processedText += getBooleanAsString(true);
        else
            processedText += input;
    }

    // {%ifcontains:<list>:<term>:<true output>[:<false output>]%}
    // If <list> contains <term> expands to <true output>, otherwise
    // expands to <false output> (or an empty string).
    // <list> is a list of comma-separated values.
    else if (cmdName == "ifcontains" && (cmdParams.Count() >= 3))
    {
        wxString listStr;
        internalProcessTemplateText(listStr, cmdParams[0], object);
        wxArrayString list(wxStringTokenize(listStr, ","));
        for (wxString::size_type i = 0; i < list.Count(); i++)
            list[i] = list[i].Trim(true).Trim(false);
        wxString val2;
        internalProcessTemplateText(val2, cmdParams[1], object);
        wxString trueText;
        internalProcessTemplateText(trueText, cmdParams[2], object);
        wxString falseText;
        if (cmdParams.Count() >= 4)
            internalProcessTemplateText(falseText, cmdParams.from(3), object);
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
    else if (cmdName == "forall" && (cmdParams.Count() >= 3))
    {
        wxString listStr;
        internalProcessTemplateText(listStr, cmdParams[0], object);
        wxArrayString list(wxStringTokenize(listStr, ","));
        for (wxString::size_type i = 0; i < list.Count(); i++)
            list[i] = list[i].Trim(true).Trim(false);

        wxString separator;
        internalProcessTemplateText(separator, cmdParams[1], object);

        bool firstItem = true;
        for (wxArrayString::iterator it = list.begin(); it != list.end(); ++it)
        {
            wxString param = cmdParams.from(2);
            // Try to replace %%current_value%% both before and after expansion.
            param.Replace("%%current_value%%", *(it));
            wxString newText;
            internalProcessTemplateText(newText, param, object);
            newText.Replace("%%current_value%%", *(it));
            if ((!firstItem) && (!newText.IsEmpty()))
                processedText += escapeChars(separator);
            if (!newText.IsEmpty())
                firstItem = false;
            processedText += newText;
        }
    }

    // {%countall:<list>%}
    // Expands to the number of strings in <list>.
    // <list> is a list of comma-separated values.
    else if (cmdName == "countall" && (cmdParams.Count() >= 1))
    {
        wxString listStr;
        internalProcessTemplateText(listStr, cmdParams.all(), object);
        wxArrayString list(wxStringTokenize(listStr, ","));
        processedText << list.Count();
    }

    // {%alternate:<text1>:<text2>%}
    // Alternates expanding to <text1> and <text2> at each call,
    // starting with <text1>.
    // Used to alternate table row colours, for example.
    else if (cmdName == "alternate" && (cmdParams.Count() >= 2))
    {
        static bool first = false;
        first = !first;
        if (first)
            processedText += cmdParams[0];
        else
            processedText += cmdParams[1];
    }

    // {%substr:<text>:<from>:<for>%}
    // Extracts a substring of <for> characters from <text>
    // starting at character <from>.
    // <from> defaults to 0. <for> defaults to <text>'s length minus 1.
    else if (cmdName == "substr" && (cmdParams.Count() >= 3))
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams[0], object);
        wxString from;
        internalProcessTemplateText(from, cmdParams[1], object);
        long fromI;
        if (!from.ToLong(&fromI))
            fromI = 0;
        wxString for_;
        internalProcessTemplateText(for_, cmdParams.from(2), object);
        long forI;
        if (!for_.ToLong(&forI))
            forI = text.Length() - 1;

        processedText += text.SubString(fromI, fromI + forI - 1);
    }

    // {%uppercase:<text>%}
    // Converts <text> to upper case.
    else if (cmdName == "uppercase" && (cmdParams.Count() >= 1))
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams.all(), object);

        processedText += text.Upper();
    }

    // {%lowercase:<text>%}
    // Converts <text> to lower case.
    else if (cmdName == "lowercase" && (cmdParams.Count() >= 1))
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams.all(), object);

        processedText += text.Lower();
    }

    // {%wrap:<text>[:<width>[:<indent>]]%}
    // Wraps <text> to lines fo maximum <width> chars, indenting all
    // resulting lines after the first one by <indent> chars.
    // <width> defaults to config item sqlEditorEdgeColumn, or 80.
    // <indent> defaults to config item sqlEditorTabSize, or 4.
    else if (cmdName == "wrap" && (cmdParams.Count() >= 1))
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams[0], object);

        long widthI;
        if (cmdParams.Count() >= 2)
        {
            wxString width;
            internalProcessTemplateText(width, cmdParams[1], object);
            if (!width.ToLong(&widthI))
                widthI = config().get("sqlEditorEdgeColumn", 80);
        }
        else
            widthI = config().get("sqlEditorEdgeColumn", 80);

        long indentI;
        if (cmdParams.Count() >= 3)
        {
            wxString indent;
            internalProcessTemplateText(indent, cmdParams[2], object);
            if (!indent.ToLong(&indentI))
                indentI = config().get("sqlEditorTabSize", 4);
        }
        else
            indentI = config().get("sqlEditorTabSize", 4);

        processedText += wrapText(text, widthI, indentI);
    }

    // {%kw:<text>%}
    // Formats <text> as a keyword (upper or lower case)
    // according to config item SQLKeywordsUpperCase.
    else if (cmdName == "kw" && (cmdParams.Count() >= 1))
    {
        wxString text;
        internalProcessTemplateText(text, cmdParams.all(), object);

        if (config().get("SQLKeywordsUpperCase", true))
            processedText += text.Upper();
        else
            processedText += text.Lower();
    }

    // {%tab%}
    // Expands to a number of spaces defined by config item
    // sqlEditorTabSize.
    else if (cmdName == "tab")
    {
        wxString tab;
        processedText += tab.Pad(config().get("sqlEditorTabSize", 4));
    }

    // Only if no internal commands are recognized, call external command handlers.
    else
    {
        getTemplateCmdHandlerRepository().handleTemplateCmd(this, cmdName,
           cmdParams, object, processedText);
    }
}

void TemplateProcessor::internalProcessTemplateText(wxString& processedText,
    const wxString& inputText, ProcessableObject* object)
{
    if (object == 0)
        object = objectM;

    // parse commands
    wxString::size_type pos = 0, oldpos = 0, endpos = 0;
    while (true)
    {
        pos = inputText.find("{%", pos);
        if (pos == wxString::npos)
        {
            processedText += inputText.substr(oldpos);
            break;
        }

        wxString::size_type check, startpos = pos;
        int cnt = 1;
        while (cnt > 0)
        {
            endpos = inputText.find("%}", startpos+1);
            if (endpos == wxString::npos)
                break;

            check = inputText.find("{%", startpos+1);
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

            if (c == ':')
            {
                if ((nestLevel == 0) && (state == inText))
                {
                    cmdParams.Add(buffer);
                    buffer.Clear();
                    continue;
                }
            }
            buffer += c;

            if ((c == '{') && (i < cmd.Length() - 1) && (cmd[i + 1] == '%'))
                nestLevel++;
            else if ((c == '}') && (i > 0) && (cmd[i - 1] == '%'))
                nestLevel--;
            else if (c == '\'')
                state == inString1 ? state = inText : state = inString1;
            else if (c == '"')
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

void TemplateProcessor::processTemplateFile(wxString& processedText,
    const wxFileName&  inputFileName, ProcessableObject* object,
    ProgressIndicator* progressIndicator)
{
    fileNameM = inputFileName;

    wxFileName infoFileName(inputFileName);
    infoFileName.SetExt("info");
    infoM.setConfigFileName(infoFileName);
    // put settings file in user writable directory
    wxString confFileNameStr(inputFileName.GetFullPath());
    confFileNameStr.Replace(config().getHomePath(),
        config().getUserHomePath(), false);
    wxFileName confFileName(confFileNameStr);
    confFileName.SetExt("conf");
    configM.setConfigFileName(confFileName);
    progressIndicatorM = progressIndicator;
    internalProcessTemplateText(processedText, loadEntireFile(fileNameM),
        object);
}

void TemplateProcessor::processTemplateText(wxString& processedText,
    const wxString& inputText, ProcessableObject* object,
    ProgressIndicator* progressIndicator)
{
    fileNameM.Clear();
    configM.setConfigFileName(wxFileName(wxEmptyString));
    progressIndicatorM = progressIndicator;
    internalProcessTemplateText(processedText, inputText, object);
}

void TemplateProcessor::setVar(const wxString& varName,
    const wxString& varValue)
{
    varsM[varName] = varValue;
}

const wxString& TemplateProcessor::getVar(const wxString& varName)
{
    return varsM[varName];
}

void TemplateProcessor::clearVar(const wxString& varName)
{
    varsM.erase(varName);
}

void TemplateProcessor::clearVars()
{
    varsM.clear();
}

wxString TemplateProcessor::getTemplatePath()
{
    return fileNameM.GetPathWithSep();
}

wxString TemplateCmdParams::all() const
{
    return from(0);
}

wxString TemplateCmdParams::from(size_t start) const
{
    wxString result;
    if (start < Count())
    {
        result = Item(start);
        for (size_t i = start + 1; i < Count(); i++)
            result += ':' + Item(i);
    }
    return result;
}

TemplateCmdHandlerRepository& getTemplateCmdHandlerRepository()
{
    static TemplateCmdHandlerRepository repository;
    return repository;
}

//! needed to disallow instantiation
TemplateCmdHandlerRepository::TemplateCmdHandlerRepository()
    : handlerListSortedM(false)
{
}

TemplateCmdHandlerRepository::~TemplateCmdHandlerRepository()
{
    while (!handlersM.empty())
        removeHandler(handlersM.front());
}

//! needed in checkHandlerListSorted() to sort on objects instead of pointers
bool templateCmdHandlerPointerLT(const TemplateCmdHandler* left,
    const TemplateCmdHandler* right)
{
    return *left < *right;
}

void TemplateCmdHandlerRepository::checkHandlerListSorted()
{
    if (!handlerListSortedM)
    {
        handlersM.sort(templateCmdHandlerPointerLT);
        handlerListSortedM = true;
    }
}

//! returns false if no suitable handler found
void TemplateCmdHandlerRepository::handleTemplateCmd(TemplateProcessor *tp,
    const wxString& cmdName, const TemplateCmdParams& cmdParams,
    ProcessableObject* object, wxString& processedText)
{
    checkHandlerListSorted();
    for (std::list<TemplateCmdHandler*>::iterator it = handlersM.begin();
        it != handlersM.end(); ++it)
    {
        (*it)->handleTemplateCmd(tp, cmdName, cmdParams, object,
            processedText);
    }
}

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

void TemplateCmdHandlerRepository::removeHandler(TemplateCmdHandler* handler)
{
    handlersM.erase(std::find(handlersM.begin(), handlersM.end(), handler));
    handler->setRepository(0);
}

TemplateCmdHandler::TemplateCmdHandler() :
    repositoryM(0)
{
    getTemplateCmdHandlerRepository().addHandler(this);
}

void TemplateCmdHandler::setRepository(TemplateCmdHandlerRepository* const repository)
{
    repositoryM = repository;
}

//! this is currently only called on program termination
TemplateCmdHandler::~TemplateCmdHandler()
{
    if (repositoryM)
        repositoryM->removeHandler(this);
}

