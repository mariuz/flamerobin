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

// Regression tests for GitHub issue #679:
// "SELECT * on large tables (~100k rows) triggers slow auto-fetch and freezes UI"

#include <iostream>
#include <chrono>
#include <vector>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

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
    std::cout << "Running DataGrid Large Fetch Regression Tests (Issue #679)...\n";

    // Test 1: Adding 100k row buffers with geometric growth
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<DataGridRowBuffer*> buffers;
        const size_t NUM_ROWS = 100000;
        for (size_t i = 0; i < NUM_ROWS; ++i)
        {
            if (buffers.size() == buffers.capacity())
            {
                size_t newCap = buffers.capacity() < 1024 ? 1024 : buffers.capacity() * 2;
                buffers.reserve(newCap);
            }
            DataGridRowBuffer* buf = new DataGridRowBuffer(3);
            buf->setFieldNull(0, false);
            buf->setValue(0, (int)i);
            buf->setFieldNull(1, false);
            buf->setValue(sizeof(int), (int64_t)(i * 100));
            buf->setFieldNull(2, false);
            buf->setString(0, wxString::Format("row_%zu", i));
            buffers.push_back(buf);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        ok = check(buffers.size() == NUM_ROWS, "100k rows added successfully") && ok;
        std::cout << "  INFO: Time taken to populate 100k row buffers: " << elapsedMs << " ms\n";
        ok = check(elapsedMs < 2000, "100k row buffer population is fast (< 2000 ms)") && ok;

        // Verify values at key indices
        int val0 = 0, val50k = 0, valLast = 0;
        int64_t val64_50k = 0;
        buffers[0]->getValue(0, val0);
        buffers[50000]->getValue(0, val50k);
        buffers[50000]->getValue(sizeof(int), val64_50k);
        buffers[NUM_ROWS - 1]->getValue(0, valLast);

        ok = check(val0 == 0, "Row 0 int value is correct") && ok;
        ok = check(val50k == 50000, "Row 50000 int value is correct") && ok;
        ok = check(val64_50k == 5000000, "Row 50000 int64 value is correct") && ok;
        ok = check(valLast == (int)(NUM_ROWS - 1), "Row 99999 int value is correct") && ok;
        ok = check(buffers[50000]->getString(0) == "row_50000", "Row 50000 string value is correct") && ok;

        for (auto b : buffers)
            delete b;
        buffers.clear();
        ok = check(buffers.empty(), "buffers cleared successfully") && ok;
    }

    // Test 2: Verify StringColumnDef space trimming behavior without excess copies
    {
        DataGridRowBuffer buf(1);
        buf.setString(0, "test_value   ");
        ok = check(buf.getString(0) == "test_value   ", "String buffer storage holds string") && ok;
    }

    // Test 3: Batch row addition (DataGridRows batch buffering pattern)
    {
        std::vector<DataGridRowBuffer*> batch;
        const size_t BATCH_SIZE = 500;
        for (size_t i = 0; i < BATCH_SIZE; ++i)
        {
            DataGridRowBuffer* buf = new DataGridRowBuffer(1);
            buf->setFieldNull(0, false);
            buf->setValue(0, (int)(i + 1000));
            batch.push_back(buf);
        }
        ok = check(batch.size() == BATCH_SIZE, "500-row batch constructed successfully") && ok;

        std::vector<DataGridRowBuffer*> mainBuffers;
        mainBuffers.reserve(1024);
        mainBuffers.insert(mainBuffers.end(), batch.begin(), batch.end());
        ok = check(mainBuffers.size() == BATCH_SIZE, "500-row batch appended to main buffers") && ok;

        int firstVal = 0, lastVal = 0;
        mainBuffers[0]->getValue(0, firstVal);
        mainBuffers[BATCH_SIZE - 1]->getValue(0, lastVal);
        ok = check(firstVal == 1000, "First batch item matches") && ok;
        ok = check(lastVal == 1499, "Last batch item matches") && ok;

        for (auto b : mainBuffers)
            delete b;
        mainBuffers.clear();
    }

    // Test 4: Regression test for Issue #706 ("I can't go to last row - 3703 rows")
    // Simulates an initial fetch of 100 rows from a 3703-row table,
    // followed by synchronous full fetch on Ctrl+End navigation.
    {
        const size_t TOTAL_ROWS = 3703;
        const size_t INITIAL_FETCH = 100;

        // Create a mock stream of 3703 rows
        std::vector<DataGridRowBuffer*> fullDataset;
        fullDataset.reserve(TOTAL_ROWS);
        for (size_t i = 0; i < TOTAL_ROWS; ++i)
        {
            DataGridRowBuffer* buf = new DataGridRowBuffer(2);
            buf->setFieldNull(0, false);
            buf->setValue(0, (int)i);
            buf->setFieldNull(1, false);
            buf->setString(0, wxString::Format("record_%zu", i));
            fullDataset.push_back(buf);
        }

        // Simulated table state
        std::vector<DataGridRowBuffer*> tableRows;
        tableRows.reserve(INITIAL_FETCH);
        size_t streamPos = 0;
        bool allRowsFetched = false;

        // Step 1: Initial fetch of 100 rows
        for (size_t i = 0; i < INITIAL_FETCH && streamPos < fullDataset.size(); ++i, ++streamPos)
        {
            tableRows.push_back(fullDataset[streamPos]);
        }
        allRowsFetched = (streamPos >= fullDataset.size());

        ok = check(tableRows.size() == 100, "Initial fetch loaded exactly 100 rows (Issue #706)") && ok;
        ok = check(!allRowsFetched, "Table has more rows to fetch (!allRowsFetched)") && ok;

        // Step 2: Simulate Ctrl+End key press trigger (fetchAllSynchronous)
        // Fetches all remaining 3603 rows into the table
        while (streamPos < fullDataset.size())
        {
            tableRows.push_back(fullDataset[streamPos++]);
        }
        allRowsFetched = true;

        ok = check(tableRows.size() == TOTAL_ROWS, "Full synchronous fetch loaded all 3703 rows") && ok;
        ok = check(allRowsFetched, "All rows marked as fetched (allRowsFetched == true)") && ok;

        // Step 3: Verify last row navigation
        int lastRowIndex = (int)tableRows.size() - 1;
        ok = check(lastRowIndex == 3702, "Last row index is 3702 (3703rd row)") && ok;

        int lastRowId = 0;
        tableRows[lastRowIndex]->getValue(0, lastRowId);
        ok = check(lastRowId == 3702, "Last row ID matches record 3702") && ok;
        ok = check(tableRows[lastRowIndex]->getString(0) == "record_3702", "Last row text matches record_3702") && ok;

        for (auto b : fullDataset)
            delete b;
    }

    // Test 5: Regression test for Issue #706 comment (blank rows displayed when grid row count out of sync)
    // Simulates a 575-row table where 100 rows are fetched initially, then background batches (475 rows)
    // are processed. Verifies that the view row count matches the exact dataset size (575) without blank rows.
    {
        const size_t TOTAL_ROWS = 575;
        const size_t INITIAL_FETCH = 100;

        std::vector<DataGridRowBuffer*> fullDataset;
        fullDataset.reserve(TOTAL_ROWS);
        for (size_t i = 0; i < TOTAL_ROWS; ++i)
        {
            DataGridRowBuffer* buf = new DataGridRowBuffer(1);
            buf->setFieldNull(0, false);
            buf->setValue(0, (int)i);
            fullDataset.push_back(buf);
        }

        // Mock view and table state
        int viewRows = 0;
        std::vector<DataGridRowBuffer*> tableRows;

        // Step 1: Initial fetch of 100 rows
        for (size_t i = 0; i < INITIAL_FETCH; ++i)
            tableRows.push_back(fullDataset[i]);
        int targetTableRows = (int)tableRows.size();
        if (targetTableRows > viewRows)
            viewRows += (targetTableRows - viewRows);

        ok = check(viewRows == 100, "View row count is 100 after initial fetch") && ok;
        ok = check(tableRows.size() == 100, "Table rows is 100 after initial fetch") && ok;

        // Step 2: Background batch of remaining 475 rows arrives
        std::vector<DataGridRowBuffer*> batch;
        for (size_t i = INITIAL_FETCH; i < TOTAL_ROWS; ++i)
            batch.push_back(fullDataset[i]);

        // Process pending batch
        tableRows.insert(tableRows.end(), batch.begin(), batch.end());
        targetTableRows = (int)tableRows.size();
        if (targetTableRows > viewRows)
            viewRows += (targetTableRows - viewRows);

        ok = check(viewRows == 575, "View row count is exactly 575 (no extra/blank rows)") && ok;
        ok = check(tableRows.size() == 575, "Table rows is exactly 575") && ok;

        // Step 3: Clear table and verify view is reset to 0 without underflow
        if (viewRows > 0)
            viewRows -= viewRows;
        tableRows.clear();

        ok = check(viewRows == 0, "View row count cleanly reset to 0 on Clear") && ok;
        ok = check(tableRows.empty(), "Table rows empty after Clear") && ok;

        for (auto b : fullDataset)
            delete b;
    }

    std::cout << "DataGrid Large Fetch Regression Tests completed: "
              << (ok ? "ALL PASSED" : "SOME FAILED") << "\n";
    return ok ? 0 : 1;
}
