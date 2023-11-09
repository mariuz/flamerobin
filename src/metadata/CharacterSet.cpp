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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "core/FRError.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/CharacterSet.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"


CollationPtr CharacterSet::findCollation(const wxString& name) const
{
    for (CollationPtrs::const_iterator it = collationsM.begin();
        it != collationsM.end(); ++it)
    {
        if ((*it)->getName_() == name)
            return *it;
    }
    return CollationPtr();
}

std::string CharacterSet::getLoadStatement(bool list)
{
    std::string stmt("select 	"
                     "c.rdb$character_set_name,  " //1
                     "c.RDB$CHARACTER_SET_ID,    " //2
                     "c.RDB$BYTES_PER_CHARACTER, " //3
                     "c.RDB$DEFAULT_COLLATE_NAME " //4
                     "from rdb$character_sets c  "
        );
    if (list)
    {
        stmt += " order by c.rdb$character_set_name";
    }
    else
        stmt += " where c.rdb$character_set_name = ?";
    return stmt;
}

void CharacterSet::loadProperties(IBPP::Statement& statement, wxMBConv* converter)
{
    setPropertiesLoaded(false);

    int Lid;
    std::string Lstr;
    statement->Get(2, Lid);
    setMetadataId(Lid);
    statement->Get(3, Lid);
    setBytesPerChar(Lid);
    
    statement->Get(4, Lstr);
    setCollationDefault(std2wxIdentifier(Lstr, converter));

    setPropertiesLoaded(true);
}

void CharacterSet::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(getLoadStatement(false));
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    if (!st1->Fetch())
        throw FRError(_("Exception not found: ") + getName_());

    loadProperties(st1, converter);
}

void CharacterSet::loadChildren()
{
    bool childrenWereLoaded = childrenLoaded();
    setChildrenLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(db.get());
    wxMBConv* converter = db->getCharsetConverter();

    std::string sql(
        "Select  k.RDB$COLLATION_NAME, c.RDB$CHARACTER_SET_ID, "
        "        c.RDB$BYTES_PER_CHARACTER, k.RDB$COLLATION_ID, c.RDB$DEFAULT_COLLATE_NAME "
        "From RDB$CHARACTER_SETS c "
        "    Join RDB$COLLATIONS k on c.RDB$CHARACTER_SET_ID = k.RDB$CHARACTER_SET_ID "
        "Where c.RDB$CHARACTER_SET_NAME = ? "
        "Order by c.RDB$CHARACTER_SET_NAME, k.RDB$COLLATION_ID "
    );

    IBPP::Statement st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();

    CollationPtrs collations;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString name(std2wxIdentifier(s, converter));
        
        CollationPtr coll = findCollation(name);
        if (!coll) {
            coll.reset(new Collation(this, name));
            initializeLockCount(coll, getLockCount());
        }
        collations.push_back(coll);


    }

    setChildrenLoaded(true);

    if (!childrenWereLoaded)
    {
        collationsM.swap(collations);
        notifyObservers();
    }


}

void CharacterSet::lockChildren()
{
    std::for_each(collationsM.begin(), collationsM.end(),
        std::mem_fn(&Collation::lockSubject));
}

void CharacterSet::unlockChildren()
{
    std::for_each(collationsM.begin(), collationsM.end(),
        std::mem_fn(&Collation::unlockSubject));
}

CharacterSet::CharacterSet()
    :MetadataItem()
{
}

CharacterSet::CharacterSet(DatabasePtr database, const wxString& name, int id, int bytesPerChar)
    :MetadataItem(ntCharacterSet, database.get(), name, id), bytesPerCharM(bytesPerChar), 
    collationDefaultM("")

{
}


CharacterSet::~CharacterSet()
{
}

CollationPtrs::iterator CharacterSet::begin()
{
    return collationsM.begin();
}

CollationPtrs::iterator CharacterSet::end()
{
    return collationsM.end();
}

CollationPtrs::const_iterator CharacterSet::begin() const
{
    return collationsM.begin();
}

CollationPtrs::const_iterator CharacterSet::end() const
{
    return collationsM.end();
}


int CharacterSet::getBytesPerChar() const
{
    return bytesPerCharM;
}

void CharacterSet::setBytesPerChar(int bytes)
{
    bytesPerCharM = bytes;         
}

wxString CharacterSet::getCollationDefault() const
{
    return collationDefaultM;
}

void CharacterSet::setCollationDefault(wxString collation)
{
    collationDefaultM = collation;
}

const wxString CharacterSet::getTypeName() const
{
    return "CHARACTERSET";
}

void CharacterSet::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitCharacterSet(*this);
}

bool CharacterSet::getChildren(std::vector<MetadataItem*>& temp)
{
    if (collationsM.empty() )
        return false;
    std::transform(collationsM.begin(), collationsM.end(),
        std::back_inserter(temp), std::mem_fn(&CollationPtr::get));
    return !collationsM.empty() ;
}

wxArrayString CharacterSet::getCollations()
{
    ensureChildrenLoaded();
    std::vector<MetadataItem*> items;
    wxArrayString temp;
    getChildren(items);
    for (MetadataItem* coll : items)
        temp.push_back(coll->getName_());
    return temp;
}


void CharacterSets::loadChildren()
{
    load(0);
}

CharacterSets::CharacterSets(DatabasePtr database)
    : MetadataCollection<CharacterSet>(ntChartersets, database, _("CharacterSets"))

{
}

void CharacterSets::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitCharacterSets(*this);
}

void CharacterSets::load(ProgressIndicator* progressIndicator)
{
 
    
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        CharacterSet::getLoadStatement(true));

    CollectionType characters;
    st1->Execute();
    checkProgressIndicatorCanceled(progressIndicator);
    while (st1->Fetch())
    {
        if (!st1->IsNull(1))
        {
            std::string s;
            st1->Get(1, s);
            wxString name(std2wxIdentifier(s, converter));

            CharacterSetPtr character = findByName(name);
            if (!character)
            {
                character.reset(new CharacterSet(db, name));
                initializeLockCount(character, getLockCount());
            }
            characters.push_back(character);
            character->loadProperties(st1, converter);
        }
        checkProgressIndicatorCanceled(progressIndicator);
    }

    setItems(characters);

    
    /*
    DatabasePtr db = getDatabase();
    wxString stmt("select rdb$character_set_name, "
        "RDB$CHARACTER_SET_ID , "
        "c.RDB$BYTES_PER_CHARACTER, "
        "c.RDB$DEFAULT_COLLATE_NAME "
        "from rdb$character_sets "
        " order by rdb$character_set_name "
    );
    setItems(db->loadIdentifiers(stmt, progressIndicator));*/
}

const wxString CharacterSets::getTypeName() const
{
    return  "CHARACTERSET_COLLECTION";
}
