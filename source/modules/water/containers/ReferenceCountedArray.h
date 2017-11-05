/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#ifndef WATER_REFERENCECOUNTEDARRAY_H_INCLUDED
#define WATER_REFERENCECOUNTEDARRAY_H_INCLUDED

#include "../memory/ReferenceCountedObject.h"

namespace water {

//==============================================================================
/**
    Holds a list of objects derived from ReferenceCountedObject, or which implement basic
    reference-count handling methods.

    The template parameter specifies the class of the object you want to point to - the easiest
    way to make a class reference-countable is to simply make it inherit from ReferenceCountedObject
    or SingleThreadedReferenceCountedObject, but if you need to, you can roll your own reference-countable
    class by implementing a set of methods called incReferenceCount(), decReferenceCount(), and
    decReferenceCountWithoutDeleting(). See ReferenceCountedObject for examples of how these methods
    should behave.

    A ReferenceCountedArray holds objects derived from ReferenceCountedObject,
    and takes care of incrementing and decrementing their ref counts when they
    are added and removed from the array.

    To make all the array's methods thread-safe, pass in "CriticalSection" as the templated
    TypeOfCriticalSectionToUse parameter, instead of the default DummyCriticalSection.

    @see Array, OwnedArray, StringArray
*/
template <class ObjectClass>
class ReferenceCountedArray
{
public:
    typedef ReferenceCountedObjectPtr<ObjectClass> ObjectClassPtr;

    //==============================================================================
    /** Creates an empty array.
        @see ReferenceCountedObject, Array, OwnedArray
    */
    ReferenceCountedArray() noexcept
        : numUsed (0)
    {
    }

    /** Creates a copy of another array */
    ReferenceCountedArray (const ReferenceCountedArray& other) noexcept
    {
        numUsed = other.size();
        data.setAllocatedSize (numUsed);
        memcpy (data.elements, other.getRawDataPointer(), (size_t) numUsed * sizeof (ObjectClass*));

        for (int i = numUsed; --i >= 0;)
            if (ObjectClass* o = data.elements[i])
                o->incReferenceCount();
    }

    /** Creates a copy of another array */
    template <class OtherObjectClass>
    ReferenceCountedArray (const ReferenceCountedArray<OtherObjectClass>& other) noexcept
    {
        numUsed = other.size();
        data.setAllocatedSize (numUsed);
        memcpy (data.elements, other.getRawDataPointer(), numUsed * sizeof (ObjectClass*));

        for (int i = numUsed; --i >= 0;)
            if (ObjectClass* o = data.elements[i])
                o->incReferenceCount();
    }

    /** Copies another array into this one.
        Any existing objects in this array will first be released.
    */
    ReferenceCountedArray& operator= (const ReferenceCountedArray& other) noexcept
    {
        ReferenceCountedArray otherCopy (other);
        swapWith (otherCopy);
        return *this;
    }

    /** Copies another array into this one.
        Any existing objects in this array will first be released.
    */
    template <class OtherObjectClass>
    ReferenceCountedArray<ObjectClass>& operator= (const ReferenceCountedArray<OtherObjectClass>& other) noexcept
    {
        ReferenceCountedArray<ObjectClass> otherCopy (other);
        swapWith (otherCopy);
        return *this;
    }

    /** Destructor.
        Any objects in the array will be released, and may be deleted if not referenced from elsewhere.
    */
    ~ReferenceCountedArray()
    {
        releaseAllObjects();
    }

    //==============================================================================
    /** Removes all objects from the array.
        Any objects in the array that whose reference counts drop to zero will be deleted.
    */
    void clear()
    {
        releaseAllObjects();
        data.setAllocatedSize (0);
    }

    /** Removes all objects from the array without freeing the array's allocated storage.
        Any objects in the array that whose reference counts drop to zero will be deleted.
        @see clear
    */
    void clearQuick()
    {
        releaseAllObjects();
    }

    /** Returns the current number of objects in the array. */
    inline int size() const noexcept
    {
        return numUsed;
    }

    /** Returns true if the array is empty, false otherwise. */
    inline bool isEmpty() const noexcept
    {
        return size() == 0;
    }

    /** Returns a pointer to the object at this index in the array.

        If the index is out-of-range, this will return a null pointer, (and
        it could be null anyway, because it's ok for the array to hold null
        pointers as well as objects).

        @see getUnchecked
    */
    inline ObjectClassPtr operator[] (const int index) const noexcept
    {
        return getObjectPointer (index);
    }

    /** Returns a pointer to the object at this index in the array, without checking
        whether the index is in-range.

        This is a faster and less safe version of operator[] which doesn't check the index passed in, so
        it can be used when you're sure the index is always going to be legal.
    */
    inline ObjectClassPtr getUnchecked (const int index) const noexcept
    {
        return getObjectPointerUnchecked (index);
    }

    /** Returns a raw pointer to the object at this index in the array.

        If the index is out-of-range, this will return a null pointer, (and
        it could be null anyway, because it's ok for the array to hold null
        pointers as well as objects).

        @see getUnchecked
    */
    inline ObjectClass* getObjectPointer (const int index) const noexcept
    {
        if (isPositiveAndBelow (index, numUsed))
        {
            jassert (data.elements != nullptr);
            return data.elements [index];
        }

        return ObjectClassPtr();
    }

    /** Returns a raw pointer to the object at this index in the array, without checking
        whether the index is in-range.
    */
    inline ObjectClass* getObjectPointerUnchecked (const int index) const noexcept
    {
        jassert (isPositiveAndBelow (index, numUsed) && data.elements != nullptr);
        return data.elements [index];
    }

    /** Returns a pointer to the first object in the array.

        This will return a null pointer if the array's empty.
        @see getLast
    */
    inline ObjectClassPtr getFirst() const noexcept
    {
        if (numUsed > 0)
        {
            jassert (data.elements != nullptr);
            return data.elements [0];
        }

        return ObjectClassPtr();
    }

    /** Returns a pointer to the last object in the array.

        This will return a null pointer if the array's empty.
        @see getFirst
    */
    inline ObjectClassPtr getLast() const noexcept
    {
        if (numUsed > 0)
        {
            jassert (data.elements != nullptr);
            return data.elements [numUsed - 1];
        }

        return ObjectClassPtr();
    }

    /** Returns a pointer to the actual array data.
        This pointer will only be valid until the next time a non-const method
        is called on the array.
    */
    inline ObjectClass** getRawDataPointer() const noexcept
    {
        return data.elements;
    }

    //==============================================================================
    /** Returns a pointer to the first element in the array.
        This method is provided for compatibility with standard C++ iteration mechanisms.
    */
    inline ObjectClass** begin() const noexcept
    {
        return data.elements;
    }

    /** Returns a pointer to the element which follows the last element in the array.
        This method is provided for compatibility with standard C++ iteration mechanisms.
    */
    inline ObjectClass** end() const noexcept
    {
        return data.elements + numUsed;
    }

    //==============================================================================
    /** Finds the index of the first occurrence of an object in the array.

        @param objectToLookFor    the object to look for
        @returns                  the index at which the object was found, or -1 if it's not found
    */
    int indexOf (const ObjectClass* const objectToLookFor) const noexcept
    {
        ObjectClass** e = data.elements.getData();
        ObjectClass** const endPointer = e + numUsed;

        while (e != endPointer)
        {
            if (objectToLookFor == *e)
                return static_cast<int> (e - data.elements.getData());

            ++e;
        }

        return -1;
    }

    /** Returns true if the array contains a specified object.

        @param objectToLookFor      the object to look for
        @returns                    true if the object is in the array
    */
    bool contains (const ObjectClass* const objectToLookFor) const noexcept
    {
        ObjectClass** e = data.elements.getData();
        ObjectClass** const endPointer = e + numUsed;

        while (e != endPointer)
        {
            if (objectToLookFor == *e)
                return true;

            ++e;
        }

        return false;
    }

    /** Appends a new object to the end of the array.

        This will increase the new object's reference count.

        @param newObject       the new object to add to the array
        @see set, insert, addIfNotAlreadyThere, addSorted, addArray
    */
    ObjectClass* add (ObjectClass* const newObject) noexcept
    {
        data.ensureAllocatedSize (numUsed + 1);
        jassert (data.elements != nullptr);
        data.elements [numUsed++] = newObject;

        if (newObject != nullptr)
            newObject->incReferenceCount();

        return newObject;
    }

    /** Inserts a new object into the array at the given index.

        If the index is less than 0 or greater than the size of the array, the
        element will be added to the end of the array.
        Otherwise, it will be inserted into the array, moving all the later elements
        along to make room.

        This will increase the new object's reference count.

        @param indexToInsertAt      the index at which the new element should be inserted
        @param newObject            the new object to add to the array
        @see add, addSorted, addIfNotAlreadyThere, set
    */
    ObjectClass* insert (int indexToInsertAt,
                         ObjectClass* const newObject) noexcept
    {
        if (indexToInsertAt < 0)
            return add (newObject);

        if (indexToInsertAt > numUsed)
            indexToInsertAt = numUsed;

        data.ensureAllocatedSize (numUsed + 1);
        jassert (data.elements != nullptr);

        ObjectClass** const e = data.elements + indexToInsertAt;
        const int numToMove = numUsed - indexToInsertAt;

        if (numToMove > 0)
            memmove (e + 1, e, sizeof (ObjectClass*) * (size_t) numToMove);

        *e = newObject;

        if (newObject != nullptr)
            newObject->incReferenceCount();

        ++numUsed;

        return newObject;
    }

    /** Appends a new object at the end of the array as long as the array doesn't
        already contain it.

        If the array already contains a matching object, nothing will be done.

        @param newObject   the new object to add to the array
        @returns           true if the object has been added, false otherwise
    */
    bool addIfNotAlreadyThere (ObjectClass* const newObject) noexcept
    {
        if (contains (newObject))
            return false;

        add (newObject);
        return true;
    }

    /** Replaces an object in the array with a different one.

        If the index is less than zero, this method does nothing.
        If the index is beyond the end of the array, the new object is added to the end of the array.

        The object being added has its reference count increased, and if it's replacing
        another object, then that one has its reference count decreased, and may be deleted.

        @param indexToChange        the index whose value you want to change
        @param newObject            the new value to set for this index.
        @see add, insert, remove
    */
    void set (const int indexToChange,
              ObjectClass* const newObject)
    {
        if (indexToChange >= 0)
        {
            if (newObject != nullptr)
                newObject->incReferenceCount();

            if (indexToChange < numUsed)
            {
                if (ObjectClass* o = data.elements [indexToChange])
                    releaseObject (o);

                data.elements [indexToChange] = newObject;
            }
            else
            {
                data.ensureAllocatedSize (numUsed + 1);
                jassert (data.elements != nullptr);
                data.elements [numUsed++] = newObject;
            }
        }
    }

    /** Adds elements from another array to the end of this array.

        @param arrayToAddFrom       the array from which to copy the elements
        @param startIndex           the first element of the other array to start copying from
        @param numElementsToAdd     how many elements to add from the other array. If this
                                    value is negative or greater than the number of available elements,
                                    all available elements will be copied.
        @see add
    */
    void addArray (const ReferenceCountedArray<ObjectClass>& arrayToAddFrom,
                   int startIndex = 0,
                   int numElementsToAdd = -1) noexcept
    {
        if (startIndex < 0)
        {
            jassertfalse;
            startIndex = 0;
        }

        if (numElementsToAdd < 0 || startIndex + numElementsToAdd > arrayToAddFrom.size())
            numElementsToAdd = arrayToAddFrom.size() - startIndex;

        if (numElementsToAdd > 0)
        {
            data.ensureAllocatedSize (numUsed + numElementsToAdd);

            while (--numElementsToAdd >= 0)
                add (arrayToAddFrom.getUnchecked (startIndex++));
        }
    }

    /** Inserts a new object into the array assuming that the array is sorted.

        This will use a comparator to find the position at which the new object
        should go. If the array isn't sorted, the behaviour of this
        method will be unpredictable.

        @param comparator   the comparator object to use to compare the elements - see the
                            sort() method for details about this object's form
        @param newObject    the new object to insert to the array
        @returns the index at which the new object was added
        @see add, sort
    */
    template <class ElementComparator>
    int addSorted (ElementComparator& comparator, ObjectClass* newObject) noexcept
    {
        const int index = findInsertIndexInSortedArray (comparator, data.elements.getData(), newObject, 0, numUsed);
        insert (index, newObject);
        return index;
    }

    /** Inserts or replaces an object in the array, assuming it is sorted.

        This is similar to addSorted, but if a matching element already exists, then it will be
        replaced by the new one, rather than the new one being added as well.
    */
    template <class ElementComparator>
    void addOrReplaceSorted (ElementComparator& comparator,
                             ObjectClass* newObject) noexcept
    {
        const int index = findInsertIndexInSortedArray (comparator, data.elements.getData(), newObject, 0, numUsed);

        if (index > 0 && comparator.compareElements (newObject, data.elements [index - 1]) == 0)
            set (index - 1, newObject); // replace an existing object that matches
        else
            insert (index, newObject);  // no match, so insert the new one
    }

    /** Finds the index of an object in the array, assuming that the array is sorted.

        This will use a comparator to do a binary-chop to find the index of the given
        element, if it exists. If the array isn't sorted, the behaviour of this
        method will be unpredictable.

        @param comparator           the comparator to use to compare the elements - see the sort()
                                    method for details about the form this object should take
        @param objectToLookFor      the object to search for
        @returns                    the index of the element, or -1 if it's not found
        @see addSorted, sort
    */
    template <class ElementComparator>
    int indexOfSorted (ElementComparator& comparator,
                       const ObjectClass* const objectToLookFor) const noexcept
    {
        ignoreUnused (comparator);
        int s = 0, e = numUsed;

        while (s < e)
        {
            if (comparator.compareElements (objectToLookFor, data.elements [s]) == 0)
                return s;

            const int halfway = (s + e) / 2;
            if (halfway == s)
                break;

            if (comparator.compareElements (objectToLookFor, data.elements [halfway]) >= 0)
                s = halfway;
            else
                e = halfway;
        }

        return -1;
    }

    //==============================================================================
    /** Removes an object from the array.

        This will remove the object at a given index and move back all the
        subsequent objects to close the gap.

        If the index passed in is out-of-range, nothing will happen.

        The object that is removed will have its reference count decreased,
        and may be deleted if not referenced from elsewhere.

        @param indexToRemove    the index of the element to remove
        @see removeObject, removeRange
    */
    void remove (const int indexToRemove)
    {
        if (isPositiveAndBelow (indexToRemove, numUsed))
        {
            ObjectClass** const e = data.elements + indexToRemove;

            if (ObjectClass* o = *e)
                releaseObject (o);

            --numUsed;
            const int numberToShift = numUsed - indexToRemove;

            if (numberToShift > 0)
                memmove (e, e + 1, sizeof (ObjectClass*) * (size_t) numberToShift);

            if ((numUsed << 1) < data.numAllocated)
                minimiseStorageOverheads();
        }
    }

    /** Removes and returns an object from the array.

        This will remove the object at a given index and return it, moving back all
        the subsequent objects to close the gap. If the index passed in is out-of-range,
        nothing will happen and a null pointer will be returned.

        @param indexToRemove    the index of the element to remove
        @see remove, removeObject, removeRange
    */
    ObjectClassPtr removeAndReturn (const int indexToRemove)
    {
        ObjectClassPtr removedItem;

        if (isPositiveAndBelow (indexToRemove, numUsed))
        {
            ObjectClass** const e = data.elements + indexToRemove;

            if (ObjectClass* o = *e)
            {
                removedItem = o;
                releaseObject (o);
            }

            --numUsed;
            const int numberToShift = numUsed - indexToRemove;

            if (numberToShift > 0)
                memmove (e, e + 1, sizeof (ObjectClass*) * (size_t) numberToShift);

            if ((numUsed << 1) < data.numAllocated)
                minimiseStorageOverheads();
        }

        return removedItem;
    }

    /** Removes the first occurrence of a specified object from the array.

        If the item isn't found, no action is taken. If it is found, it is
        removed and has its reference count decreased.

        @param objectToRemove   the object to try to remove
        @see remove, removeRange
    */
    void removeObject (ObjectClass* const objectToRemove)
    {
        remove (indexOf (objectToRemove));
    }

    /** Removes a range of objects from the array.

        This will remove a set of objects, starting from the given index,
        and move any subsequent elements down to close the gap.

        If the range extends beyond the bounds of the array, it will
        be safely clipped to the size of the array.

        The objects that are removed will have their reference counts decreased,
        and may be deleted if not referenced from elsewhere.

        @param startIndex       the index of the first object to remove
        @param numberToRemove   how many objects should be removed
        @see remove, removeObject
    */
    void removeRange (const int startIndex,
                      const int numberToRemove)
    {
        const int start    = jlimit (0, numUsed, startIndex);
        const int endIndex = jlimit (0, numUsed, startIndex + numberToRemove);

        if (endIndex > start)
        {
            int i;
            for (i = start; i < endIndex; ++i)
            {
                if (ObjectClass* o = data.elements[i])
                {
                    releaseObject (o);
                    data.elements[i] = nullptr; // (in case one of the destructors accesses this array and hits a dangling pointer)
                }
            }

            const int rangeSize = endIndex - start;
            ObjectClass** e = data.elements + start;
            i = numUsed - endIndex;
            numUsed -= rangeSize;

            while (--i >= 0)
            {
                *e = e [rangeSize];
                ++e;
            }

            if ((numUsed << 1) < data.numAllocated)
                minimiseStorageOverheads();
        }
    }

    /** Removes the last n objects from the array.

        The objects that are removed will have their reference counts decreased,
        and may be deleted if not referenced from elsewhere.

        @param howManyToRemove   how many objects to remove from the end of the array
        @see remove, removeObject, removeRange
    */
    void removeLast (int howManyToRemove = 1)
    {
        if (howManyToRemove > numUsed)
            howManyToRemove = numUsed;

        while (--howManyToRemove >= 0)
            remove (numUsed - 1);
    }

    /** Swaps a pair of objects in the array.

        If either of the indexes passed in is out-of-range, nothing will happen,
        otherwise the two objects at these positions will be exchanged.
    */
    void swap (const int index1,
               const int index2) noexcept
    {
        if (isPositiveAndBelow (index1, numUsed)
             && isPositiveAndBelow (index2, numUsed))
        {
            std::swap (data.elements [index1],
                       data.elements [index2]);
        }
    }

    /** Moves one of the objects to a different position.

        This will move the object to a specified index, shuffling along
        any intervening elements as required.

        So for example, if you have the array { 0, 1, 2, 3, 4, 5 } then calling
        move (2, 4) would result in { 0, 1, 3, 4, 2, 5 }.

        @param currentIndex     the index of the object to be moved. If this isn't a
                                valid index, then nothing will be done
        @param newIndex         the index at which you'd like this object to end up. If this
                                is less than zero, it will be moved to the end of the array
    */
    void move (const int currentIndex,
               int newIndex) noexcept
    {
        if (currentIndex != newIndex)
        {
            if (isPositiveAndBelow (currentIndex, numUsed))
            {
                if (! isPositiveAndBelow (newIndex, numUsed))
                    newIndex = numUsed - 1;

                ObjectClass* const value = data.elements [currentIndex];

                if (newIndex > currentIndex)
                {
                    memmove (data.elements + currentIndex,
                             data.elements + currentIndex + 1,
                             sizeof (ObjectClass*) * (size_t) (newIndex - currentIndex));
                }
                else
                {
                    memmove (data.elements + newIndex + 1,
                             data.elements + newIndex,
                             sizeof (ObjectClass*) * (size_t) (currentIndex - newIndex));
                }

                data.elements [newIndex] = value;
            }
        }
    }

    //==============================================================================
    /** This swaps the contents of this array with those of another array.

        If you need to exchange two arrays, this is vastly quicker than using copy-by-value
        because it just swaps their internal pointers.
    */
    template <class OtherArrayType>
    void swapWith (OtherArrayType& otherArray) noexcept
    {
        data.swapWith (otherArray.data);
        std::swap (numUsed, otherArray.numUsed);
    }

    //==============================================================================
    /** Compares this array to another one.

        @returns true only if the other array contains the same objects in the same order
    */
    bool operator== (const ReferenceCountedArray& other) const noexcept
    {
        if (numUsed != other.numUsed)
            return false;

        for (int i = numUsed; --i >= 0;)
            if (data.elements [i] != other.data.elements [i])
                return false;

        return true;
    }

    /** Compares this array to another one.

        @see operator==
    */
    bool operator!= (const ReferenceCountedArray<ObjectClass>& other) const noexcept
    {
        return ! operator== (other);
    }

    //==============================================================================
    /** Sorts the elements in the array.

        This will use a comparator object to sort the elements into order. The object
        passed must have a method of the form:
        @code
        int compareElements (ElementType first, ElementType second);
        @endcode

        ..and this method must return:
          - a value of < 0 if the first comes before the second
          - a value of 0 if the two objects are equivalent
          - a value of > 0 if the second comes before the first

        To improve performance, the compareElements() method can be declared as static or const.

        @param comparator   the comparator to use for comparing elements.
        @param retainOrderOfEquivalentItems     if this is true, then items
                            which the comparator says are equivalent will be
                            kept in the order in which they currently appear
                            in the array. This is slower to perform, but may
                            be important in some cases. If it's false, a faster
                            algorithm is used, but equivalent elements may be
                            rearranged.

        @see sortArray
    */
    template <class ElementComparator>
    void sort (ElementComparator& comparator,
               const bool retainOrderOfEquivalentItems = false) const noexcept
    {
        ignoreUnused (comparator); // if you pass in an object with a static compareElements() method, this
                                   // avoids getting warning messages about the parameter being unused

        sortArray (comparator, data.elements.getData(), 0, size() - 1, retainOrderOfEquivalentItems);
    }

    //==============================================================================
    /** Reduces the amount of storage being used by the array.

        Arrays typically allocate slightly more storage than they need, and after
        removing elements, they may have quite a lot of unused space allocated.
        This method will reduce the amount of allocated storage to a minimum.
    */
    void minimiseStorageOverheads() noexcept
    {
        data.shrinkToNoMoreThan (numUsed);
    }

    /** Increases the array's internal storage to hold a minimum number of elements.

        Calling this before adding a large known number of elements means that
        the array won't have to keep dynamically resizing itself as the elements
        are added, and it'll therefore be more efficient.
    */
    void ensureStorageAllocated (const int minNumElements)
    {
        data.ensureAllocatedSize (minNumElements);
    }

private:
    //==============================================================================
    ArrayAllocationBase <ObjectClass*> data;
    int numUsed;

    void releaseAllObjects()
    {
        while (numUsed > 0)
            if (ObjectClass* o = data.elements [--numUsed])
                releaseObject (o);

        jassert (numUsed == 0);
    }

    static void releaseObject (ObjectClass* o)
    {
        if (o->decReferenceCountWithoutDeleting())
            delete o;
    }
};

}

#endif // WATER_REFERENCECOUNTEDARRAY_H_INCLUDED
