/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

//-----------------------------------------------------------------------------
#ifndef FR_TEMPLATEPROCESSOR_H
#define FR_TEMPLATEPROCESSOR_H

#include <wx/filename.h>
#include <wx/arrstr.h>

#include <vector>
#include <map>

#include "metadata/metadataitem.h"
#include "core/ProgressIndicator.h"

//-----------------------------------------------------------------------------
class TemplateCmdParams: public wxArrayString
{
public:
    static const wxChar SEPARATOR;
	//!returns all params concatenated with the default separator.
	wxString all() const;
};
//-----------------------------------------------------------------------------
typedef std::map<wxString, wxString> wxStringMap;
//-----------------------------------------------------------------------------
class TemplateProcessor
{
private:
    MetadataItem *objectM;  // main observed object
    std::vector<MetadataItem *> allowedObjectsM;
    bool flagNextM;
	wxFileName fileNameM;
	wxStringMap varsM;
	ProgressIndicator* progressIndicatorM;
protected:
    ProgressIndicator* getProgressIndicator() { return progressIndicatorM; };
	//! processes a command found in template text
    virtual void processCommand(wxString cmdName,
		TemplateCmdParams cmdParams, MetadataItem* object,
        wxString& processedText, wxWindow *window, bool first);
	//! processor-specific way of escaping special chars
	virtual wxString escapeChars(const wxString& input, bool processNewlines = true) = 0;
	TemplateProcessor(MetadataItem *m,
        std::vector<MetadataItem *> *allowedObjects = 0);
	//! processes all commands without resetting fileNameM. Should be used
	// internally, while processTemplateText() is for external use.
	void internalProcessTemplateText(wxString& processedText, wxString inputText,
        MetadataItem* object, wxWindow *window, bool first = true);
	//! returns the loaded file's path, including the trailing separator.
	wxString getTemplatePath();
public:
	//! processes all known commands found in template text
	//! commands are in format: {%cmdName:cmdParams%}
	//! cmdParams field may be empty, in which case the format is {%cmdName*}
    void processTemplateText(wxString& processedText, wxString inputText,
		MetadataItem* object, wxWindow *window, bool first = true,
		ProgressIndicator* progressIndicator = 0);
	//! loads the contents of the specified file and calls internalProcessTemplateText().
    void processTemplateFile(wxString& processedText, wxFileName inputFileName,
        MetadataItem* object, wxWindow *window, bool first = true,
        ProgressIndicator* progressIndicator = 0);
	//! sets a variable value. If the variable already exists it is overwritten.
	//! To clear a variable, set it to an empty string.
	void setVar(wxString varName, wxString varValue);
	//! gets a variable value. If the variable doesn't exist, an empty string is returned.
	wxString getVar(wxString varName);
	//! clears the specified variable.
	void clearVar(wxString varName);
	//! clears all variables.
	void clearVars();
};
//-----------------------------------------------------------------------------
class TemplateCmdHandler;
//-----------------------------------------------------------------------------
class TemplateCmdHandlerRepository
{
public:
    // interface for handler providers.
    void addHandler(TemplateCmdHandler *handler);
    void removeHandler(TemplateCmdHandler *handler);

    // interface for consumers.
    void handleTemplateCmd(TemplateProcessor *tp, wxString cmdName,
        TemplateCmdParams cmdParams, MetadataItem* object, wxString& processedText,
        wxWindow *window, bool first);

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
//-----------------------------------------------------------------------------
//!singleton instance of the command handler repository.
TemplateCmdHandlerRepository& getTemplateCmdHandlerRepository();
//-----------------------------------------------------------------------------
//! pure virtual class, specific handlers should be derived from it
class TemplateCmdHandler
{
    friend class TemplateCmdHandlerRepository;
public:
    TemplateCmdHandler();
    virtual ~TemplateCmdHandler();

    virtual void handleTemplateCmd(TemplateProcessor *tp, wxString cmdName,
        TemplateCmdParams cmdParams, MetadataItem* object, wxString& processedText,
        wxWindow *window, bool first) = 0;
    
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
//-----------------------------------------------------------------------------

#endif // FR_TEMPLATEPROCESSOR_H
