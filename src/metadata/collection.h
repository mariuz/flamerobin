/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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


  $Id$

*/

#ifndef FR_COLLECTION_H
#define FR_COLLECTION_H
//-----------------------------------------------------------------------------
#include <algorithm>

#include <boost/ptr_container/ptr_list.hpp>

#include "metadata/metadataitem.h"
//-----------------------------------------------------------------------------
class MetadataCollectionBase : public MetadataItem
{
public:
    MetadataCollectionBase(NodeType type, MetadataItem* parent,
            const wxString& name)
        : MetadataItem(type, parent, name)
    {
    }

    virtual bool isSystem() const { return false; }
};
//-----------------------------------------------------------------------------
template <class T>
class MetadataCollection : public MetadataCollectionBase,
    public MetadataItemLink<Database>
{
public:
    typedef typename boost::ptr_list<T> CollectionType;
    typedef typename CollectionType::iterator iterator;
    typedef typename CollectionType::const_iterator const_iterator;
private:
    CollectionType itemsM;

    iterator getPosition(wxString name)
    {
        Identifier id(name);
        for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
        {
            if ((*it).getIdentifier().equals(id))
                return it;
        }
        return itemsM.end();
    }

    // helper structs for find_if() and erase_if()
    struct FindByAddress
    {
        T* itemM;

        FindByAddress(T* item) : itemM(item) {}
        bool operator()(const T& item) { return &item == itemM; }
    };

    struct InsertionPosByName
    {
        wxString nameM;

        InsertionPosByName(wxString name) : nameM(name) {}
        bool operator()(const T& item) { return item.getName_() > nameM; }
    };

    struct IsUserItem
    {
        bool operator()(const T& item) const { return !item.isSystem(); }
    };

public:
    MetadataCollection<T>(NodeType type, DatabasePtr database,
            const wxString& name)
        : MetadataCollectionBase(type, database.get(), name),
            MetadataItemLink<Database>(database)
    {
    }

    DatabasePtr getDatabase() const
    {
        return MetadataItemLink<Database>::getLink();
    }

    // inserts new item into list at correct position to preserve alphabetical
    // order of item names, and returns pointer to it
    T* insert(wxString name)
    {
        iterator pos = std::find_if(itemsM.begin(), itemsM.end(),
            InsertionPosByName(name));
        T* item = &(*itemsM.insert(pos, new T(getDatabase(), name)));
        for (unsigned int i = getLockCount(); i > 0; i--)
            item->lockSubject();
        notifyObservers();
        return item;
    }

    // removes item from list
    void remove(T* item)
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
        bool changed = false;
        CollectionType newItems;
        for (size_t i = 0; i < names.size(); ++i)
        {
            wxString itemName(names[i]);
            iterator oldPos = getPosition(itemName);
            if (oldPos == itemsM.end())
            {
                wxLogDebug(wxT("Creating new item \"%s\""), itemName.c_str());
                changed = true;

                newItems.push_back(new T(getDatabase(), names[i]));
                T* item = &newItems.back();
                for (unsigned int j = getLockCount(); j > 0; j--)
                    item->lockSubject();
            }
            else if (oldPos == itemsM.begin())
            {
                wxLogDebug(wxT("Keeping item \"%s\" at same position"),
                    itemName.c_str());
                newItems.transfer(newItems.end(), oldPos, itemsM);
            }
            else
            {
                wxLogDebug(wxT("Moving item \"%s\" to different position"),
                    itemName.c_str());
                changed = true;
                newItems.transfer(newItems.end(), oldPos, itemsM);
            }
        }

        if (!itemsM.empty())
        {
            wxLogDebug(wxT("Deleting all %d items in old vector"),
                itemsM.size());
            changed = true;
            itemsM.clear();
        }
        itemsM.transfer(itemsM.begin(), newItems);

        setChildrenLoaded(true);
        // call notifyObservers() only if any items have been added, moved
        // or deleted
        if (changed)
            notifyObservers();
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
        if (itemsM.size())
        {
            itemsM.clear();
            notifyObservers();
        }
    };

    T* findByName(wxString name)
    {
        iterator it = getPosition(name);
        return (it != itemsM.end()) ? &(*it) : 0;
    };

    virtual bool getChildren(std::vector<MetadataItem *>& temp)         // returns vector of all subnodes
    {
        if (!childrenLoaded())
            return false;
        for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
            temp.push_back(&(*it));
        return (itemsM.size() != 0);
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
            (*it).lockSubject();
    }

    virtual void unlockChildren()
    {
        for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
            (*it).unlockSubject();
    }
};
//-----------------------------------------------------------------------------
#endif
