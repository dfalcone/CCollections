// MIT License - CCollections
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)
#pragma once
#include "CTypes.h"
#include <string.h>
#include <malloc.h>
#include <assert.h>

//#if defined(_MSC_VER)
//#pragma warning(disable:4820)
//#endif
//#include <immintrin.h>
//#if defined(_MSC_VER)
//#pragma warning(default:4820)
//#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static inline uint32_t clistSizeOfCapacity(CList* list);
static inline uint32_t clistSizeOfItems(CList* list);
static inline void* clistBegin(CList* list);
static inline void* clistBegin(CList* list);
static inline void* clistEnd(CList* list);
static inline void* clistLast(CList* list);
static inline void* clistCapacityEnd(CList* list);
static inline void clistAlloc(CList* list, uint32_t stride, uint32_t capacity);
static inline CList clistCreate(uint32_t stride, uint32_t capacity);
static inline void clistFree(CList* list);
static inline void clistZeroMem(CList* list);
static inline void clistRealloc(CList* list, uint32_t newCapacity);
static inline void clistGrow(CList* list, uint32_t numNewItems);
static inline void clistGrowZero(CList* list, uint32_t numNewItems);
static inline void clistShrink(CList* list, uint32_t numLessItems);
static inline void clistAdd(CList* list, void* item);
static inline void clistAddRange(CList* list, void* items, uint32_t itemsCount);
static inline void* clistItemAt(CList* list, uint32_t index);
static inline void clistInsertAt(CList* list, void* item, uint32_t index);
static inline void clistInsertRangeAt(CList* list, void* items, uint32_t itemsCount, uint32_t index);
static inline void clistZeroItemAt(CList* list, uint32_t index);
static inline void clistZeroRangeAt(CList* list, uint32_t itemsCount, uint32_t index);
static inline uint32_t clistFindIndex(CList* list, void* item);
static inline uint32_t clistFindZeroIndex(CList* list);
static inline void clistRemoveAt(CList* list, uint32_t index);
static inline void clistRemove(CList* list, void* item);




uint32_t clistSizeOfCapacity(CList* list)
{
    return list->Stride * list->Capacity;
}

uint32_t clistSizeOfItems(CList* list)
{
    return list->Stride * list->Count;
}

void* clistBegin(CList* list)
{
    return list->Data;
}

void* clistEnd(CList* list)
{
    uint8_t* ptr = (uint8_t*)list->Data;
    uint32_t size = clistSizeOfItems(list);
    return ptr + size;
}

void* clistLast(CList* list)
{
    uint8_t* ptr = (uint8_t*)clistEnd(list);
    return ptr - list->Stride;
}

void* clistCapacityEnd(CList* list)
{
    uint8_t* ptr = (uint8_t*)list->Data;
    uint32_t size = clistSizeOfCapacity(list);
    return ptr + size;
}

void clistAlloc(CList* list, uint32_t stride, uint32_t capacity)
{
    if (!CCOLLECTIONS_IS_POW2(capacity))
    {
        CCOLLECTIONS_SET_NEXT_POW2(capacity);
    }
    list->Count = 0;
    list->Capacity = capacity;
    list->Stride = stride;
    uint32_t size = clistSizeOfCapacity(list);
    list->Data = (uint8_t*)_mm_malloc(size, CCOLLECTIONS_ALIGNMENT);
    list->Type = CCOLLECTION_TYPE_LIST;
}

CList clistCreate(uint32_t stride, uint32_t capacity)
{
    CList list;
    clistAlloc(&list, stride, capacity);
    return list;
}

void clistFree(CList* list)
{
    _mm_free(list->Data);
}

void clistZeroMem(CList* list)
{
    uint32_t size = clistSizeOfCapacity(list);
    memset(list->Data, 0, size);
}

void clistRealloc(CList* list, uint32_t newCapacity)
{
    if (!CCOLLECTIONS_IS_POW2(newCapacity))
    {
        CCOLLECTIONS_SET_NEXT_POW2(newCapacity);
    }
    void* data = list->Data;
    list->Capacity = newCapacity;
    list->Data = (uint8_t*)_mm_malloc(clistSizeOfCapacity(list), CCOLLECTIONS_ALIGNMENT);
    memcpy(list->Data, data, clistSizeOfItems(list));
    _mm_free(data);
}

void clistGrow(CList* list, uint32_t numNewItems)
{
    uint32_t newcapacity = list->Capacity + numNewItems;
    clistRealloc(list, newcapacity);
}

void clistGrowZero(CList* list, uint32_t numNewItems)
{
    clistGrow(list, numNewItems);
    uint8_t* dst = (uint8_t*)list->Data + clistSizeOfItems(list);
    uint8_t* capEnd = (uint8_t*)list->Data + clistSizeOfCapacity(list);
    memset(dst, 0, capEnd - dst);
}

void clistShrink(CList* list, uint32_t numLessItems)
{
    uint32_t newcapacity = list->Capacity - numLessItems;
    clistRealloc(list, newcapacity);
}

void clistAdd(CList* list, void* item)
{
    uint8_t* end = (uint8_t*)clistEnd(list);
    memcpy(end, item, list->Stride);
    ++list->Count;
    assert(list->Count <= list->Capacity && "CList out of bounds");
}

void clistAddRange(CList* list, void* items, uint32_t itemsCount)
{
    uint8_t* end = (uint8_t*)clistEnd(list);
    memcpy(end, items, (size_t)list->Stride * itemsCount);
    list->Count += itemsCount;
    assert(list->Count <= list->Capacity && "CList out of bounds");
}

void* clistItemAt(CList* list, uint32_t index)
{
    uint8_t* ptr = (uint8_t*)list->Data;
    uint32_t offset = list->Stride * index;
    assert(ptr < clistEnd(list) && "CList out of bounds");
    return ptr + offset;
}

void clistInsertAt(CList* list, void* item, uint32_t index)
{
    size_t stride = (size_t)list->Stride;
    uint8_t* insdst = (uint8_t*)clistItemAt(list, index);
    uint8_t* movdst = insdst + stride;
    uint8_t* end = (uint8_t*)clistEnd(list);
    memmove(movdst, insdst, end - movdst);
    memcpy(insdst, item, stride);
    ++list->Count;
    assert(list->Count <= list->Capacity && "CList out of bounds");
}

void clistInsertRangeAt(CList* list, void* items, uint32_t itemsCount, uint32_t index)
{
    size_t stride = (size_t)list->Stride;
    size_t itemsSize = stride * itemsCount;
    uint8_t* insdst = (uint8_t*)clistItemAt(list, index);
    uint8_t* movdst = insdst + itemsSize;
    uint8_t* end = (uint8_t*)clistEnd(list);
    memmove(movdst, insdst, end - movdst);
    memcpy(insdst, items, itemsSize);
    list->Count += itemsCount;
    assert(list->Count <= list->Capacity && "CList out of bounds");
}

void clistZeroItemAt(CList* list, uint32_t index)
{
    void* item = clistItemAt(list, index);
    assert(item < clistEnd(list) && "CList out of bounds");
    memset(item, 0, list->Stride);
}

void clistZeroRangeAt(CList* list, uint32_t itemsCount, uint32_t index)
{
    uint8_t* dst = (uint8_t*)clistItemAt(list, index);
    size_t size = (size_t)list->Stride * itemsCount;
    assert(dst + size < clistEnd(list) && "CList out of bounds");
    memset(dst, 0, size);
}

uint32_t clistFindIndex(CList* list, void* item)
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

uint32_t clistFindZeroIndex(CList* list)
{
    uint8_t* itr = (uint8_t*)clistBegin(list);
    uint8_t* end = (uint8_t*)clistCapacityEnd(list);
    uint32_t stride = list->Stride;
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

void clistRemoveAt(CList* list, uint32_t index)
{
    uint8_t* dst = (uint8_t*)clistItemAt(list, index);
    uint8_t* src = dst + list->Stride;
    uint8_t* end = (uint8_t*)clistEnd(list);
    memmove(dst, src, end - src);
    --list->Count;
}

void clistRemoveRangeAt(CList* list, uint32_t index, uint32_t count)
{
    uint32_t remsize = list->Stride * count;
    uint8_t* dst = (uint8_t*)clistItemAt(list, index);
    uint8_t* src = dst + remsize;
    uint8_t* end = (uint8_t*)clistEnd(list);
    memmove(dst, src, end - src);
    list->Count -= count;
}

void clistRemove(CList* list, void* item)
{
    uint32_t index = clistFindIndex(list, item);
    if (index == CCOLLECTION_ERROR) return;
    clistRemoveAt(list, index);
}

#ifdef __cplusplus
}
#endif // __cplusplus
