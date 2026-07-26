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

#include "core/VectorHelper.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace fr
{

bool VectorHelper::parseVectorString(const std::string& input, std::vector<float>& values)
{
    values.clear();
    std::string s = input;

    // Strip whitespace and brackets [] or ()
    size_t first = s.find_first_not_of(" \t\r\n[(");
    size_t last = s.find_last_not_of(" \t\r\n])");
    if (first == std::string::npos || last == std::string::npos || first > last)
        return false;

    s = s.substr(first, last - first + 1);

    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        size_t tfirst = token.find_first_not_of(" \t\r\n");
        size_t tlast = token.find_last_not_of(" \t\r\n");
        if (tfirst != std::string::npos && tlast != std::string::npos)
        {
            token = token.substr(tfirst, tlast - tfirst + 1);
            try
            {
                float val = std::stof(token);
                values.push_back(val);
            }
            catch (...)
            {
                return false;
            }
        }
    }
    return !values.empty();
}

std::string VectorHelper::formatVectorString(const std::vector<float>& values, int precision)
{
    if (values.empty())
        return "[]";

    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0) ss << ", ";
        ss << std::fixed << std::setprecision(precision) << values[i];
    }
    ss << "]";
    return ss.str();
}

float VectorHelper::calculateCosineDistance(const std::vector<float>& v1, const std::vector<float>& v2)
{
    if (v1.size() != v2.size() || v1.empty())
        return 1.0f;

    float dot = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;

    for (size_t i = 0; i < v1.size(); ++i)
    {
        dot += v1[i] * v2[i];
        norm1 += v1[i] * v1[i];
        norm2 += v2[i] * v2[i];
    }

    if (norm1 <= 0.0f || norm2 <= 0.0f)
        return 1.0f;

    float similarity = dot / (std::sqrt(norm1) * std::sqrt(norm2));
    return 1.0f - similarity; // Cosine distance = 1 - Cosine similarity
}

float VectorHelper::calculateL2Distance(const std::vector<float>& v1, const std::vector<float>& v2)
{
    if (v1.size() != v2.size() || v1.empty())
        return 0.0f;

    float sum = 0.0f;
    for (size_t i = 0; i < v1.size(); ++i)
    {
        float diff = v1[i] - v2[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float VectorHelper::calculateInnerProduct(const std::vector<float>& v1, const std::vector<float>& v2)
{
    if (v1.size() != v2.size() || v1.empty())
        return 0.0f;

    float dot = 0.0f;
    for (size_t i = 0; i < v1.size(); ++i)
    {
        dot += v1[i] * v2[i];
    }
    return dot;
}

std::string VectorHelper::generateSimilarityQuery(const std::string& tableName,
                                            const std::string& vectorColumn,
                                            const std::vector<float>& queryVector,
                                            VectorMetric metric,
                                            int limit)
{
    std::string vecStr = formatVectorString(queryVector);
    std::string func;
    std::string order;

    switch (metric)
    {
        case VectorMetric::CosineDistance:
            func = "COSINE_DISTANCE";
            order = "ASC";
            break;
        case VectorMetric::L2Distance:
            func = "L2_DISTANCE";
            order = "ASC";
            break;
        case VectorMetric::InnerProduct:
            func = "INNER_PRODUCT";
            order = "DESC";
            break;
        case VectorMetric::ManhattanDistance:
            func = "L1_DISTANCE";
            order = "ASC";
            break;
    }

    std::stringstream ss;
    ss << "SELECT FIRST " << limit << " *,\n"
       << "       " << func << "(" << vectorColumn << ", VECTOR '" << vecStr << "') AS similarity_score\n"
       << "FROM " << tableName << "\n"
       << "ORDER BY similarity_score " << order << ";";

    return ss.str();
}

bool VectorHelper::isFbVectorInstalled()
{
    // Return true by default if fbvector UDF / Firebird 6 VECTOR module is loaded
    return true;
}

} // namespace fr
