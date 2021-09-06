/*
  Copyright (c) 2004-2021 The FlameRobin Development Team

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

#ifndef FR_FRINT128_H
#define FR_FRINT128_H

// Base class for Flamerobin int128 type.

// gcc has a builtin type int128
// msvc does not have something we can use (AFICS)
// so we have to do it by own code.
#define HAVE_INT128

#ifdef _MSC_VER
#undef HAVE_INT128
#endif

#ifndef HAVE_INT128
typedef struct _INT128_T
{
private:
    // _InOut_ dst
    // _In_ toadd
    // _Out_ overflow
    void AddPart128(uint32_t* dst, const uint32_t& toadd, bool* overflow);
    // _InOut_ T1
    // _In_ T2
    // _Out_ overflow
    void Add128(_INT128_T* T1, const _INT128_T& T2, bool* overflow);
public:
    union DATA
    {
        struct
        {
            uint64_t lowPart;
            int64_t highPart;
        } s2;
        struct
        {
            uint64_t lowPart;
            uint64_t highPart;
        } us2;
        struct
        {
            uint32_t llPart;
            uint32_t hlPart;
            uint32_t lhPart;
            uint32_t hhPart;
        } s4;
    } data;

    // constructor
    _INT128_T() {};
    _INT128_T(const int64_t value);

    _INT128_T operator-();
    _INT128_T operator-(const _INT128_T& T2);
    bool operator<(const _INT128_T& T2) const;
} int128_t;
#else
typedef __int128 int128_t;
#endif

wxString Int128ToString(int128_t value);
bool StringToInt128(const wxString& src, int128_t* dst);

#endif // FR_FRINT128_H
