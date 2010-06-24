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
#include <sstream>

#include <boost/ptr_container/ptr_list.hpp>
#include <boost/function.hpp>

#include "metadata/metadataitem.h"

class ProgressIndicator;
//-----------------------------------------------------------------------------
template <class T>
class MetadataCollection: public MetadataItem
{
public:
    typedef typename boost::ptr_list<T> CollectionType;
    typedef typename CollectionType::iterator iterator;
    typedef typename CollectionType::const_iterator const_iterator;
private:
    CollectionType itemsM;
    boost::function<void (ProgressIndicator*)> loadChildrenProcM;

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
    // removes item from list
    void remove(T* item)
    {
        if (!item)
            return;
        remove(std::find_if(itemsM.begin(), itemsM.end(), 
            FindByAddress(item)));
    }

    // removes item pointed at from list
    void remove(iterator pos)
    {
        if (pos != itemsM.end())
        {
            itemsM.erase(pos);
            notifyObservers();
        }
    }

    // removes items in range from list
    void remove(iterator first, iterator last)
    {
        if (first != last)
        {
            itemsM.erase(first, last);
            notifyObservers();
        }
    }

    // adds new item to end of list and returns pointer to it
    T* add(T& item)
    {
        item.setParent(this);
        if (isLocked())
        {
            for (unsigned int i = getLockCount(); i > 0; i--)
                item.lockSubject();
        }
        itemsM.push_back(new T(item));
        notifyObservers();
        return &itemsM.back();
    }

    // adds new item to end of list and returns pointer to it
    T* add(MetadataItem* parent, wxString name, NodeType type)
    {
        return insert(itemsM.end(), parent, name, type);
    }

    // inserts new item into list at correct position to preserve alphabetical
    // order of item names, and returns pointer to it
    T* insert(MetadataItem* parent, wxString name, NodeType type)
    {
        iterator pos = std::find_if(itemsM.begin(), itemsM.end(),
            InsertionPosByName(name));
        return insert(pos, parent, name, type);
    }

    // inserts new item at given position into list and returns pointer to it
    T* insert(iterator pos, MetadataItem* parent, wxString name,
        NodeType type)
    {
        T* item = &(*itemsM.insert(pos, new T()));
        for (unsigned int i = getLockCount(); i > 0; i--)
            item->lockSubject();
        item->setProperties(parent, name, type);
        notifyObservers();
        return item;
    }

    void moveItem(iterator currentPos, iterator newPos)
    {
        if (currentPos != newPos)
        {
            boost::ptr_list<T> copied;
            // I did not find an easier way to move the item in the list
            copied.transfer(copied.begin(), currentPos, itemsM);
            itemsM.transfer(newPos, copied);
            notifyObservers();
        }
    }

    void setItems(MetadataItem* parent, NodeType type, wxArrayString names)
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

                newItems.push_back(new T());
                T* item = &newItems.back();
                for (unsigned int j = getLockCount(); j > 0; j--)
                    item->lockSubject();
                item->setProperties(parent, names[i], type);
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

    void setUserItems(MetadataItem* parent, NodeType type, wxArrayString names)
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

                newItems.push_back(new T());
                T* item = &newItems.back();
                for (unsigned int j = getLockCount(); j > 0; j--)
                    item->lockSubject();
                item->setProperties(parent, names[i], type);
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
            size_t oldCount = itemsM.size();
            // all remaining items that are not system items must be invalid
            // (have been deleted probably), so delete them from this list
            IsUserItem isUserItem;
            itemsM.erase_if(isUserItem);
            size_t newCount = itemsM.size();

            wxLogDebug(wxT("User items (%d) in old vector deleted,")
                wxT(" %d remaining system items"),
                oldCount - newCount, newCount);
            if (newCount < oldCount)
                changed = true;
        }
        itemsM.transfer(itemsM.begin(), newItems);

        setChildrenLoaded(true);
        // call notifyObservers() only if any items have been added, moved
        // or deleted
        if (changed)
            notifyObservers();
    }

    virtual bool isSystem() const
    {
        return getType() == ntSysTables;
    };

    inline const_iterator begin() const
    {
        if (!childrenLoaded())
            return itemsM.end();
        return itemsM.begin();
    };

    inline const_iterator end() const
    {
        return itemsM.end();
    };

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

    virtual MetadataItem* findByName(wxString name)
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

    void getChildrenNames(std::vector<Identifier>& temp)
    {
        if (!childrenLoaded())
            return;
        for (const_iterator it = itemsM.begin(); it != itemsM.end(); ++it)
            temp.push_back((*it).getIdentifier());
    }

    wxString getCreateSqlTemplate() const        // this could be done better, but I haven't got the idea
    {                                               // function looks like it could be static, but then it
        T dummy;                                    // can't be virtual, and vice versa. So I just create a dummy
        return dummy.getCreateSqlTemplate();        // object to call the function on.
    }

    virtual size_t getChildrenCount() const
    {
        if (!childrenLoaded())
            return 0;
        return itemsM.size();
    }

    void setloadChildrenProc(boost::function<void (ProgressIndicator*)> proc)
    {
        loadChildrenProcM = proc;
    }

protected:
    virtual void loadChildren()
    {
        if (!loadChildrenProcM.empty())
            loadChildrenProcM(0);
    }

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
