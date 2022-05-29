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

#include <list>
#include <algorithm>

#include "core/URIProcessor.h"

URI::URI()
{
}

URI::URI(const wxString& uri)
{
    parseURI(uri);
}

//! pair has format: name=value
void URI::addParam(const wxString& pair)
{
    wxString::size_type p = pair.find("=");
    if (p == wxString::npos)
        params[pair] = "";
    else
        params[pair.substr(0, p)] = pair.substr(p + 1);
}

wxString URI::getParam(const wxString& name) const
{
    std::map<wxString, wxString>::const_iterator it = params.find(name);
    if (it == params.end())
        return "";
    else
        return (*it).second;
}

bool URI::parseURI(const wxString& uri)
{
    wxString::size_type p = uri.find("://");               // find ://
    if (p == wxString::npos)
        return false;
    protocol = uri.substr(0, p);

    wxString::size_type p2 = uri.find("?", p);             // ?
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
        p = par.find("&");
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

URIProcessor& getURIProcessor()
{
    static URIProcessor uriProcessor;
    return uriProcessor;
}

//! needed to disallow instantiation
URIProcessor::URIProcessor()
    : handlerListSortedM(false)
{
}

URIProcessor::~URIProcessor()
{
    while (!handlersM.empty())
        removeHandler(handlersM.front());
}

//! needed in checkHandlerListSorted() to sort on objects instead of pointers
bool uriHandlerPointerLT(const URIHandler* left, const URIHandler* right)
{
    return *left < *right;
}

void URIProcessor::checkHandlerListSorted()
{
    if (!handlerListSortedM)
    {
        handlersM.sort(uriHandlerPointerLT);
        handlerListSortedM = true;
    }
}

//! returns false if no suitable handler found
bool URIProcessor::handleURI(URI& uri)
{
    checkHandlerListSorted();
    for (std::list<URIHandler*>::iterator it = handlersM.begin(); it != handlersM.end(); ++it)
        if ((*it)->handleURI(uri))
            return true;
    return false;
}

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

void URIProcessor::removeHandler(URIHandler* handler)
{
    handlersM.erase(std::find(handlersM.begin(), handlersM.end(), handler));
    handler->setProcessor(0);
}

URIHandler::URIHandler() :
    processorM(0)
{
    getURIProcessor().addHandler(this);
}

void URIHandler::setProcessor(URIProcessor* const processor)
{
    processorM = processor;
}

//! this is currently only called on program termination
URIHandler::~URIHandler()
{
    if (processorM)
        processorM->removeHandler(this);
}

