// MIT License - CCollections
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)
#pragma once
#include "CList.h"
#include <assert.h>

/*  CArrayList

    A linked list of fixed size arrays backed by dynamic heap allocation in n-array size growth
    Choose a list capacity (n-arrays) that is the expected average number of arrays to avoid additional allocation
*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// size of arrays capacity (including array headers)
static inline uint32_t carraylistSizeOfCapacity(CArrayList* list);
// size of arrays count (including array headers)
static inline uint32_t carraylistSizeOfArrays(CArrayList* list);
// size of arrays raw data (without array headers)
static inline uint32_t carraylistSizeOfData(CArrayList* list);

static inline void* carraylistBegin(CArrayList* list);
static inline void* carraylistEnd(CArrayList* list);
static inline void* carraylistLast(CArrayList* list);
static inline void* carraylistCapacityEnd(CArrayList* list);
// ! get the next array, useful for iterating
static inline CArrayHeader* carraylistNext(CArrayList* list, CArrayHeader* itr);
// ! get the raw data from array header
static inline void* carraylistData(CArrayHeader* itr);
// ! get size of item type within arrays
static inline uint32_t carraylistItemStride(CArrayList* list);

static inline CArrayList carraylistCreate(uint32_t arrayItemStride, uint32_t arrayItemCapacity, uint32_t listCapacity);
static inline void carraylistAlloc(CArrayList* list, uint32_t arrayItemStride, uint32_t arrayItemCapacity, uint32_t listCapacity);
static inline void carraylistRealloc(CArrayList* list, uint32_t newListCapacity);
static inline void carraylistFree(CArrayList* list);
// zero all raw data
static inline void carraylistZeroMem(CArrayList* list);
// grow capacity
static inline void carraylistGrow(CArrayList* list, uint32_t numNewItems);
// grow capacity and zero new raw data
static inline void carraylistGrowZero(CArrayList* list, uint32_t numNewItems);
// shrink capacity
static inline void carraylistShrink(CArrayList* list, uint32_t numLessItems);

// !
static inline uint32_t carraylistAddArray(CArrayList* list);
// !
static inline uint32_t carraylistAddArrayRange(CArrayList* list, uint32_t arraysCount);
// ! get the array header at array index
static inline CArrayHeader* carraylistArrayAt(CArrayList* list, uint32_t index);

// ! add item to array at index
static inline void carraylistAddItem(CArrayList* list, uint32_t arrayIndex, void* item);
// ! add items to array at index
static inline void carraylistAddItemRange(CArrayList* list, uint32_t arrayIndex, void* items, uint32_t itemsCount);
// get the raw data at an array index
static inline void* carraylistItemAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemIndex);


static inline void carraylistInsertItemAt(CArrayList* list, uint32_t arrayIndex, void* item, uint32_t insertItemIndex);
static inline void carraylistInsertItemRangeAt(CArrayList* list, uint32_t arrayIndex, void* items, uint32_t itemsCount, uint32_t insertItemsIndex);
static inline void carraylistZeroItemAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemIndex);
static inline void carraylistZeroItemRangeAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemsCount, uint32_t itemsIndex);
static inline uint32_t carraylistFindItemIndex(CArrayList* list, uint32_t arrayIndex, void* item);
static inline uint32_t carraylistFindItemZeroIndex(CArrayList* list, uint32_t arrayIndex);

static inline void carraylistRemoveArrayAt(CArrayList* list, uint32_t arrayIndex);
static inline void carraylistRemoveItemAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemIndex);
static inline void carraylistRemoveItemRangeAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemsIndex, uint32_t itemsCount);
static inline void carraylistRemoveItem(CArrayList* list, uint32_t arrayIndex, void* item);


inline uint32_t carraylistSizeOfCapacity(CArrayList* list)
{
    return clistSizeOfCapacity(list);
}

inline uint32_t carraylistSizeOfArrays(CArrayList* list)
{
    return clistSizeOfItems(list);
}

inline uint32_t carraylistSizeOfData(CArrayList* list)
{
    uint32_t itemStride = ((CArrayHeader*)list->Data[0])->Stride;
    uint32_t size = itemStride * list->Count;
    return size;
}

inline void* carraylistBegin(CArrayList* list)
{
    return clistBegin(list);
}

inline void* carraylistEnd(CArrayList* list)
{
    return clistEnd(list);
}

inline void* carraylistLast(CArrayList* list)
{
    return clistLast(list);
}

inline void* carraylistCapacityEnd(CArrayList* list)
{
    return clistCapacityEnd(list);
}

CArrayHeader* carraylistNext(CArrayList* list, CArrayHeader* itr)
{
    CArrayHeader* ptr = (CArrayHeader*)((uint8_t*)itr + list->Stride);
    return ptr;
}

inline void* carraylistData(CArrayHeader* itr)
{
    uint8_t* data = (uint8_t*)itr + sizeof(CArrayHeader);
    return (void*)data;
}

inline uint32_t carraylistItemStride(CArrayList* list)
{
    CArrayHeader* header = (CArrayHeader*)list->Data;
    return header->Stride;
}

inline CArrayList carraylistCreate(uint32_t arrayItemStride, uint32_t arrayItemCapacity, uint32_t listCapacity)
{
    CArrayList list;
    carraylistAlloc(&list, arrayItemStride, arrayItemCapacity, listCapacity);
    return list;
}

inline void carraylistAlloc(CArrayList* list, uint32_t arrayItemStride, uint32_t arrayItemCapacity, uint32_t listCapacity)
{
    uint32_t arraySize = arrayItemStride * arrayItemCapacity;
    uint32_t listStride = (uint32_t)sizeof(CArrayHeader) + arraySize;
    clistAlloc(list, listStride, listCapacity);

    // init headers
    CArrayHeader  ref; ref.Count = 0u; ref.Capacity = arrayItemCapacity; ref.Stride = arrayItemStride; ref.UserData = 0u;
    CArrayHeader* itr = (CArrayHeader*)clistBegin(list);
    CArrayHeader* end = (CArrayHeader*)clistCapacityEnd(list);
    while (itr != end)
    {
        *itr = ref;
        itr = carraylistNext(list, itr);
    }
}

inline void carraylistRealloc(CArrayList* list, uint32_t newListCapacity)
{
    clistRealloc(list, newListCapacity);

    // init headers
    CArrayHeader  ref = *(CArrayHeader*)clistBegin(list); ref.Count = 0u; ref.UserData = 0u;
    CArrayHeader* itr = (CArrayHeader*)clistEnd(list);
    CArrayHeader* end = (CArrayHeader*)clistCapacityEnd(list);
    while (itr != end)
    {
        *itr = ref;
        itr = carraylistNext(list, itr);
    }
}

void carraylistFree(CArrayList* list)
{
    clistFree(list);
}

inline void carraylistZeroMem(CArrayList* list)
{
    CArrayHeader* itr = (CArrayHeader*)clistEnd(list);
    uint32_t capsize = itr->Stride * itr->Capacity;
    CArrayHeader* end = (CArrayHeader*)clistCapacityEnd(list);
    while (itr != end)
    {
        memset((uint8_t*)itr + sizeof(CArrayHeader), 0, capsize);
        itr = carraylistNext(list, itr);
    }
}

inline void carraylistGrow(CArrayList* list, uint32_t numNewItems)
{
    uint32_t newcapacity = list->Capacity + numNewItems;
    carraylistRealloc(list, newcapacity);
}

inline void carraylistGrowZero(CArrayList* list, uint32_t numNewItems)
{
    CArrayHeader* itr = (CArrayHeader*)clistCapacityEnd(list);
    uint32_t capsize = itr->Stride * itr->Capacity;
    carraylistGrow(list, numNewItems);
    CArrayHeader* end = (CArrayHeader*)clistCapacityEnd(list);
    while (itr != end)
    {
        memset((uint8_t*)itr + sizeof(CArrayHeader), 0, capsize);
        itr = carraylistNext(list, itr);
    }
}

inline void carraylistShrink(CArrayList* list, uint32_t numLessItems)
{
    uint32_t newcapacity = list->Capacity - numLessItems;
    carraylistRealloc(list, newcapacity);
}

// returns new list index
uint32_t carraylistAddArray(CArrayList* list)
{
    uint32_t index = list->Count;
    clistGrow(list, 1);
    ++list->Count;
    return index;
}

// returns first new list index, increment index per list
uint32_t carraylistAddArrayRange(CArrayList* list, uint32_t arraysCount)
{
    uint32_t index = list->Count;
    clistGrow(list, arraysCount);
    list->Count += arraysCount;
    return index;
}

inline CArrayHeader* carraylistArrayAt(CArrayList* list, uint32_t index)
{
    CArrayHeader* header = (CArrayHeader*)clistItemAt(list, index);
    return header;
}

void carraylistAddItem(CArrayList* list, uint32_t arrayIndex, void* item)
{
    CArrayHeader* header = (CArrayHeader*)clistItemAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)header + (uint32_t)sizeof(CArrayHeader);
    assert(header->Count + 1 <= header->Capacity);
    uint8_t* dst = data + header->Count;
    uint32_t cpysize = header->Stride;
    memcpy(dst, item, cpysize);
    ++header->Count;
}

void carraylistAddItemRange(CArrayList* list, uint32_t arrayIndex, void* items, uint32_t itemsCount)
{
    CArrayHeader* header = (CArrayHeader*)clistItemAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)header + (uint32_t)sizeof(CArrayHeader);
    assert(header->Count + itemsCount <= header->Capacity);
    uint8_t* dst = data + header->Count;
    uint32_t cpysize = header->Stride * itemsCount;
    memcpy(dst, items, cpysize);
    header->Count += itemsCount;
}

inline void* carraylistItemAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemIndex)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)header + sizeof(CArrayHeader);
    uint32_t offset = header->Stride * itemIndex;
    return data + offset;
}

inline void carraylistInsertItemAt(CArrayList* list, uint32_t arrayIndex, void* item, uint32_t insertItemIndex)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);

    uint32_t stride = header->Stride;
    uint32_t insoff = stride * insertItemIndex;
    uint8_t* insdst = data + insoff;
    uint8_t* movdst = insdst + stride;
    uint8_t* end = data + header->Count;
    memmove(movdst, insdst, end - movdst);
    memcpy(insdst, item, stride);
    ++header->Count;
    assert(header->Count <= header->Capacity && "CArray out of bounds");
}

inline void carraylistInsertItemRangeAt(CArrayList* list, uint32_t arrayIndex, void* items, uint32_t itemsCount, uint32_t insertItemsIndex)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);

    uint32_t stride = header->Stride;
    uint32_t itemsSize = stride * itemsCount;
    uint32_t insoff = stride * insertItemsIndex;
    uint8_t* insdst = data + insoff;
    uint8_t* movdst = insdst + itemsSize;
    uint8_t* end = data + header->Count;
    memmove(movdst, insdst, end - movdst);
    memcpy(insdst, items, itemsSize);
    header->Count += itemsCount;
    assert(header->Count <= header->Capacity && "CArray out of bounds");
}

inline void carraylistZeroItemAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemIndex)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t stride = header->Stride;
    uint32_t off = stride * itemIndex;
    uint8_t* dst = data + off;
    memset(dst, 0, stride);
}

inline void carraylistZeroItemRangeAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemsCount, uint32_t itemsIndex)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t stride = header->Stride;
    uint32_t size = stride * itemsCount;
    uint32_t off = stride * itemsIndex;
    uint8_t* dst = data + off;
    memset(dst, 0, size);
}

inline uint32_t carraylistFindItemIndex(CArrayList* list, uint32_t arrayIndex, void* item)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t stride = header->Stride;
    uint32_t capsize = stride * header->Capacity;

    uint8_t* itr = data;
    uint8_t* end = data + capsize;
    for (uint32_t i = 0; itr != end; itr += stride, ++i)
    {
        if (memcmp(itr, item, stride) == 0)
        {
            return i;
        }
    }
    return CCOLLECTION_ERROR;
}

inline uint32_t carraylistFindItemZeroIndex(CArrayList* list, uint32_t arrayIndex)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t stride = header->Stride;
    uint32_t capsize = stride * header->Capacity;

    uint8_t* itr = data;
    uint8_t* end = data + capsize;
    for (uint32_t i = 0; itr != end; itr += stride, ++i)
    {
        uint8_t bytesOr = 0;
        for (uint8_t* byte = itr, *byteEnd = itr + stride; byte != byteEnd; ++byte)
        {
            bytesOr |= *byte;
        }
        if (bytesOr == 0)
        {
            return i;
        }
    }
    return CCOLLECTION_ERROR;
}

inline void carraylistRemoveArrayAt(CArrayList* list, uint32_t arrayIndex)
{
    CArrayHeader* lastheader = (CArrayHeader*)carraylistLast(list);
    CArrayHeader* remheader = (CArrayHeader*)carraylistArrayAt(list, arrayIndex);
    memcpy(remheader, lastheader, list->Stride);
    --list->Count;
}

inline void carraylistRemoveItemAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemIndex)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t stride = header->Stride;
    uint32_t capsize = stride * header->Capacity;
    uint32_t off = stride * itemIndex;
    uint8_t* dst = data + off;
    uint8_t* src = dst + stride;
    uint8_t* end = data + capsize;
    memmove(dst, src, end - src);
    --header->Count;
}

inline void carraylistRemoveItemRangeAt(CArrayList* list, uint32_t arrayIndex, uint32_t itemsIndex, uint32_t itemsCount)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t stride = header->Stride;
    uint32_t capsize = stride * header->Capacity;
    uint32_t remsize = stride * itemsCount;
    uint32_t off = stride * itemsIndex;
    uint8_t* dst = data + off;
    uint8_t* src = dst + remsize;
    uint8_t* end = data + capsize;
    memmove(dst, src, end - src);
    header->Count -= itemsCount;
}

inline void carraylistRemoveItem(CArrayList* list, uint32_t arrayIndex, void* item)
{
    CArrayHeader* header = carraylistArrayAt(list, arrayIndex);
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t stride = header->Stride;
    uint32_t capsize = stride * header->Capacity;

    uint8_t* itr = data;
    uint8_t* end = data + capsize;
    for (uint32_t i = 0; itr != end; itr += stride, ++i)
    {
        if (memcmp(itr, item, stride) == 0)
        {
            carraylistRemoveItemAt(list, arrayIndex, i);
            return;
        }
    }
}


#ifdef __cplusplus
}
#endif // __cplusplus
