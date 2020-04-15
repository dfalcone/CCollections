// Copyright 2020, Dante Falcone. MIT License.
#pragma once
#ifndef CLIST_H
#define CLIST_H

#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <immintrin.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


    //typedef struct CList_T
    //{
    //    uint8_t* Data;
    //    uint8_t* DataEnd;
    //
    //    uint32_t* Capacity;
    //    uint32_t  ItemSize;
    //
    //    uint32_t Alignment;
    //
    //} CList;

    //typedef struct CContainer_T
    //{
    //    uint32_t Count;
    //    uint32_t Capacity;
    //    uint8_t* Data;
    //} CList;

    //struct mything1 { int a, b; };
    //struct mything2 { float a, b, c, d; };

    //struct thing1list
    //{
    //    uint32_t Count;
    //    uint32_t Capacity;
    //    struct mything1* Data;
    //};

    //struct thing1list
    //{
    //    uint32_t Count;
    //    uint32_t Capacity;
    //    struct mything2* Data;
    //};
#ifndef CLIST_ALIGNMENT
#define CLIST_ALIGNMENT 16
#endif // !CLIST_ALIGNMENT

#ifndef IS_POW2
#define IS_POW2(x) ( (x & (x - 1)) == 0 )
#endif // !CX_IS_POW2

#ifndef SET_NEXT_POW2
#define SET_NEXT_POW2(x) { --x; x|x >>1; x|x>>2; x|x>>4; x|x>>8; x|x>>16; ++x; }
#endif // !SET_NEXT_POW2



#define CLIST_DECLARE(T)        \
    typedef struct CList_##T##_t\
    {                           \
        T* Data;                \
        uint32_t Count;         \
        uint32_t Capacity;      \
    } CList_##T             

#define CLIST_FORWARD_DECLARE(T) typedef struct CList_##T##_t CList_ ##T
#define CList(T) CList_##T

#define clistGetCount(pList)    (pList)->Count
#define clistGetCapacity(pList) (pList)->Capacity 
#define clistGetData(pList)     (pList)->Data

#define clistSetCapacity(pList, capacity) {     \
    uint32_t newCap = capacity;                 \
    uint32_t count  = (pList)->Count;           \
    uint32_t oldCap = (pList)->Capacity;        \
    void* oldData   = (pList)->Data;            \
                                                \
    if (newCap > oldCap) {                      \
        if (!IS_POW2(newCap))                   \
            SET_NEXT_POW2(newCap);              \
                                                \
        uint32_t itemSize = sizeof(*(pList));   \
        uint32_t dataSize = count * itemSize;   \
        uint32_t capSize = newCap * itemSize;   \
                                                \
        void* newData = _mm_malloc(capSize, CLIST_ALIGNMENT);\
        assert(newData != NULL);                \
                                                \
        if (oldData) {                          \
            memcpy(newData, oldData, dataSize); \
            _mm_free(oldData);                  \
        }                                       \
                                                \
        (pList)->Capacity = newCap;             \
        (pList)->Data = newData;                \
    }                                           \
}

#define clistSetCount(pList, count) { clistSetCapacity(pList, count); (pList)->Count = count; }


#define clistNew(pList, capacity) { memset((pList), 0, sizeof(*(pList))); clistReserve(pList, capacity); }
#define clistDelete(pList) if((pList)->Data) _mm_free((pList)->Data)
#define clistClear(pList) (pList)->Count = 0

#define clistAdd(pList, item) {             \
    uint32_t count = clistGetCount(pList);  \
    clistSetCapacity(pList, count);         \
    memcpy(clistGetData(pList) + count, item, sizeof(*(pList))); ++clistGetData(pList); \
}

#define clistRemoveAt(pList, index) {               \
    uint32_t count = (pList)->Count;                \
    assert(index < (pList)->Count);                 \
    uint32_t moveCount = count - index;             \
    size_t moveSize = moveCount * sizeof(*(pList)); \
    auto removeData = (pList)->Data + index;        \
    auto removeDataEnd = removeData + 1;            \
    memmove(removeData, removeDataEnd, moveSize);   \
    --(pList)->Count;                               \
}

#define clistRemove(pList, pItem) {                     \
    auto data = (pList)->Data;                          \
    auto dataEnd = (pList)->Data + (pList)->Count;      \
                                                        \
    uint32_t i = 0;                                     \
    while (data != dataEnd) {                           \
        if (memcmp(pList, pItem, sizeof(*(pList))) == 0)\
            break;                                      \
        ++data;                                         \
        ++i;                                            \
    }                                                   \
                                                        \
    clistRemoveAt(pList, i);                            \
}

    typedef struct CContainer
    {
        uint8_t* Data;
        uint32_t Count;
        uint32_t Capacity;

    } CContainer;


void clistSetCapacity_(void* pList, uint32_t capacity, size_t itemSize)
{
    CContainer c = *(CContainer*)pList;
    uint32_t newCap = capacity;                                                
    if (newCap > c.Capacity) {
        if (!IS_POW2(newCap)) {
            SET_NEXT_POW2(newCap);
        }

        uint32_t dataSize = c.Count * itemSize;   
        uint32_t capSize = newCap * itemSize;   

        void* oldData = c.Data;            
        void* newData = _mm_malloc(capSize, CLIST_ALIGNMENT);
        assert(newData != NULL);
        
        if (oldData) {
            memcpy(newData, oldData, dataSize);
            _mm_free(oldData);
        }

        c.Capacity = newCap;
        c.Data = newData;
    }
}


    //void cxListRemove(CList* pList, uint32_t index)
    //{
    //    CList list = *pList;

    //    uint8_t* dataRemove = list.Data + (list.ItemSize * index);
    //    uint8_t* dataRemoveEnd = dataRemove + list.ItemSize;
    //    uint32_t moveCount = list.DataEnd - dataRemoveEnd;
    //    memcpy(dataRemove, dataRemoveEnd, moveCount * list.ItemSize);

    //    list.DataEnd -= list.ItemSize;

    //    *pList = list;
    //}


    //CList clistCreate(size_t itemSize, uint32_t capacity, uint32_t alignment)
    //{
    //    CList list = { 0 };
    //    list.Capacity = capacity;
    //    list.Count;

    //}

    //void clistSetCapacity(CList* pList, size_t itemSize, uint32_t capacity)
    //{
    //    CList list = *pList;

    //    uint32_t oldCapacity = list.Capacity;
    //    if (oldCapacity >= capacity)
    //        return;

    //    uint32_t newCapacity = capacity;
    //    uint8_t isNotPow2 = (newCapacity & (newCapacity - 1)) != 0;
    //    if (!CX_IS_POW2(newCapacity))
    //        newCapacity = CX_NEXT_POW2(newCapacity);
    //    list.Capacity = newCapacity;

    //    uint8_t* pDataOld = list.Data;
    //    uint32_t dataSize = list.Count * itemSize;
    //    list.Data = (uint8_t*)_mm_malloc(newCapacity * itemSize, CLIST_ALIGNMENT);
    //    assert(list.Data != NULL);
    //    memcpy(list.Data, pDataOld, dataSize);
    //    if (pDataOld) _mm_free(pDataOld);

    //    *pList = list;
    //}

    //CList cxListCreate(size_t itemSize, uint32_t capacity, uint32_t alignment)
    //{
    //    CList list = { 0 };
    //    if (itemSize > UINT32_MAX) { assert(0 && "cxListCreate::Item size must be smaller than 4GB"); return list; }
    //    list.ItemSize = itemSize;
    //    list.Alignment = alignment;
    //    return list;
    //}

    //void cxListSetCapacity(CList* pList, uint32_t capacity)
    //{
    //    CList list = *pList;

    //    uint32_t oldCapacity = list.Capacity;
    //    if (oldCapacity >= capacity)
    //        return;

    //    uint32_t newCapacity = capacity;
    //    uint8_t isNotPow2 = (newCapacity & (newCapacity - 1)) != 0;
    //    if (!CX_IS_POW2(newCapacity))
    //        newCapacity = CX_NEXT_POW2(newCapacity);
    //    list.Capacity = newCapacity;

    //    uint8_t* pDataOld = list.Data;
    //    uint32_t dataSize = list.DataEnd - list.Data;
    //    list.Data = (uint8_t*)(_mm_malloc(newCapacity * list.ItemSize, list.Alignment));
    //    assert(list.Data != NULL);
    //    memcpy(list.Data, pDataOld, dataSize);
    //    _mm_free(pDataOld);

    //    list.DataEnd = list.Data + dataSize;

    //    *pList = list;
    //}

    //void cxListSetCount(CList* pList, uint32_t count)
    //{
    //    cxListSetCapacity(pList, count);
    //    pList->DataEnd = pList->Data + (count * pList->ItemSize);
    //}

    //uint32_t cxListGetCount(CList* pList)
    //{
    //    return (pList->DataEnd - pList->Data);
    //}

    //void cxListAdd(CList* pList, void* item)
    //{
    //    cxListSetCapacity(pList, cxListGetCount(pList) + 1);
    //    memcpy(pList->DataEnd, item, pList->ItemSize);
    //    pList->DataEnd += pList->ItemSize;
    //}

    //void cxListRemove(CList* pList, uint32_t index)
    //{
    //    CList list = *pList;

    //    uint8_t* dataRemove = list.Data + (list.ItemSize * index);
    //    uint8_t* dataRemoveEnd = dataRemove + list.ItemSize;
    //    uint32_t moveCount = list.DataEnd - dataRemoveEnd;
    //    memcpy(dataRemove, dataRemoveEnd, moveCount * list.ItemSize);

    //    list.DataEnd -= list.ItemSize;

    //    *pList = list;
    //}

    //void cxListRemoveAt(uint32_t index) {}
    //void cxListRemoveRange(uint32_t index, uint32_t count) {}
    //void cxListAddRange(void* pItems, uint32_t count) {}
    //void cxListClear() {}
    //bool cxListContains(void* item) {}
    //uint32_t cxListIndexOf(void* item) {}
    //void cxListInsert(uint32_t index, void* item) {}
    //void cxListInsertRange(uint32_t index, void* pItems, uint32_t count) {}
    //void cxListReverse();
    //void cxListReverse(uint32_t index, uint32_t count) {}

    // SORT???


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !CLIST_H
