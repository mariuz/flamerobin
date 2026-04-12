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

#ifndef FR_LOCALEMANAGER_H
#define FR_LOCALEMANAGER_H

#include <wx/intl.h>

// Application-lifetime locale manager.
// Holds a single wxLocale instance initialized from application config.
// Call initFromConfig() once at startup and reinitFromConfig() whenever
// the locale preference changes (e.g. after saving preferences).
class LocaleManager
{
public:
    static LocaleManager& get();

    // Initialize locale from config (UseLocalConfig preference).
    // Should be called once during Application::OnInit().
    void initFromConfig();

    // Re-initialize locale from config after preferences have been saved.
    void reinitFromConfig();

private:
    LocaleManager();
    ~LocaleManager();

    // Non-copyable
    LocaleManager(const LocaleManager&);
    LocaleManager& operator=(const LocaleManager&);

    void applyConfig();

    wxLocale* localeM;
};

#endif // FR_LOCALEMANAGER_H
