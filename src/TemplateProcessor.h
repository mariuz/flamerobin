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
protected:
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
		MetadataItem* object, wxWindow *window, bool first = true);
	//! loads the contents of the specified file and calls internalProcessTemplateText().
    void processTemplateFile(wxString& processedText, wxFileName inputFileName,
        MetadataItem* object, wxWindow *window, bool first = true);
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
#endif // FR_TEMPLATEPROCESSOR_H
