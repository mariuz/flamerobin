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

#ifndef FR_URIHANDLER_H
#define FR_URIHANDLER_H

#include <list>
#include <string>
#include <map>
//-----------------------------------------------------------------------------
//! almost like a struct, but with few helper functions
class YURI
{
public:
	std::string protocol;
	std::string action;
	std::map<std::string, std::string> params;

	YURI();
	YURI(const std::string& uri);
	bool parseURI(const std::string& uri);
	void addParam(const std::string& pair);
	std::string getParam(const std::string& name) const;
};
//-----------------------------------------------------------------------------
class YxURIHandler;
//-----------------------------------------------------------------------------
class YURIProcessor
{
public:
    // interface for handler providers.
	void addHandler(YxURIHandler *handler);
	void removeHandler(YxURIHandler *handler);

    // interface for consumers.
	bool handleURI(std::string& uriStr);

    virtual ~YURIProcessor();
private:
	std::list<YxURIHandler*> handlersM;
    bool handlerListSortedM;
    void checkHandlerListSorted();

    // only getURIProcessor() may instantiate an object of this class.
    friend YURIProcessor& getURIProcessor();

    // Disable construction, copy-construction and assignment.
    YURIProcessor();
    YURIProcessor(const YURIProcessor&) {};
    YURIProcessor operator==(const YURIProcessor&);
};
//-----------------------------------------------------------------------------
YURIProcessor& getURIProcessor();
//-----------------------------------------------------------------------------
//! pure virtual class, specific handlers should be derived from it
class YxURIHandler
{
    friend class YURIProcessor;
public:
    YxURIHandler();
    virtual ~YxURIHandler();
	virtual bool handleURI(std::string& uriStr) = 0;
	bool operator<(const YxURIHandler& right) const
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
	wxWindow *YxURIHandler::getWindow(YURI& uriObj);
	void *getObject(YURI& uriObj);

private:
    YURIProcessor* processorM;
    void setProcessor(YURIProcessor* const processor);
};
//-----------------------------------------------------------------------------

#endif
