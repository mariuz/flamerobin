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

#ifndef FR_COLLECTION_H
#define FR_COLLECTION_H

#include <algorithm>
#include <iterator>
#include <vector>
#include <functional>

#include "metadata/database.h"

class MetadataCollectionBase : public MetadataItem
{
private:
    DatabaseWeakPtr databaseM;
protected:
    MetadataCollectionBase(NodeType type, DatabasePtr database,
            const wxString& name)
        : MetadataItem(type, database.get(), name, -1), databaseM(database)
    {
    }

    // helper structs for find_if()
    struct FindByAddress
    {
        MetadataItem* itemM;

        FindByAddress(MetadataItem* item) : itemM(item) {}
        bool operator()(const MetadataItemPtr item)
        {
            wxASSERT(item);
            return item.get() == itemM;
        }
    };

    struct InsertionPosByName
    {
        wxString nameM;

        InsertionPosByName(const wxString& name) : nameM(name) {}
        bool operator()(const MetadataItemPtr item)
        {
            wxASSERT(item);
            return item->getName_() > nameM;
        }
    };

public:
    virtual DatabasePtr getDatabase() const
    {
        return DatabasePtr(databaseM);
    }

    virtual bool isSystem() const { return false; }
};

template <class T>
class MetadataCollection : public MetadataCollectionBase
{
public:
    typedef typename std::shared_ptr<T> ItemType;
    typedef typename std::vector<ItemType> CollectionType;
    typedef typename CollectionType::iterator iterator;
    typedef typename CollectionType::const_iterator const_iterator;
private:
    CollectionType itemsM;

    iterator getPosition(const wxString& name)
    {
        Identifier id(name);
        for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
        {
            if ((*it)->getIdentifier().equals(id))
                return it;
        }
        return itemsM.end();
    }

    iterator getPositionMetadataId(const int id)
    {
        for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
        {
            if ((*it)->getMetadataId() == id)
                return it;
        }
        return itemsM.end();

    }

protected:
    MetadataCollection<T>(NodeType type, DatabasePtr database,
            const wxString& name)
        : MetadataCollectionBase(type, database, name)
    {
    }

public:
    // inserts new item into list at correct position to preserve alphabetical
    // order of item names, and returns pointer to it
    ItemType insert(const wxString& name)
    {
        iterator pos = std::find_if(itemsM.begin(), itemsM.end(),
            InsertionPosByName(name));
        ItemType item(new T(getDatabase(), name));
        initializeLockCount(item, getLockCount());
        itemsM.insert(pos, item);
        notifyObservers();
        return item;
    }

    // removes item from list
    void remove(MetadataItem* item)
    {
        if (!item)
            return;
        iterator pos = std::find_if(itemsM.begin(), itemsM.end(),
            FindByAddress(item));
        if (pos != itemsM.end())
        {
            itemsM.erase(pos);
            notifyObservers();
        }
    }

    void setItems(wxArrayString names)
    {
        DatabasePtr database = getDatabase();
        CollectionType newItems;
        for (size_t i = 0; i < names.size(); ++i)
        {
            wxString itemName(names[i]);
            iterator oldPos = getPosition(itemName);
            if (oldPos == itemsM.end())
            {
               ItemType item(new T(database, names[i]));
                newItems.push_back(item);
                initializeLockCount(item, getLockCount());
            }
            else
                newItems.push_back(*oldPos);
        }
        setItems(newItems);
    }

    void setInactiveItems(wxArrayString names)
    {
        for (size_t i = 0; i < names.size(); ++i)
        {
            ItemType item = findByName(names[i]);
            if (item) {
                item.get()->setActive(false);
            }
        }
    }

    void setItems(CollectionType items)
    {
        if (itemsM != items)
        {
            itemsM = items;
            notifyObservers();
        }
        setChildrenLoaded(true);
    }

    inline iterator begin()
    {
        if (!childrenLoaded())
            return itemsM.end();
        return itemsM.begin();
    };

    inline iterator end()
    {
        return itemsM.end();
    };

    inline bool empty() const
    {
        if (!childrenLoaded())
            return true;
        return itemsM.empty();
    };

    inline void clear()
    {
        if (!itemsM.empty())
        {
            itemsM.clear();
            notifyObservers();
        }
    };

    ItemType findByName(const wxString& name)
    {
        iterator it = getPosition(name);
        return (it != itemsM.end()) ? (*it) : ItemType();
    };

    ItemType findByMetadataId(const int id)
    {
        iterator it = getPositionMetadataId(id);
        return (it != itemsM.end()) ? (*it) : ItemType();
    }

    // returns vector of all subnodes
    virtual bool getChildren(std::vector<MetadataItem *>& temp)
    {
        if (!childrenLoaded())
            return false;
        std::transform(itemsM.begin(), itemsM.end(),
            std::back_inserter(temp), std::mem_fn(&ItemType::get));
        return !itemsM.empty();
    }

    virtual size_t getChildrenCount() const
    {
        if (!childrenLoaded())
            return 0;
        return itemsM.size();
    }

protected:
    virtual void lockChildren()
    {
        for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
            (*it)->lockSubject();
    }

    virtual void unlockChildren()
    {
        for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
            (*it)->unlockSubject();
    }
};

#endif
