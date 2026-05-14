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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "SecretStore.h"

#if wxUSE_SECRETSTORE
#include <wx/secretstore.h>

bool isSecretStoreAvailable()
{
    return wxSecretStore::GetDefault().IsOk();
}

bool savePasswordInSecretStore(const wxString& service, const wxString& user, const wxString& password)
{
    wxSecretStore store = wxSecretStore::GetDefault();
    if (!store.IsOk())
        return false;
    wxString secretService = "org.flamerobin.FlameRobin:" + service + ":" + user;
    return store.Save(secretService, user, wxSecretValue(password));
}

bool loadPasswordFromSecretStore(const wxString& service, const wxString& user, wxString& password)
{
    wxSecretStore store = wxSecretStore::GetDefault();
    if (!store.IsOk())
        return false;
    wxString secretService = "org.flamerobin.FlameRobin:" + service + ":" + user;
    wxString returnedUser;
    wxSecretValue value;
    if (store.Load(secretService, returnedUser, value))
    {
        password = value.GetAsString();
        return true;
    }
    return false;
}

bool deletePasswordFromSecretStore(const wxString& service, const wxString& user)
{
    wxSecretStore store = wxSecretStore::GetDefault();
    if (!store.IsOk())
        return false;
    wxString secretService = "org.flamerobin.FlameRobin:" + service + ":" + user;
    return store.Delete(secretService);
}

#else

bool isSecretStoreAvailable()
{
    return false;
}

bool savePasswordInSecretStore(const wxString&, const wxString&, const wxString&)
{
    return false;
}

bool loadPasswordFromSecretStore(const wxString&, const wxString&, wxString&)
{
    return false;
}

bool deletePasswordFromSecretStore(const wxString&, const wxString&)
{
    return false;
}

#endif
