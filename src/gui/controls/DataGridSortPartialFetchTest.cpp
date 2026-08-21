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

// Regression tests for GitHub issue #689:
// "Sorting partially fetched result set renders subsequent fetched rows empty and halts further data retrieval"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

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

struct RowData
{
    std::vector<wxString> cols;
    std::vector<bool> isNull;
};

class MockDataGridTable
{
private:
    std::vector<RowData> rowsM;
    std::vector<size_t> rowMappingM;
    bool filterOrSortActiveM = false;
    int sortedColM = -1;
    bool sortAscendingM = true;
    wxString currentFilterTextM;
    unsigned maxRowToFetchM = 100;
    bool allRowsFetchedM = false;
    bool fetchAllRowsM = false;

public:
    void updateRowMapping()
    {
        size_t count = rowsM.size();
        rowMappingM.clear();

        wxString query = currentFilterTextM.Lower();
        for (size_t r = 0; r < count; ++r)
        {
            if (query.IsEmpty())
            {
                rowMappingM.push_back(r);
            }
            else
            {
                bool match = false;
                size_t cols = GetNumberCols();
                for (size_t c = 0; c < cols; ++c)
                {
                    wxString val = getInternalFieldValue(r, c);
                    if (val.Lower().Contains(query))
                    {
                        match = true;
                        break;
                    }
                }
                if (match)
                    rowMappingM.push_back(r);
            }
        }

        if (sortedColM >= 0 && sortedColM < (int)GetNumberCols())
        {
            int col = sortedColM;
            bool asc = sortAscendingM;
            std::stable_sort(rowMappingM.begin(), rowMappingM.end(), [this, col, asc](size_t a, size_t b) {
                bool nullA = isInternalFieldNull(a, col);
                bool nullB = isInternalFieldNull(b, col);
                if (nullA != nullB)
                {
                    return asc ? !nullA : nullA;
                }
                if (nullA && nullB)
                    return false;

                wxString valA = getInternalFieldValue(a, col);
                wxString valB = getInternalFieldValue(b, col);

                double numA = 0, numB = 0;
                bool isNumA = valA.ToDouble(&numA);
                bool isNumB = valB.ToDouble(&numB);

                if (isNumA && isNumB)
                {
                    return asc ? (numA < numB) : (numA > numB);
                }

                int cmp = valA.CmpNoCase(valB);
                return asc ? (cmp < 0) : (cmp > 0);
            });
        }

        filterOrSortActiveM = (!query.IsEmpty() || sortedColM >= 0);
    }

    int getRealRowIndex(int row) const
    {
        if (filterOrSortActiveM)
        {
            if (row >= 0 && row < (int)rowMappingM.size())
                return (int)rowMappingM[row];
            return -1;
        }
        return row;
    }

    bool isValidCellPos(int row, int col) const
    {
        return (row >= 0 && col >= 0 && row < GetNumberRows()
            && col < (int)GetNumberCols());
    }

    int GetNumberRows() const
    {
        if (filterOrSortActiveM)
            return (int)rowMappingM.size();
        return (int)rowsM.size();
    }

    size_t GetNumberCols() const
    {
        return rowsM.empty() ? 0 : rowsM[0].cols.size();
    }

    wxString getInternalFieldValue(size_t row, size_t col) const
    {
        if (row >= rowsM.size() || col >= rowsM[row].cols.size())
            return wxEmptyString;
        return rowsM[row].cols[col];
    }

    bool isInternalFieldNull(size_t row, size_t col) const
    {
        if (row >= rowsM.size() || col >= rowsM[row].isNull.size())
            return true;
        return rowsM[row].isNull[col];
    }

    wxString GetValue(int row, int col)
    {
        if (!isValidCellPos(row, col))
            return wxEmptyString;

        int realRow = getRealRowIndex(row);
        if (realRow < 0)
            return wxEmptyString;

        // On-demand fetching check using view row position
        if (!allRowsFetchedM && !fetchAllRowsM)
        {
            if ((unsigned)row + 20 >= (unsigned)GetNumberRows())
            {
                if (maxRowToFetchM <= (unsigned)rowsM.size())
                    maxRowToFetchM = (unsigned)rowsM.size() + 100;
            }
        }

        if (isInternalFieldNull(realRow, col))
            return "[null]";
        return getInternalFieldValue(realRow, col);
    }

    void sortColumn(int col, bool ascending = true)
    {
        sortedColM = col;
        sortAscendingM = ascending;
        updateRowMapping();
    }

    void filterRows(const wxString& filterText)
    {
        currentFilterTextM = filterText;
        updateRowMapping();
    }

    void clearFilterAndSort()
    {
        currentFilterTextM.Clear();
        sortedColM = -1;
        sortAscendingM = true;
        filterOrSortActiveM = false;
        rowMappingM.clear();
    }

    void appendBatch(const std::vector<RowData>& newRows)
    {
        rowsM.insert(rowsM.end(), newRows.begin(), newRows.end());
        if (filterOrSortActiveM)
            updateRowMapping();
    }

    unsigned getMaxRowToFetch() const { return maxRowToFetchM; }
    void setMaxRowToFetch(unsigned m) { maxRowToFetchM = m; }
    size_t getTotalRowCount() const { return rowsM.size(); }
};

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
    std::cout << "Running DataGrid Sort Partial Fetch Regression Tests (Issue #689)...\n";

    // Test 1: Sorting a partially fetched result set, then appending subsequent rows
    {
        MockDataGridTable table;

        // Initial batch of 5 rows (e.g. out of 10)
        std::vector<RowData> batch1 = {
            { { "50", "Apple" }, { false, false } },
            { { "10", "Banana" }, { false, false } },
            { { "40", "Cherry" }, { false, false } },
            { { "20", "Date" }, { false, false } },
            { { "30", "Elderberry" }, { false, false } }
        };
        table.appendBatch(batch1);

        ok = check(table.GetNumberRows() == 5, "Initial batch has 5 rows") && ok;

        // User sorts by column 0 (numeric) ascending
        table.sortColumn(0, true);

        ok = check(table.GetNumberRows() == 5, "Sorted batch has 5 rows") && ok;
        ok = check(table.GetValue(0, 0) == "10", "Sorted row 0 is 10") && ok;
        ok = check(table.GetValue(1, 0) == "20", "Sorted row 1 is 20") && ok;
        ok = check(table.GetValue(2, 0) == "30", "Sorted row 2 is 30") && ok;
        ok = check(table.GetValue(3, 0) == "40", "Sorted row 3 is 40") && ok;
        ok = check(table.GetValue(4, 0) == "50", "Sorted row 4 is 50") && ok;

        // Second batch arrives (e.g. user scrolled or background fetched next 5 rows)
        std::vector<RowData> batch2 = {
            { { "15", "Fig" }, { false, false } },
            { { "5",  "Grape" }, { false, false } },
            { { "45", "Honeydew" }, { false, false } },
            { { "25", "Kiwi" }, { false, false } },
            { { "35", "Lemon" }, { false, false } }
        };
        table.appendBatch(batch2);

        // Verify that subsequent fetched rows are NOT empty and row count is updated
        ok = check(table.GetNumberRows() == 10, "After second batch, view row count is 10 (not stuck at 5)") && ok;

        for (int r = 0; r < 10; ++r)
        {
            wxString val = table.GetValue(r, 0);
            ok = check(!val.IsEmpty(), wxString::Format("Row %d value is not empty (val=%s)", r, val).c_str()) && ok;
        }

        // Verify full sort order across all 10 rows: 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
        std::vector<wxString> expectedOrder = { "5", "10", "15", "20", "25", "30", "35", "40", "45", "50" };
        bool orderMatches = true;
        for (int r = 0; r < 10; ++r)
        {
            if (table.GetValue(r, 0) != expectedOrder[r])
            {
                orderMatches = false;
                std::cout << "  Mismatch at row " << r << ": expected " << expectedOrder[r]
                          << ", got " << table.GetValue(r, 0) << "\n";
            }
        }
        ok = check(orderMatches, "All 10 rows from both batches are sorted in global ascending order") && ok;
    }

    // Test 2: Descending sort with NULLs across partial batches
    {
        MockDataGridTable table;

        std::vector<RowData> batch1 = {
            { { "100", "A" }, { false, false } },
            { { "",    "B" }, { true, false } }, // NULL
            { { "300", "C" }, { false, false } }
        };
        table.appendBatch(batch1);

        table.sortColumn(0, false); // Descending

        std::vector<RowData> batch2 = {
            { { "200", "D" }, { false, false } },
            { { "",    "E" }, { true, false } }, // NULL
            { { "400", "F" }, { false, false } }
        };
        table.appendBatch(batch2);

        ok = check(table.GetNumberRows() == 6, "Descending sort with NULLs has 6 rows") && ok;

        // Descending sort places NULLs at start, then 400, 300, 200, 100
        ok = check(table.GetValue(0, 0) == "[null]", "Row 0 is NULL in descending sort") && ok;
        ok = check(table.GetValue(1, 0) == "[null]", "Row 1 is NULL in descending sort") && ok;
        ok = check(table.GetValue(2, 0) == "400", "Row 2 is 400 in descending sort") && ok;
        ok = check(table.GetValue(3, 0) == "300", "Row 3 is 300 in descending sort") && ok;
        ok = check(table.GetValue(4, 0) == "200", "Row 4 is 200 in descending sort") && ok;
        ok = check(table.GetValue(5, 0) == "100", "Row 5 is 100 in descending sort") && ok;
    }

    // Test 3: Scrolling boundary check does NOT halt data retrieval when sorted
    {
        MockDataGridTable table;
        std::vector<RowData> batch1;
        for (int i = 0; i < 100; ++i)
        {
            // Reversed order so row 99 in database has value "0", row 0 has value "99"
            batch1.push_back({ { wxString::Format("%d", 99 - i), "item" }, { false, false } });
        }
        table.appendBatch(batch1);
        table.setMaxRowToFetch(100);

        // Sort ascending by column 0.
        // View row 0 = "0" (database row 99), view row 85 = "85" (database row 14)
        table.sortColumn(0, true);

        // In the bug, accessing view row 85 tested realRow (14) + 20 >= 100 (34 >= 100 -> FALSE),
        // which caused maxRowToFetchM to stay 100 and halted further data retrieval!
        // With the fix, view row 85 tests row (85) + 20 >= 100 (105 >= 100 -> TRUE),
        // correctly updating maxRowToFetchM to 200!
        table.GetValue(85, 0);

        ok = check(table.getMaxRowToFetch() == 200,
            "Scrolling near boundary in sorted view triggers fetch boundary increase (maxRowToFetch == 200)") && ok;
    }

    // Test 4: Filter + Sort active across multiple batches
    {
        MockDataGridTable table;
        std::vector<RowData> batch1 = {
            { { "10", "match_banana" }, { false, false } },
            { { "50", "other_apple" },  { false, false } },
            { { "30", "match_cherry" }, { false, false } }
        };
        table.appendBatch(batch1);

        table.filterRows("match");
        table.sortColumn(0, true);

        ok = check(table.GetNumberRows() == 2, "Filtered batch 1 has 2 matches") && ok;
        ok = check(table.GetValue(0, 0) == "10", "Filtered row 0 is 10") && ok;
        ok = check(table.GetValue(1, 0) == "30", "Filtered row 1 is 30") && ok;

        std::vector<RowData> batch2 = {
            { { "20", "match_date" },       { false, false } },
            { { "60", "other_elderberry" }, { false, false } },
            { { "5",  "match_fig" },        { false, false } }
        };
        table.appendBatch(batch2);

        ok = check(table.GetNumberRows() == 4, "Filtered batches 1+2 have 4 matches (not stuck)") && ok;
        ok = check(table.GetValue(0, 0) == "5", "Filtered row 0 is 5") && ok;
        ok = check(table.GetValue(1, 0) == "10", "Filtered row 1 is 10") && ok;
        ok = check(table.GetValue(2, 0) == "20", "Filtered row 2 is 20") && ok;
        ok = check(table.GetValue(3, 0) == "30", "Filtered row 3 is 30") && ok;

        table.clearFilterAndSort();
        ok = check(table.GetNumberRows() == 6, "Clearing filter and sort restores all 6 rows") && ok;
    }

    std::cout << "DataGrid Sort Partial Fetch Regression Tests completed: "
              << (ok ? "ALL PASSED" : "SOME FAILED") << "\n";
    return ok ? 0 : 1;
}
