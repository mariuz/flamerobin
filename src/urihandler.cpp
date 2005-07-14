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
#include <string>
#include <list>
#include <algorithm>
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
YURI::YURI()
{
}
//-----------------------------------------------------------------------------
YURI::YURI(const std::string& uri)
{
	parseURI(uri);
}
//-----------------------------------------------------------------------------
//! pair has format: name=value
void YURI::addParam(const std::string& pair)
{
	std::string::size_type p = pair.find("=");
	if (p == std::string::npos)
		params[pair] = "";
	else
		params[pair.substr(0,p)] = pair.substr(p+1);
}
//-----------------------------------------------------------------------------
std::string YURI::getParam(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator it = params.find(name);
	if (it == params.end())
		return "";
	else
		return (*it).second;
}
//-----------------------------------------------------------------------------
bool YURI::parseURI(const std::string& uri)
{
	using namespace std;
	string::size_type p = uri.find("://");				// find ://
	if (p == string::npos)
		return false;
	protocol = uri.substr(0, p);

	string::size_type p2 = uri.find("?", p);				// ?
	if (p2 == string::npos)
	{
		action = uri.substr(p+3);
		params.clear();
		return true;
	}

	action = uri.substr(p+3, p2-p-3);
	std::string par = uri.substr(p2+1);
	while (true)
	{
		p = par.find("&");
		if (p == string::npos)
		{
			addParam(par);
			break;
		}

		addParam(par.substr(0, p));
		par.erase(0, p+1);
	}

	return true;
}
//-----------------------------------------------------------------------------
YURIProcessor& getURIProcessor()
{
	static YURIProcessor uriProcessor;
	return uriProcessor;
}
//-----------------------------------------------------------------------------
//! needed to disallow instantiation
YURIProcessor::YURIProcessor() :
    handlerListSortedM(false)
{
}
//-----------------------------------------------------------------------------
YURIProcessor::~YURIProcessor()
{
    while (!handlersM.empty())
        removeHandler(handlersM.front());
}
//-----------------------------------------------------------------------------
//! needed in checkHandlerListSorted() to sort on objects instead of pointers
bool uriHandlerPointerLT(const YxURIHandler* left, const YxURIHandler* right)
{
    return *left < *right;
}
//-----------------------------------------------------------------------------
void YURIProcessor::checkHandlerListSorted()
{
    if (!handlerListSortedM)
    {
        handlersM.sort(uriHandlerPointerLT);
        handlerListSortedM = true;
    }
}
//-----------------------------------------------------------------------------
//! returns false if no suitable handler found
bool YURIProcessor::handleURI(std::string& uriStr)
{
    checkHandlerListSorted();
    for (std::list<YxURIHandler*>::iterator it = handlersM.begin(); it != handlersM.end(); ++it)
		if ((*it)->handleURI(uriStr))
			return true;
    return false;
}
//-----------------------------------------------------------------------------
void YURIProcessor::addHandler(YxURIHandler* handler)
{
    // can't do ordered insert here, since the getPosition() function that
    // serves YxURIHandler::operator< is virtual, and this function (addHandler)
    // is called in the constructor of YxURIHandler.
    // The list will be sorted on demand (see checkHandlerListSorted()).
	handlersM.push_back(handler);
	handler->setProcessor(this);
	handlerListSortedM = false;
}
//-----------------------------------------------------------------------------
void YURIProcessor::removeHandler(YxURIHandler* handler)
{
	handlersM.erase(std::find(handlersM.begin(), handlersM.end(), handler));
	handler->setProcessor(0);
}
//-----------------------------------------------------------------------------
YxURIHandler::YxURIHandler() :
    processorM(0)
{
    getURIProcessor().addHandler(this);
}
//-----------------------------------------------------------------------------
void YxURIHandler::setProcessor(YURIProcessor* const processor)
{
    processorM = processor;
}
//-----------------------------------------------------------------------------
//! this is currently only called on program termination
YxURIHandler::~YxURIHandler()
{
    if (processorM)
        processorM->removeHandler(this);
}
//-----------------------------------------------------------------------------
wxWindow *YxURIHandler::getWindow(YURI& uriObj)
{
	std::string ms = uriObj.getParam("parent_window");		// window
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return 0;
	return (wxWindow *)mo;
}
//-----------------------------------------------------------------------------
void *YxURIHandler::getObject(YURI& uriObj)
{
	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return 0;
	return (void *)mo;
}
//-----------------------------------------------------------------------------
