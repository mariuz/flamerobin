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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/wxcrt.h>

#include "gui/controls/DataGridFilterUtils.h"

namespace
{

inline unsigned char asciiToLower(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c + ('a' - 'A')) : c;
}

// Locale-independent Unicode simple case folding for common scripts (ASCII,
// Latin-1, Latin Extended-A, Greek, Cyrillic) with wxTolower fallback, so
// case-insensitive filtering works identically even under the "C" POSIX locale.
inline wxUniChar::value_type unicodeFoldLower(wxUniChar::value_type cp)
{
    if (cp >= 'A' && cp <= 'Z')
        return cp + ('a' - 'A');
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE))
        return cp + 0x20;
    if (cp == 0x0178) // Ÿ -> ÿ
        return 0x00FF;
    if (cp == 0x1E9E) // ẞ -> ß
        return 0x00DF;
    if ((cp >= 0x0100 && cp <= 0x012E) || (cp >= 0x0132 && cp <= 0x0136)
        || (cp >= 0x014A && cp <= 0x0176))
    {
        return (cp & 1) == 0 ? (cp + 1) : cp;
    }
    if ((cp >= 0x0139 && cp <= 0x0147) || (cp >= 0x0179 && cp <= 0x017D))
    {
        return (cp & 1) == 1 ? (cp + 1) : cp;
    }
    if ((cp >= 0x0391 && cp <= 0x03A1) || (cp >= 0x03A3 && cp <= 0x03AB))
        return cp + 0x20;
    if (cp == 0x0401) // Ё -> ё
        return 0x0451;
    if (cp >= 0x0410 && cp <= 0x042F)
        return cp + 0x20;
    return wxTolower(cp);
}

} // anonymous namespace

CaseInsensitiveNeedle::CaseInsensitiveNeedle(const wxString& filterText)
{
    if (filterText.empty())
    {
        emptyM = true;
        asciiOnlyM = true;
        return;
    }

    emptyM = false;
    asciiOnlyM = true;

    for (wxString::const_iterator it = filterText.begin(), end = filterText.end();
        it != end; ++it)
    {
        wxUniChar::value_type cp = (*it).GetValue();
        if (cp >= 0x80)
        {
            asciiOnlyM = false;
            break;
        }
    }

    if (asciiOnlyM)
    {
        asciiLowerM.reserve(filterText.length());
        for (wxString::const_iterator it = filterText.begin(), end = filterText.end();
            it != end; ++it)
        {
            unsigned char ch = static_cast<unsigned char>((*it).GetValue());
            asciiLowerM.push_back(static_cast<char>(asciiToLower(ch)));
        }
    }
    else
    {
        unicodeLowerM.reserve(filterText.length());
        for (wxString::const_iterator it = filterText.begin(), end = filterText.end();
            it != end; ++it)
        {
            unicodeLowerM.push_back(unicodeFoldLower((*it).GetValue()));
        }
        if (unicodeLowerM.empty())
            emptyM = true;
    }
}

bool CaseInsensitiveNeedle::matches(const wxString& haystack) const
{
    if (emptyM)
        return true;
    if (haystack.empty())
        return false;

    if (asciiOnlyM)
    {
        const size_t nlen = asciiLowerM.size();
        const unsigned char first = static_cast<unsigned char>(asciiLowerM[0]);

#if wxUSE_UNICODE_UTF8
        // In UTF-8 builds, utf8_str() is a non-owning view over internal storage.
        // ASCII bytes (< 0x80) never occur inside multi-byte UTF-8 sequences.
        const wxScopedCharBuffer utf8 = haystack.utf8_str();
        const char* data = utf8.data();
        const size_t hlen = utf8.length();
        if (!data || hlen < nlen)
            return false;

        const size_t limit = hlen - nlen;
        for (size_t i = 0; i <= limit; ++i)
        {
            if (asciiToLower(static_cast<unsigned char>(data[i])) != first)
                continue;
            size_t j = 1;
            while (j < nlen
                && asciiToLower(static_cast<unsigned char>(data[i + j]))
                    == static_cast<unsigned char>(asciiLowerM[j]))
            {
                ++j;
            }
            if (j == nlen)
                return true;
        }
        return false;
#else
        const wchar_t* data = haystack.wc_str();
        const size_t hlen = haystack.length();
        if (!data || hlen < nlen)
            return false;

        const size_t limit = hlen - nlen;
        for (size_t i = 0; i <= limit; ++i)
        {
            wchar_t wc = data[i];
            if (wc >= 0x80)
                continue;
            if (asciiToLower(static_cast<unsigned char>(wc)) != first)
                continue;
            size_t j = 1;
            while (j < nlen)
            {
                wchar_t wcNext = data[i + j];
                if (wcNext >= 0x80
                    || asciiToLower(static_cast<unsigned char>(wcNext))
                        != static_cast<unsigned char>(asciiLowerM[j]))
                {
                    break;
                }
                ++j;
            }
            if (j == nlen)
                return true;
        }
        return false;
#endif
    }

    // Unicode path: reuse a thread-local codepoint buffer so scanning hundreds
    // of thousands of cells performs zero per-cell heap allocations.
    thread_local std::vector<wxUniChar::value_type> hayLowerBuf;
    hayLowerBuf.clear();

    for (wxString::const_iterator it = haystack.begin(), end = haystack.end();
        it != end; ++it)
    {
        hayLowerBuf.push_back(unicodeFoldLower((*it).GetValue()));
    }

    const size_t nlen = unicodeLowerM.size();
    if (hayLowerBuf.size() < nlen)
        return false;

    const wxUniChar::value_type first = unicodeLowerM[0];
    const size_t limit = hayLowerBuf.size() - nlen;
    for (size_t i = 0; i <= limit; ++i)
    {
        if (hayLowerBuf[i] != first)
            continue;
        size_t j = 1;
        while (j < nlen && hayLowerBuf[i + j] == unicodeLowerM[j])
            ++j;
        if (j == nlen)
            return true;
    }
    return false;
}

bool filterRowRangeCancellable(
    size_t startRow,
    size_t endRow,
    const std::function<bool(size_t)>& rowPred,
    const std::function<bool()>& shouldCancel,
    std::vector<size_t>& outMatches,
    size_t cancelCheckInterval)
{
    if (cancelCheckInterval == 0)
        cancelCheckInterval = 1;

    size_t step = 0;
    for (size_t r = startRow; r < endRow; ++r, ++step)
    {
        if ((step % cancelCheckInterval) == 0 && shouldCancel && shouldCancel())
        {
            outMatches.clear();
            return false;
        }
        if (rowPred(r))
            outMatches.push_back(r);
    }

    if (shouldCancel && shouldCancel())
    {
        outMatches.clear();
        return false;
    }
    return true;
}
