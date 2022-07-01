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


#ifndef FR_TEMPLATEPROCESSOR_H
#define FR_TEMPLATEPROCESSOR_H

#include <wx/filename.h>
#include <wx/arrstr.h>

#include <list>
#include <map>

#include "config/Config.h"
#include "core/ProcessableObject.h"
#include "core/ProgressIndicator.h"


class TemplateCmdParams: public wxArrayString
{
public:
    // returns all params concatenated with the default separator.
    wxString all() const;
    // returns all params from start, concatenated with the default separator.
    wxString from(size_t start) const;
};

typedef std::map<wxString, wxString> wxStringMap;

class TemplateCmdHandler;

class TemplateProcessor
{
private:
    ProcessableObject* objectM;
    bool flagNextM;
    wxFileName fileNameM;
    wxStringMap varsM;
    ProgressIndicator* progressIndicatorM;
    Config configM;
    Config infoM;
    wxWindow* windowM;
protected:
    TemplateProcessor(ProcessableObject* object, wxWindow* window);
    // Processes a command found in template text
    virtual void processCommand(const wxString& cmdName,
        const TemplateCmdParams& cmdParams, ProcessableObject* object,
        wxString& processedText);
    // Returns the loaded file's path, including the trailing separator.
    wxString getTemplatePath();
public:
    wxWindow* getWindow() { return windowM; };
    // Returns a reference to the current progress indicator, so that
    // external command handlers can use it.
    ProgressIndicator* getProgressIndicator() { return progressIndicatorM; };
    // Processes all known commands found in template text
    // commands are in format: {%cmdName:cmdParams%}
    // cmdParams field may be empty, in which case the format is {%cmdName*}
    void processTemplateText(wxString& processedText, const wxString& inputText,
        ProcessableObject* object, ProgressIndicator* progressIndicator = 0);
    // Loads the contents of the specified file and calls internalProcessTemplateText().
    void processTemplateFile(wxString& processedText,
        const wxFileName& inputFileName, ProcessableObject* object,
        ProgressIndicator* progressIndicator = 0);
    // Sets a variable value. If the variable already exists it is overwritten.
    // To clear a variable, set it to an empty string.
    void setVar(const wxString& varName, const wxString& varValue);
    // Gets a variable value. If the variable doesn't exist, an empty string is returned.
    const wxString& getVar(const wxString& varName);
    // Clears the specified variable.
    void clearVar(const wxString& varName);
    // Clears all variables.
    void clearVars();
    // The internal config object, used to store user-supplied parameters in
    // interactive templates.
    Config& getConfig() { return configM; }
    // The template info config object, used to store template metadata such as
    // title, position in menu, etc.
    Config& getInfo() { return infoM; }
    // Name of the current template file if processTemplateFile() has been called.
    wxFileName getCurrentTemplateFileName() { return fileNameM; }
    // Processes all commands without resetting fileNameM. Should be used
    // internally and from command handlers, while processTemplateText()
    // is for external use.
    void internalProcessTemplateText(wxString& processedText,
        const wxString& inputText, ProcessableObject* object);
    // Processor-specific way of escaping special chars
    virtual wxString escapeChars(const wxString& input,
        bool processNewlines = true) = 0;
};

class TemplateCmdHandlerRepository
{
public:
    // interface for handler providers.
    void addHandler(TemplateCmdHandler* handler);
    void removeHandler(TemplateCmdHandler* handler);

    // interface for consumers.
    void handleTemplateCmd(TemplateProcessor* tp, const wxString& cmdName,
        const TemplateCmdParams& cmdParams, ProcessableObject* object,
        wxString& processedText);

    virtual ~TemplateCmdHandlerRepository();
private:
    std::list<TemplateCmdHandler*> handlersM;
    bool handlerListSortedM;
    void checkHandlerListSorted();

    // only getTemplateCmdHandlerRepository() may instantiate an object of this class.
    friend TemplateCmdHandlerRepository& getTemplateCmdHandlerRepository();

    // Disable construction, copy-construction and assignment.
    TemplateCmdHandlerRepository();
    TemplateCmdHandlerRepository(const TemplateCmdHandlerRepository&) {};
    TemplateCmdHandlerRepository operator==(const TemplateCmdHandlerRepository&);
};

//!singleton instance of the command handler repository.
TemplateCmdHandlerRepository& getTemplateCmdHandlerRepository();

// Pure virtual class, specific handlers should be derived from it
class TemplateCmdHandler
{
    friend class TemplateCmdHandlerRepository;
public:
    TemplateCmdHandler();
    virtual ~TemplateCmdHandler();

    virtual void handleTemplateCmd(TemplateProcessor* tp,
        const wxString& cmdName, const TemplateCmdParams& cmdParams,
        ProcessableObject* object, wxString& processedText) = 0;

    bool operator<(const TemplateCmdHandler& right) const
    {
        return getPosition() < right.getPosition();
    }
protected:
    virtual int getPosition() const
    {
        // By default all handlers are walked in undefined order; override this
        // function to force a handler to be processed earlier (return a lower
        // number) or later (return a higher number).
        return 1024;
    }
private:
    TemplateCmdHandlerRepository* repositoryM;
    void setRepository(TemplateCmdHandlerRepository* const repository);
};


#endif // FR_TEMPLATEPROCESSOR_H
