/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_ABOUTBOXPLUGINS_H
#define FR_ABOUTBOXPLUGINS_H

#include <wx/string.h>

// What the About box reports about the Firebird client in use.
//
// None of this loads a plugin module, and that is deliberate.  Asking the
// plugin manager to enumerate a plugin type loads every module it names, and
// releasing the set unloads them again - which runs the engine's shutdown
// path, and that throws out of a destructor when the lock directory is not
// accessible, taking the whole application down with it.  Reading the
// configuration and looking at the plugins directory answers the same question
// without ever calling into a plugin.

// The plugin names this client is configured to use, comma separated and in
// configuration order, for example "Remote, Engine13, Loopback, Srp256, ...".
wxString getFirebirdConfiguredPlugins();

// The directory the client loads plugin modules from, or an empty string.
wxString getFirebirdPluginDirectory();

// The plugin modules present in that directory, comma separated and with the
// platform's library prefix and extension removed, for example
// "ChaCha, Engine13, Srp".  An Engine* entry here is what makes an embedded
// connection possible.
wxString getFirebirdPluginModules();

#endif // FR_ABOUTBOXPLUGINS_H
