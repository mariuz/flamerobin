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

#ifndef FR_VECTORHELPER_H
#define FR_VECTORHELPER_H

#include <wx/wx.h>
#include <vector>
#include <string>

namespace fr
{

enum class VectorMetric
{
    CosineDistance,
    L2Distance,
    InnerProduct,
    ManhattanDistance
};

class VectorHelper
{
public:
    // Parse vector text format, e.g. "[0.12, 0.45, -0.89, 0.33]"
    static bool parseVectorString(const std::string& input, std::vector<float>& values);

    // Format vector floats into Firebird VECTOR string
    static std::string formatVectorString(const std::vector<float>& values, int precision = 4);

    // Calculate similarity metrics between two vectors
    static float calculateCosineDistance(const std::vector<float>& v1, const std::vector<float>& v2);
    static float calculateL2Distance(const std::vector<float>& v1, const std::vector<float>& v2);
    static float calculateInnerProduct(const std::vector<float>& v1, const std::vector<float>& v2);

    // Generate Firebird 6 / fbvector similarity query snippet
    static std::string generateSimilarityQuery(const std::string& tableName,
                                                const std::string& vectorColumn,
                                                const std::vector<float>& queryVector,
                                                VectorMetric metric,
                                                int limit = 10);

    // Check if Firebird engine has fbvector extension installed or native VECTOR support
    static bool isFbVectorInstalled();
};

} // namespace fr

#endif // FR_VECTORHELPER_H
