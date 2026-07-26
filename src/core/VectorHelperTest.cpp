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

#include <iostream>
#include <cmath>
#include "core/VectorHelper.h"

int main()
{
    bool ok = true;

    // Test 1: parse and format vector
    std::string input = "[0.12, 0.45, -0.89, 0.33]";
    std::vector<float> values;
    if (!fr::VectorHelper::parseVectorString(input, values) || values.size() != 4)
    {
        std::cerr << "FAIL: parseVectorString" << std::endl;
        ok = false;
    }

    std::string formatted = fr::VectorHelper::formatVectorString(values, 2);
    if (formatted != "[0.12, 0.45, -0.89, 0.33]")
    {
        std::cerr << "FAIL: formatVectorString output: " << formatted << std::endl;
        ok = false;
    }

    // Test 2: cosine distance
    std::vector<float> v1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {1.0f, 0.0f, 0.0f};
    if (std::abs(fr::VectorHelper::calculateCosineDistance(v1, v2)) > 0.001f)
    {
        std::cerr << "FAIL: calculateCosineDistance identical" << std::endl;
        ok = false;
    }

    // Test 3: L2 distance
    std::vector<float> p1 = {0.0f, 0.0f};
    std::vector<float> p2 = {3.0f, 4.0f};
    if (std::abs(fr::VectorHelper::calculateL2Distance(p1, p2) - 5.0f) > 0.001f)
    {
        std::cerr << "FAIL: calculateL2Distance" << std::endl;
        ok = false;
    }

    // Test 4: query generation
    std::vector<float> q = {0.1f, 0.2f};
    std::string sql = fr::VectorHelper::generateSimilarityQuery("DOCUMENTS", "EMBEDDING", q, fr::VectorMetric::CosineDistance, 5);
    if (sql.find("SELECT FIRST 5") == std::string::npos || sql.find("COSINE_DISTANCE(EMBEDDING, VECTOR '[0.1000, 0.2000]')") == std::string::npos)
    {
        std::cerr << "FAIL: generateSimilarityQuery: " << sql << std::endl;
        ok = false;
    }

    if (ok)
    {
        std::cout << "All VectorHelper tests passed successfully!" << std::endl;
        return 0;
    }
    return 1;
}
