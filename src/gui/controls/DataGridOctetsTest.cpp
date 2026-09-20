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

// Regression tests for GitHub issue #714:
// "UUID = OCTET char(16) is showing strange chars instead of HEXA or UUID format in Grid"

#include <iostream>
#include <vector>
#include <string>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "core/StringUtils.h"
#include "gui/controls/DataGridRowBuffer.h"

namespace
{

static bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "  PASSED: " << testName << "\n";
        return true;
    }
    else
    {
        std::cout << "  FAILED: " << testName << "\n";
        return false;
    }
}

wxString formatOctets(const std::string& value)
{
    wxString val;
    if (value.length() == 16)
    {
        const uint8_t* b = reinterpret_cast<const uint8_t*>(value.data());
        val = wxString::Format("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
            b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    }
    else
    {
        for (std::string::size_type p = 0; p < value.length(); p++)
            val += wxString::Format("%02X", uint8_t(value[p]));
    }
    return val;
}

std::vector<uint8_t> parseOctetsInput(wxString input)
{
    input.Replace("-", "");
    input.Replace("{", "");
    input.Replace("}", "");
    input.Replace(" ", "");

    if (input.length() % 2 != 0)
        throw std::runtime_error("Invalid HEX length");

    std::vector<uint8_t> octet;
    for (size_t i = 0; i < input.length(); i += 2)
    {
        if (!wxIsxdigit(input[i]) || !wxIsxdigit(input[i + 1]))
            throw std::runtime_error("Invalid HEX character");
        wxString num = input.Mid(i, 2);
        octet.push_back(static_cast<uint8_t>(std::stoi(std::string(num.mb_str()), nullptr, 16)));
    }
    return octet;
}

} // namespace

int main()
{
    wxInitializer initializer;
    if (!initializer.IsOk())
    {
        std::cerr << "Failed to initialize wxWidgets.\n";
        return 1;
    }

    bool ok = true;
    std::cout << "Running DataGrid OCTETS & UUID Regression Tests (Issue #714)...\n";

    // Test 1: 16-byte OCTETS format as canonical UUID (8-4-4-4-12 hex uppercase)
    {
        // Sample 16-byte UUID: 99 AA 9F F5 2A 4B 4E 3F B5 A1 9F F4 A6 D5 62 09
        const uint8_t rawBytes[16] = {
            0x99, 0xAA, 0x9F, 0xF5, 0x2A, 0x4B, 0x4E, 0x3F,
            0xB5, 0xA1, 0x9F, 0xF4, 0xA6, 0xD5, 0x62, 0x09
        };
        std::string rawStr(reinterpret_cast<const char*>(rawBytes), 16);
        wxString formatted = formatOctets(rawStr);
        ok = check(formatted == "99AA9FF5-2A4B-4E3F-B5A1-9FF4A6D56209",
            "16-byte OCTETS formatted to RFC UUID string") && ok;
    }

    // Test 2: Non-16-byte OCTETS format as uppercase hex pairs
    {
        const uint8_t rawBytes[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
        std::string rawStr(reinterpret_cast<const char*>(rawBytes), 4);
        wxString formatted = formatOctets(rawStr);
        ok = check(formatted == "DEADBEEF", "4-byte OCTETS formatted to DEADBEEF") && ok;
    }

    // Test 3: Parse formatted UUID (with hyphens) into raw 16 bytes
    {
        wxString uuidInput = "99AA9FF5-2A4B-4E3F-B5A1-9FF4A6D56209";
        auto bytes = parseOctetsInput(uuidInput);
        ok = check(bytes.size() == 16, "Parsed UUID has 16 bytes") && ok;
        const uint8_t expected[16] = {
            0x99, 0xAA, 0x9F, 0xF5, 0x2A, 0x4B, 0x4E, 0x3F,
            0xB5, 0xA1, 0x9F, 0xF4, 0xA6, 0xD5, 0x62, 0x09
        };
        bool match = true;
        for (size_t i = 0; i < 16; ++i)
        {
            if (bytes[i] != expected[i]) match = false;
        }
        ok = check(match, "Parsed UUID byte values match expected") && ok;
    }

    // Test 4: Parse braced UUID: {99AA9FF5-2A4B-4E3F-B5A1-9FF4A6D56209}
    {
        wxString bracedUuid = "{99AA9FF5-2A4B-4E3F-B5A1-9FF4A6D56209}";
        auto bytes = parseOctetsInput(bracedUuid);
        ok = check(bytes.size() == 16, "Parsed braced UUID has 16 bytes") && ok;
    }

    // Test 5: Parse hex string with spaces: "99 AA 9F F5 2A 4B 4E 3F B5 A1 9F F4 A6 D5 62 09"
    {
        wxString spacedHex = "99 AA 9F F5 2A 4B 4E 3F B5 A1 9F F4 A6 D5 62 09";
        auto bytes = parseOctetsInput(spacedHex);
        ok = check(bytes.size() == 16, "Parsed space-separated hex has 16 bytes") && ok;
    }

    // Test 6: Invalid hex strings throw error
    {
        bool caughtOdd = false;
        try {
            parseOctetsInput("ABC");
        } catch (const std::runtime_error&) {
            caughtOdd = true;
        }
        ok = check(caughtOdd, "Odd-length hex string throws error") && ok;

        bool caughtInvalidChar = false;
        try {
            parseOctetsInput("ZZZZ");
        } catch (const std::runtime_error&) {
            caughtInvalidChar = true;
        }
        ok = check(caughtInvalidChar, "Non-hex character throws error") && ok;
    }

    // Test 7: DataGridRowBuffer string buffer storage for formatted UUID
    {
        DataGridRowBuffer buffer(1);
        buffer.setString(0, "99AA9FF5-2A4B-4E3F-B5A1-9FF4A6D56209");
        ok = check(buffer.getString(0) == "99AA9FF5-2A4B-4E3F-B5A1-9FF4A6D56209",
            "DataGridRowBuffer correctly stores and returns UUID string") && ok;
    }

    std::cout << "\nAll OCTETS/UUID tests " << (ok ? "PASSED" : "FAILED") << ".\n";
    return ok ? 0 : 1;
}
