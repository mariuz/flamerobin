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

#ifndef FR_OBJECTWITHHANDLE_H
#define FR_OBJECTWITHHANDLE_H

#include <wx/string.h>

#include <map>

template<class T>
class ObjectWithHandle
{
public:
    typedef unsigned long Handle;
private:
    const Handle handleM;

    typedef std::map<Handle, ObjectWithHandle*> HandleMap;
    typedef std::pair<Handle, ObjectWithHandle*> HandlePair;

    static HandleMap handleMap;
    static Handle nextHandle;
protected:
    // protected constructor initializes the handle
    ObjectWithHandle()
        : handleM(++nextHandle)
    {
        handleMap.insert(HandlePair(handleM, this));
    }

    // protected copy constructor initializes the handle
    // each copy needs to have its own handle
    ObjectWithHandle(const ObjectWithHandle&)
        : handleM(++nextHandle)
    {
        handleMap.insert(HandlePair(handleM, this));
    }
public:
    virtual ~ObjectWithHandle()
    {
        handleMap.erase(handleM);
    }

    // compiler can't auto-create assignment operator due to const member
    // handle must not be changed, so assignment operator does nothing
    ObjectWithHandle& operator = (const ObjectWithHandle&)
    {
        // let each instance keep its own handle
        return *this;
    }

    Handle getHandle() const
    {
         return handleM;
    }

    static bool findObjectFromHandle(Handle handle, T*& obj)
    {
        typename HandleMap::iterator it = handleMap.find(handle);
        if (it == handleMap.end())
        {
            obj = 0;
            return false;
        }
        obj = static_cast<T*>(it->second);
        return true;
    }

    static T* getObjectFromHandle(Handle handle)
    {
        T* obj;
        return findObjectFromHandle(handle, obj) ? obj : 0;
    }

    static T* getObjectFromHandle(const wxString& handle)
    {
        Handle hv;
        if (handle.ToULong(&hv))
            return getObjectFromHandle(hv);
        return 0;
    }
};

#endif // FR_OBJECTWITHHANDLE_H
