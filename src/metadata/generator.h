/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Michael Hieke, Nando Dessena
*/

#ifndef FR_GENERATOR_H
#define FR_GENERATOR_H
//-----------------------------------------------------------------------------
// TODO: we should have a header (usually setup.h) to hide such differences
#if !defined(_MSC_VER) && !defined(__BORLANDC__)
    #include <stdint.h>
#endif

#include "metadataitem.h"
//-----------------------------------------------------------------------------
// TODO: we should have a header (usually setup.h) to define such missing types
#if defined(_MSC_VER) || defined(__BORLANDC__)
    typedef __int64 int64_t;
#endif
//-----------------------------------------------------------------------------
class Generator: public MetadataItem
{
private:
    int64_t valueM;
    bool valueLoadedM;
    void setValue(int64_t value);
public:
    Generator();
    // overrides MetadataItem::getCreateSqlTemplate()
    std::string getCreateSqlTemplate() const;

    bool loadValue(bool force = false);
    int64_t getValue();

    virtual std::string getPrintableName();
    virtual const std::string getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
