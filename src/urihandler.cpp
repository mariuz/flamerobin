/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena.
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
//-----------------------------------------------------------------------------
#include <list>
#include <algorithm>

#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
URI::URI()
{
}
//-----------------------------------------------------------------------------
URI::URI(const wxString& uri)
{
	parseURI(uri);
}
//-----------------------------------------------------------------------------
//! pair has format: name=value
void URI::addParam(const wxString& pair)
{
	wxString::size_type p = pair.find(wxT("="));
	if (p == wxString::npos)
		params[pair] = wxT("");
	else
		params[pair.substr(0, p)] = pair.substr(p + 1);
}
//-----------------------------------------------------------------------------
wxString URI::getParam(const wxString& name) const
{
	std::map<wxString, wxString>::const_iterator it = params.find(name);
	if (it == params.end())
		return wxT("");
	else
		return (*it).second;
}
//-----------------------------------------------------------------------------
bool URI::parseURI(const wxString& uri)
{
	using namespace std;
	wxString::size_type p = uri.find(wxT("://"));				// find ://
	if (p == wxString::npos)
		return false;
	protocol = uri.substr(0, p);

	wxString::size_type p2 = uri.find(wxT("?"), p);				// ?
	if (p2 == wxString::npos)
	{
		action = uri.substr(p + 3);
		params.clear();
		return true;
	}

	action = uri.substr(p + 3, p2 - p - 3);
	wxString par = uri.substr(p2 + 1);
	while (true)
	{
		p = par.find(wxT("&"));
		if (p == wxString::npos)
		{
			addParam(par);
			break;
		}

		addParam(par.substr(0, p));
		par.erase(0, p + 1);
	}

	return true;
}
//-----------------------------------------------------------------------------
URIProcessor& getURIProcessor()
{
	static URIProcessor uriProcessor;
	return uriProcessor;
}
//-----------------------------------------------------------------------------
//! needed to disallow instantiation
URIProcessor::URIProcessor()
    : handlerListSortedM(false)
{
}
//-----------------------------------------------------------------------------
URIProcessor::~URIProcessor()
{
    while (!handlersM.empty())
        removeHandler(handlersM.front());
}
//-----------------------------------------------------------------------------
//! needed in checkHandlerListSorted() to sort on objects instead of pointers
bool uriHandlerPointerLT(const URIHandler* left, const URIHandler* right)
{
    return *left < *right;
}
//-----------------------------------------------------------------------------
void URIProcessor::checkHandlerListSorted()
{
    if (!handlerListSortedM)
    {
        handlersM.sort(uriHandlerPointerLT);
        handlerListSortedM = true;
    }
}
//-----------------------------------------------------------------------------
//! returns false if no suitable handler found
bool URIProcessor::handleURI(URI& uri)
{
    checkHandlerListSorted();
    for (std::list<URIHandler*>::iterator it = handlersM.begin(); it != handlersM.end(); ++it)
		if ((*it)->handleURI(uri))
			return true;
    return false;
}
//-----------------------------------------------------------------------------
void URIProcessor::addHandler(URIHandler* handler)
{
    // can't do ordered insert here, since the getPosition() function that
    // serves URIHandler::operator< is virtual, and this function (addHandler)
    // is called in the constructor of URIHandler.
    // The list will be sorted on demand (see checkHandlerListSorted()).
	handlersM.push_back(handler);
	handler->setProcessor(this);
	handlerListSortedM = false;
}
//-----------------------------------------------------------------------------
void URIProcessor::removeHandler(URIHandler* handler)
{
	handlersM.erase(std::find(handlersM.begin(), handlersM.end(), handler));
	handler->setProcessor(0);
}
//-----------------------------------------------------------------------------
URIHandler::URIHandler() :
    processorM(0)
{
    getURIProcessor().addHandler(this);
}
//-----------------------------------------------------------------------------
void URIHandler::setProcessor(URIProcessor* const processor)
{
    processorM = processor;
}
//-----------------------------------------------------------------------------
//! this is currently only called on program termination
URIHandler::~URIHandler()
{
    if (processorM)
        processorM->removeHandler(this);
}
//-----------------------------------------------------------------------------
wxWindow* URIHandler::getWindow(const URI& uri)
{
	wxString ms = uri.getParam(wxT("parent_window"));		// window
	unsigned long mo;
	if (!ms.ToULong(&mo))
		return 0;
	return (wxWindow*)mo;
}
//-----------------------------------------------------------------------------
void* URIHandler::getObject(const URI& uri)
{
	wxString ms = uri.getParam(wxT("object_address"));		// object
	unsigned long mo;
	if (!ms.ToULong(&mo))
		return 0;
	return (void*)mo;
}
//-----------------------------------------------------------------------------
