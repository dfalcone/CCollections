// MIT License - CCollections
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)
#pragma once
#include "CArrayList.h"
/////////////////
// CMultiArrayList
// - list of indirect keys
// - multiple lists

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// internal use only
typedef struct CMultiArrayListKey
{
    uint32_t ListIndex;
    uint32_t SubListIndex;
} CMultiArrayListKey;



uint32_t cmultiarraylistItemStride(CMultiArrayList* c);
uint32_t cmultiarraylistFindListIndexFromItems(uint32_t itemsStride, uint32_t itemsCount);

void cmultiarraylistAlloc(CMultiArrayList* c, uint32_t itemsStride, uint32_t keyCapacity);
void cmultiarraylistFree(CMultiArrayList* c);

CMultiArrayListKey cmultiarraylistAddRange(CMultiArrayList* c, void* items, uint32_t itemsCount, uint32_t itemStride);
void cmultiarraylistAddRangeAtKey(CMultiArrayList* c, CMultiArrayListKey* key, void* items, uint32_t itemsCount);
void cmultiarraylistRemoveRangeAtKey(CMultiArrayList* c, CMultiArrayListKey* key, uint32_t itemsIndex, uint32_t itemsCount);


inline uint32_t cmultiarraylistItemStride(CMultiArrayList* c)
{
    return carraylistItemStride(c->ArrayLists);
}

//inline uint32_t cmultiarraylistFindListIndexFromItemCount(CMultiArrayList* c, uint32_t itemsCount)
//{
//    uint32_t itemsSize = itemsCount * cmultiarraylistItemStride(c);
//    CList* list = c->ArrayLists;
//    for (uint32_t i = 0; i != 16; ++i, ++list)
//    {
//        if (itemsSize <= list->Stride)
//            return i;
//    }
//    return CCOLLECTION_ERROR;
//}
//
//inline uint32_t cmultiarraylistFindListIndexFromItemsSize(CMultiArrayList* c, uint32_t itemsSize)
//{
//    CArrayList* list = c->ArrayLists;
//    for (uint32_t i = 0; i != 16; ++i, ++list)
//    {
//        if (itemsSize > list->Stride)
//            continue;
//        return i;
//    }
//    return CCOLLECTION_ERROR;
//}

inline uint32_t cmultiarraylistFindListIndexFromItems(uint32_t itemsStride, uint32_t itemsCount)
{
    uint32_t itemsSize = itemsStride * itemsCount;
    for (uint32_t i = 0; i != 16; i = i << 1)
    {
        uint32_t capSize = i * itemsStride;
        if (itemsSize < capSize)
            return i;
    }
    return CCOLLECTION_ERROR;
}

inline void cmultiarraylistAlloc(CMultiArrayList* c, uint32_t itemsStride, uint32_t keyCapacity)
{
    memset(c, 0, sizeof(CMultiArrayList));

    clistAlloc(&c->Keys, (uint32_t)sizeof(CMultiArrayListKey), keyCapacity);

    CArrayList* itr = c->ArrayLists;
    CArrayList* end = itr + 16;
    for (uint32_t i = 16; itr != end; ++itr, i = i << 1)
    {
        itr->Stride = (uint32_t)sizeof(CArrayHeader) + (itemsStride * i);
        itr->Type = CCOLLECTION_TYPE_ARRAYLIST;
    }
}

inline void cmultiarraylistFree(CMultiArrayList* c)
{
    clistFree(&c->Keys);
    CArrayList* itr = c->ArrayLists;
    CArrayList* end = itr + 16;
    for (; itr != end; ++itr)
    {
        if (itr->Data == NULL)
            continue;
        carraylistFree(itr);
    }
}

inline CMultiArrayListKey cmultiarraylistAddRange(CMultiArrayList* c, void* items, uint32_t itemsCount, uint32_t itemsStride)
{
    CMultiArrayListKey key;
    uint32_t itemsSize = itemsStride * itemsCount;
    key.ListIndex = cmultiarraylistFindListIndexFromItems(itemsStride, itemsCount);
    CArrayList* list = &c->ArrayLists[key.ListIndex];
    key.SubListIndex = list->Count;

    if (list->Count == list->Capacity)
    {
        // grow
        if (list->Capacity)
        {
            // shift to next pow 2
            uint32_t newcapacity = list->Capacity << 1u;
            carraylistRealloc(list, newcapacity);
        }
        // first alloc
        else
        {
            uint32_t arrayItemsSize = list->Stride - (uint32_t)sizeof(CArrayHeader);
            uint32_t arrayCapacity = arrayItemsSize / itemsStride;
            carraylistAlloc(list, itemsStride, arrayCapacity, 8u);
        }

        // init arrays
        CArrayHeader* itr = (CArrayHeader*)carraylistEnd(list);
        CArrayHeader* end = (CArrayHeader*)carraylistCapacityEnd(list);
        CArrayHeader ref = { 0u, 0u, itemsStride, 0u};
        for ( ; itr != end; ++itr)
        {
            *itr = ref;
        }
    }

    // copy
    CArrayHeader* header = (CArrayHeader*)carraylistEnd(list);
    header->Count = itemsCount;
    header->Capacity = (list->Stride - (uint32_t)sizeof(CArrayHeader)) / itemsStride;
    header->Stride = itemsStride;
    header->UserData = 0u;
    uint8_t* dst = (uint8_t*)header + sizeof(CArrayHeader);
    memcpy(dst, items, itemsSize);

    return key;
}

void cmultiarraylistAddRangeAtKey(CMultiArrayList* c, CMultiArrayListKey* key, void* items, uint32_t itemsCount)
{
    CArrayList* list = &c->ArrayLists[key->ListIndex];
    CArrayHeader* header = (CArrayHeader*)carraylistArrayAt(list, key->SubListIndex);

    uint32_t totalItemsCount = header->Count + itemsCount;
    if (totalItemsCount > header->Capacity)
    {
        uint32_t oldListIndex = key->ListIndex;
        uint32_t oldArrayIndex = key->SubListIndex;

        key->ListIndex = cmultiarraylistFindListIndexFromItems(header->Stride, totalItemsCount);
        CArrayList* movlist = &c->ArrayLists[key->ListIndex];
        key->SubListIndex = carraylistAddArray(movlist);
        
        // move data to bigger list
        CArrayHeader* movheader = (CArrayHeader*)carraylistArrayAt(movlist, key->SubListIndex);
        uint32_t cpysize = header->Stride * header->Count;
        memcpy(carraylistData(movheader), carraylistData(header), cpysize);

        // update key with last array index to remove array index
        uint32_t lastArrayIndex = (uint32_t)((uint8_t*)carraylistLast(list) - (uint8_t*)carraylistBegin(list));
        CMultiArrayListKey lastKeyVal = { oldListIndex, lastArrayIndex };
        CMultiArrayListKey* lastKeyPtr = (CMultiArrayListKey*)clistItemAt(&c->Keys, clistFindIndex(&c->Keys, &lastKeyVal));
        lastKeyPtr->SubListIndex = oldArrayIndex;
        carraylistRemoveArrayAt(list, oldArrayIndex);

        list = movlist;
        header = movheader;
    }

    // copy
    uint8_t* data = (uint8_t*)carraylistData(header);
    uint32_t dataSize = header->Stride * header->Count;
    uint8_t* dst = data + dataSize;
    uint32_t cpysize = header->Stride * itemsCount;
    memcpy(dst, items, cpysize);
    header->Count += itemsCount;
}

inline void cmultiarraylistRemoveRangeAtKey(CMultiArrayList* c, CMultiArrayListKey* key, uint32_t itemsIndex, uint32_t itemsCount)
{
    CArrayList* list = &c->ArrayLists[key->ListIndex];
    carraylistRemoveItemRangeAt(list, key->SubListIndex, itemsIndex, itemsCount);
}

#ifdef __cplusplus
}
#endif // __cplusplus
