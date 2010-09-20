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

#include <vector>

#include "metadata/metadataitem.h"

//-----------------------------------------------------------------------------
class TemplateProcessor
{
private:
    MetadataItem *objectM;  // main observed object
    std::vector<MetadataItem *> allowedObjectsM;
    bool flagNextM;
protected:
	//! processes a command found in template text
    virtual void processCommand(wxString cmdName,
		wxString cmdParams, MetadataItem* object,
        wxString& processedText, wxWindow *window, bool first);
	//! processor-specific way of escaping special chars
	virtual wxString escapeChars(const wxString& input, bool processNewlines = true) = 0;
	TemplateProcessor(MetadataItem *m,
        std::vector<MetadataItem *> *allowedObjects = 0);
public:
	//! processes all known commands found in template text
	//! commands are in format: {%cmdName:cmdParams%}
	//! cmdParams field may be empty, in which case the format is {%cmdName*}
    void processTemplateText(wxString& processedText, wxString inputText,
        MetadataItem* object, wxWindow *window, bool first = true);
};
//-----------------------------------------------------------------------------
#endif // FR_TEMPLATEPROCESSOR_H
