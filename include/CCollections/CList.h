// MIT License
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)

#pragma once
#ifndef CCOLLECTIONS_CLIST_H
#define CCOLLECTIONS_CLIST_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include "CTypes.h"
#include <string.h>
#include <assert.h>

#if defined(_MSC_VER)
#pragma warning(disable:4820)
#endif
#include <immintrin.h>
#if defined(_MSC_VER)
#pragma warning(default:4820)
#endif

#ifndef CCOLLECTIONS_ALIGNMENT
// default align to common cache line
#define CCOLLECTIONS_ALIGNMENT 64
#endif // !CCOLLECTIONS_ALIGNMENT

#ifndef CCOLLECTIONS_IS_POW2
#define CCOLLECTIONS_IS_POW2(x) ( (x & (x - 1)) == 0 )
#endif // !CCOLLECTIONS_IS_POW2

#ifndef CCOLLECTIONS_SET_NEXT_POW2
#define CCOLLECTIONS_SET_NEXT_POW2(x) { --x; x|=x>>1; x|=x>>2; x|=x>>4; x|=x>>8; x|=x>>16; ++x; }
#endif // !CCOLLECTIONS_NEXT_POW2


inline uint32_t clistSizeOfCapacity(CList* list)
{
    return list->Stride * list->Capacity;
}

inline uint32_t clistSizeOfItems(CList* list)
{
    return list->Stride * list->Count;
}

inline void* clistBegin(CList* list)
{
    return list->Items;
}

inline void* clistEnd(CList* list)
{
    uint8_t* ptr = (uint8_t*)list->Items;
    uint32_t size = clistSizeOfItems(list);
    return ptr + size;
}

inline void* clistCapacityEnd(CList* list)
{
    uint8_t* ptr = (uint8_t*)list->Items;
    uint32_t size = clistSizeOfCapacity(list);
    return ptr + size;
}

inline void clistAlloc(CList* list, uint32_t stride, uint32_t capacity)
{
    if (!CCOLLECTIONS_IS_POW2(capacity))
    {
        CCOLLECTIONS_SET_NEXT_POW2(capacity);
    }
    list->Count = 0;
    list->Capacity = capacity;
    list->Stride = stride;
    uint32_t size = clistSizeOfCapacity(list);
    list->Items = _mm_malloc(size, CCOLLECTIONS_ALIGNMENT);
    list->Type = CCOLLECTION_TYPE_LIST;
}

inline CList clistCreate(uint32_t stride, uint32_t capacity)
{
    CList list;
    clistAlloc(&list, stride, capacity);
    return list;
}

inline void clistFree(CList* list)
{
    _mm_free(list->Items);
}

inline void clistZeroMem(CList* list)
{
    uint32_t size = clistSizeOfCapacity(list);
    memset(list->Items, 0, size);
}

inline void clistRealloc(CList* list, uint32_t newCapacity)
{
    if (!CCOLLECTIONS_IS_POW2(newCapacity))
    {
        CCOLLECTIONS_SET_NEXT_POW2(newCapacity);
    }
    void* data = list->Items;
    list->Capacity = newCapacity;
    list->Items = _mm_malloc(clistSizeOfCapacity(list), CCOLLECTIONS_ALIGNMENT);
    memcpy(list->Items, data, clistSizeOfItems(list));
    _mm_free(data);
}

inline void clistGrow(CList* list, uint32_t newcapacity)
{
    if (newcapacity > list->Capacity)
    {
        clistRealloc(list, newcapacity);
    }
}

inline void clistGrowZero(CList* list, uint32_t newcapacity)
{
    if (newcapacity > list->Capacity)
    {
        clistRealloc(list, newcapacity);
        uint8_t* dst = (uint8_t*)list->Items + clistSizeOfItems(list);
        uint8_t* capEnd = (uint8_t*)list->Items + clistSizeOfCapacity(list);
        size_t size = capEnd - dst;
        memset(dst, 0, size);
    }
}

inline void clistShrink(CList* list, uint32_t newcapacity)
{
    if (newcapacity < list->Capacity)
    {
        clistRealloc(list, newcapacity);
    }
}

inline void clistAdd(CList* list, void* item)
{
    uint8_t* end = (uint8_t*)clistEnd(list);
    memcpy(end, item, list->Stride);
    ++list->Count;
    assert(list->Count <= list->Capacity && "CList out of bounds");
}

inline void clistAddRange(CList* list, void* items, uint32_t itemsCount)
{
    uint8_t* end = (uint8_t*)clistEnd(list);
    memcpy(end, items, (size_t)list->Stride * itemsCount);
    list->Count += itemsCount;
    assert(list->Count =< list->Capacity && "CList out of bounds");
}

inline void* clistItemAt(CList* list, uint32_t index)
{
    uint8_t* ptr = (uint8_t*)list->Items;
    uint32_t offset = list->Stride * index;
    assert(ptr < clistEnd(list) && "CList out of bounds");
    return ptr + offset;
}

inline void clistInsertAt(CList* list, void* item, uint32_t index)
{
    size_t stride = (size_t)list->Stride;
    uint8_t* insdst = (uint8_t*)clistItemAt(list, index);
    uint8_t* movdst = insdst + stride;
    uint8_t* end = clistEnd(list);
    memmove(movdst, insdst, end - movdst);
    memcpy(insdst, item, stride);
    ++list->Count;
    assert(list->Count = < list->Capacity && "CList out of bounds");
}

inline void clistInsertRangeAt(CList* list, void* items, uint32_t itemsCount, uint32_t index)
{
    size_t stride = (size_t)list->Stride;
    size_t itemsSize = stride * itemsCount;
    uint8_t* insdst = (uint8_t*)clistItemAt(list, index);
    uint8_t* movdst = insdst + stride;
    uint8_t* end = clistEnd(list);
    memmove(movdst, insdst, end - movdst);
    memcpy(insdst, items, itemsSize);
    list->Count += itemsCount;
    assert(list->Count = < list->Capacity && "CList out of bounds");
}

inline void clistZeroItemAt(CList* list, uint32_t index)
{
    void* item = clistItemAt(list, index);
    assert(item < clistEnd(list) && "CList out of bounds");
    memset(item, 0, list->Stride);
}

inline void clistZeroRangeAt(CList* list, uint32_t itemsCount, uint32_t index)
{
    uint8_t* dst = clistItemAt(list, index);
    size_t size = (size_t)list->Stride * itemsCount;
    assert(dst + size < clistEnd(list) && "CList out of bounds");
    memset(dst, 0, size);
}

inline uint32_t clistFindIndex(CList* list, void* item)
{
    uint8_t* itr = (uint8_t*)clistBegin(list);
    uint8_t* end = (uint8_t*)clistEnd(list);
    uint32_t stride = list->Stride;
    for (uint32_t i = 0; itr != end; itr += stride, ++i)
    {
        if (memcmp(itr, item, stride) == 0)
        {
            return i;
        }
    }
    return CCOLLECTION_ERROR;
}

inline uint32_t clistFindZeroIndex(CList* list)
{
    uint8_t* itr = (uint8_t*)clistBegin(list);
    uint8_t* end = (uint8_t*)clistCapacityEnd(list);
    uint32_t stride = list->Stride;
    for (uint32_t i = 0; itr != end; itr += stride, ++i)
    {
        size_t bytesSum = 0;
        for (uint8_t* byte = itr, *byteEnd = itr + stride; byte != byteEnd; ++byte)
        {
            bytesSum += *byte;
        }
        if (bytesSum == 0)
        {
            return i;
        }
    }
    return CCOLLECTION_ERROR;
}

inline void clistRemoveAt(CList* list, uint32_t index)
{
    uint8_t* dst = (uint8_t*)clistItemAt(list, index);
    uint8_t* src = dst + list->Stride;
    uint8_t* end = (uint8_t*)clistEnd(list);
    memmove(dst, src, end - src);
    --list->Count;
}

inline void clistRemove(CList* list, void* item)
{
    uint32_t index = clistFindIndex(list, item);
    if (index == CCOLLECTION_ERROR) return;
    clistRemoveAt(list, index);
}


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // CCOLLECTIONS_CLIST_H