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

#ifndef FR_URIHANDLER_H
#define FR_URIHANDLER_H

#include <list>
#include <map>
#include <string>
//-----------------------------------------------------------------------------
//! almost like a struct, but with few helper functions
class URI
{
public:
	std::string protocol;
	std::string action;
	std::map<std::string, std::string> params;

	URI();
	URI(const std::string& uri);
	bool parseURI(const std::string& uri);
	void addParam(const std::string& pair);
	std::string getParam(const std::string& name) const;
};
//-----------------------------------------------------------------------------
class URIHandler;
//-----------------------------------------------------------------------------
class URIProcessor
{
public:
    // interface for handler providers.
	void addHandler(URIHandler *handler);
	void removeHandler(URIHandler *handler);

    // interface for consumers.
	bool handleURI(URI& uri);

    virtual ~URIProcessor();
private:
	std::list<URIHandler*> handlersM;
    bool handlerListSortedM;
    void checkHandlerListSorted();

    // only getURIProcessor() may instantiate an object of this class.
    friend URIProcessor& getURIProcessor();

    // Disable construction, copy-construction and assignment.
    URIProcessor();
    URIProcessor(const URIProcessor&) {};
    URIProcessor operator==(const URIProcessor&);
};
//-----------------------------------------------------------------------------
URIProcessor& getURIProcessor();
//-----------------------------------------------------------------------------
//! pure virtual class, specific handlers should be derived from it
class URIHandler
{
    friend class URIProcessor;
public:
    URIHandler();
    virtual ~URIHandler();
	virtual bool handleURI(URI& uri) = 0;
	bool operator<(const URIHandler& right) const
	{
        return getPosition() < right.getPosition();
    }
protected:
    virtual int getPosition() const
    {
        /*
        By default all handlers are walked in undefined order; override this
        function to force a handler to be processed earlier (return a lower number)
        or later (return a higher number). By convention, use numbers below 1024 for
        pseudo-handlers (handlers that may change the URI and generally must act
        before real handlers) and higher numbers for the others.
        Examples of pseudo-handlers are macro-substitutors and loggers.
        */
        return 1024;
    }

	// some helper functions
	wxWindow *URIHandler::getWindow(const URI& uri);
	void *getObject(const URI& uri);

private:
    URIProcessor* processorM;
    void setProcessor(URIProcessor* const processor);
};
//-----------------------------------------------------------------------------

#endif
