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

#ifndef FR_METADATAITEMURIHANDLERHELPER_H
#define FR_METADATAITEMURIHANDLERHELPER_H

#include "core/URIProcessor.h"
#include "metadata/MetadataClasses.h"
#include "metadata/metadataitem.h"

// URI parsing helper for metadata-related URIHandlers.
class MetadataItemURIHandlerHelper
{
private:
    MetadataItem* doExtractMetadataItemFromURI(const URI& uri);
protected:
    template<class T>
    inline T* extractMetadataItemFromURI(const URI& uri)
    {
        return dynamic_cast<T*>(doExtractMetadataItemFromURI(uri));
    }

    template<class T>
    std::shared_ptr<T> extractMetadataItemPtrFromURI(const URI& uri)
    {
        if (T* t = extractMetadataItemFromURI<T>(uri))
            return t->shared_from_this();
        return std::shared_ptr<T>();
    }
};

template<>
inline MetadataItem*
MetadataItemURIHandlerHelper::extractMetadataItemFromURI<MetadataItem>(
    const URI& uri)
{
    return doExtractMetadataItemFromURI(uri);
}

#endif // FR_METADATAITEMURIHANDLERHELPER_H
