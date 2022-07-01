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

#ifndef FR_URIPROCESSOR_H
#define FR_URIPROCESSOR_H

#include <list>
#include <map>


class URI
{
public:
    wxString protocol;
    wxString action;
    std::map<wxString, wxString> params;

    URI();
    URI(const wxString& uri);
    bool parseURI(const wxString& uri);
    void addParam(const wxString& pair);
    wxString getParam(const wxString& name) const;
};

class URIHandler;

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

URIProcessor& getURIProcessor();

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
private:
    URIProcessor* processorM;
    void setProcessor(URIProcessor* const processor);
};


#endif // FR_URIPROCESSOR_H
