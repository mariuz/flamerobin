/*
  Copyright (c) 2004-2023 The FlameRobin Development Team

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

#include "_ibpp.h"

namespace ibpp_internals
{

fbIntfClass fbIntfClass::gmFbIntf;

fbIntfClass::fbIntfClass()
{
    mMaster = NULL; 
    mStatus = NULL; 
    mUtil = NULL;    
}

fbIntfClass* fbIntfClass::getInstance()
{
    std::string errMsg;
    if (!gmFbIntf.init(errMsg))
        throw LogicExceptionImpl(errMsg);
    return &gmFbIntf;
}

bool fbIntfClass::init(std::string& errmsg)
{
    if (mMaster != NULL)
        return true;

    if (gds.m_get_master_interface == NULL)
    {
        errmsg = "fb_get_master_interface is not present in fbclient.so/dll. "
                 "You need a fbclient 4.0+!";
        return false;
    }
    
    mMaster = gds.m_get_master_interface();
    mStatus = new Firebird::ThrowStatusWrapper(mMaster->getStatus());
    mUtil   = mMaster->getUtilInterface();
    return true;
}

fbIntfClass::~fbIntfClass()
{
    mMaster = NULL;
    if (mStatus != NULL)
        delete mStatus;
    mStatus = NULL;
    mUtil = NULL;
}

} // ibpp_internals