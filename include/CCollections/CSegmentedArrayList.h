// MIT License - CCollections
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)
#pragma once
#include "CArrayList.h"
#include <assert.h>

/*  CSegmentedArrayList

    A linked list of fixed size arrays backed by dynamic heap allocation in n-array size growth
    Arrays are optionally segmented, meaning that the item count can exceed the array capacity and link to additional arrays for overflow
    This is a good general container type when the number of items per container are unknown and there are potentially a lot of containers ( > 1000 ) 
    Cache effeciency is acheived through a balance of per-array item capacity and average array item count
    ---- [x][x][x][x] > [x][x][x][ ] - [x][x][x][ ]   // ex. good use: first array segmented (linked) taking 2 fixed array nodes - near full capacity good cache efficiency
    ---- [x][x][ ][ ] - [x][ ][ ][ ] - [x][ ][ ][ ]   // ex. bad use: bad cache coherency, large gaps in data cause memory jumps when iterating
    Choose a list capacity (n-arrays) that is the expected average number of arrays to avoid additional allocations

*/

// array of items with linked-list node properties next/prev
typedef struct CBlockContainer
{
    //! offset of this block within the page, data is immediately after this container
    uint32_t Offset;

    //! number of items in the container
    uint32_t Count;
    //! maximum number of items in the container
    uint32_t Capacity;
    //! size in bytes of each item
    uint32_t Stride;

    CBlockContainer* Next;
    // without Prev, freeing a block is O(n) of block count of the data
    //CBlockContainer* Prev; // would allow freeing block as O(1), but costs extra 16 bytes for alignment not good

    // data immediately following, 16 byte aligned
} CBlockContainer;

// array of blocks with linked-list node properties next/prev
typedef struct CPageContainer
{
    //! number of blocks
    uint32_t Count;
    //! maximum number of blocks
    uint32_t Capacity;
    //! size in bytes of each block
    uint32_t Stride;
    uint32_t Unused;
    
    CPageContainer* Next;

    // data immediately following, 16 byte aligned
} CPageContainer;

// linked list of pages
// in this scheme pointers never become invalidated because memory is only added, never reallocated/moved
// a key list of indirection would need to contain the pointers for O(2) access, if referring object stored pointer directly it would be O(1) - therefore no overhead of key list needed
// usage: create a cache container, individual objects should store a pointer to a block which gives access to all items in and linked to that block
typedef struct CCacheContainer
{
    ////! size in bytes of each page
    //uint32_t PageStride;
    //uint32_t BlockStride;
    //uint32_t ItemStride;

    CPageContainer* First;
    CPageContainer* Last;
} CCacheContainer;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


static inline void ccacheAlloc(CCacheContainer* c, uint32_t itemStride, uint32_t itemCapacity, uint32_t blockCapacity);
static inline void ccacheFree(CCacheContainer* c);
// add page to cache container (internal use)
static inline CPageContainer* ccacheAddPage(CCacheContainer* c);
// ! add block - store this pointer as your container
static inline CBlockContainer* ccacheAddBlock(CCacheContainer* c);
// ! add item to block
static inline void ccacheAddItem(CCacheContainer* c, CBlockContainer* b, void* item);
// ! add item range to block
static inline void ccacheAddItemRange(CCacheContainer* c, CBlockContainer* b, void* items, uint32_t itemsCount);

inline void ccacheAlloc(CCacheContainer* c, uint32_t itemStride, uint32_t itemCapacity, uint32_t blockCapacity)
{
    uint32_t blockStride = itemStride * itemCapacity + (uint32_t)sizeof(CBlockContainer);
    uint32_t pageStride = blockStride * blockCapacity + (uint32_t)sizeof(CPageContainer);

    CPageContainer* page = (CPageContainer*)_mm_malloc(pageStride, CCOLLECTIONS_ALIGNMENT);
    page->Count = 0;
    page->Capacity = blockCapacity;
    page->Stride = blockStride;
    page->Next = NULL;

    CBlockContainer* block = (CBlockContainer*)((uint8_t*)page + sizeof(CPageContainer));
    for (uint32_t i = 0; i != blockCapacity; ++i)
    {
        block->Count = 0;
        block->Capacity = itemCapacity;
        block->Stride = itemStride;
        block->Offset = (uint32_t)((uint8_t*)block - (uint8_t*)page);
        block->Next = NULL;

        block = (CBlockContainer*)((uint8_t*)block + blockStride);
    }

    c->First = page;
    c->Last = page;
}

inline void ccacheFree(CCacheContainer* c)
{
    CPageContainer* page = c->First;
    CPageContainer* next = page->Next;
    do
    {
        _mm_free(page);
        page = next;
        next = page->Next;
    } while (next);
}

inline CPageContainer* ccacheAddPage(CCacheContainer* c)
{
    CPageContainer* pageLastOld = c->Last;
    uint32_t blockCapacity = pageLastOld->Capacity;
    uint32_t blockStride = pageLastOld->Stride;
    uint32_t pageStride = pageLastOld->Stride * pageLastOld->Capacity + (uint32_t)sizeof(CPageContainer);

    CPageContainer* pageLastNew = (CPageContainer*)_mm_malloc(pageStride, CCOLLECTIONS_ALIGNMENT);
    pageLastNew->Count = 0;
    pageLastNew->Capacity = blockCapacity;
    pageLastNew->Stride = blockStride;
    pageLastNew->Next = NULL;

    CBlockContainer* blockRef = (CBlockContainer*)((uint8_t*)pageLastOld + sizeof(CPageContainer));
    uint32_t itemCapacity = blockRef->Capacity;
    uint32_t itemStride = blockRef->Stride;

    CBlockContainer* block = (CBlockContainer*)((uint8_t*)pageLastNew + sizeof(CPageContainer));
    for (uint32_t i = 0; i != blockCapacity; ++i)
    {
        block->Count = 0;
        block->Capacity = itemCapacity;
        block->Stride = itemStride;
        block->Offset = (uint32_t)((uint8_t*)block - (uint8_t*)pageLastNew);
        block->Next = NULL;

        block = (CBlockContainer*)((uint8_t*)block + blockStride);
    }

    pageLastOld->Next = pageLastNew;
    c->Last = pageLastNew;
    return pageLastNew;
}

inline CBlockContainer* ccacheAddBlock(CCacheContainer* c)
{
    CPageContainer* page = c->Last;
    if (page->Count == page->Capacity)
    {
        page = ccacheAddPage(c);
    }
    
    uint32_t blockOffset = page->Stride * page->Count;
    CBlockContainer* block = (CBlockContainer*)((uint8_t*)page + sizeof(CPageContainer) + blockOffset);

    ++page->Count;
    return block;
}

inline void ccacheAddItem(CCacheContainer* c, CBlockContainer* b, void* item)
{
    while (b->Next) b = b->Next; // seek last
    if (b->Count == b->Capacity)
    {
        CBlockContainer* bNew = ccacheAddBlock(c);
        b->Next = bNew; // link to next block
        b = b->Next;
    }

    uint32_t itemOffset = b->Stride * b->Count;
    uint8_t* dst = (uint8_t*)b + sizeof(CBlockContainer) + itemOffset;
    memcpy(dst, item, b->Stride);

    ++b->Count;
}

inline void ccacheAddItemRange(CCacheContainer* c, CBlockContainer* b, void* items, uint32_t itemsCount)
{
    while (b->Next) b = b->Next; // seek last
    uint32_t itemStride = b->Stride;

    if (b->Count == b->Capacity)
    {
        CBlockContainer* bNew = ccacheAddBlock(c);
        b->Next = bNew; // link to next block
        b = b->Next;
    }

    uint32_t capAvail = b->Capacity - b->Count;
    while (itemsCount > capAvail)
    {
        if (items)
        {
            uint8_t* src = (uint8_t*)items;
            uint8_t* dst = (uint8_t*)b + sizeof(CBlockContainer) + (size_t)itemStride * b->Count;
            uint32_t cpySize = itemStride * capAvail;
            memcpy(src, dst, cpySize);
        }

        b->Count += capAvail;
        itemsCount -= capAvail;

        if (itemsCount)
        {
            CBlockContainer* bNew = ccacheAddBlock(c);
            b->Next = bNew; // link to next block
            b = b->Next;
        }
        
        capAvail = b->Capacity - b->Count;
    }

    if (itemsCount && items)
    {
        uint8_t* src = (uint8_t*)items;
        uint8_t* dst = (uint8_t*)b + sizeof(CBlockContainer) + (size_t)itemStride * b->Count;
        uint32_t cpySize = itemStride * itemsCount;
        memcpy(src, dst, cpySize);
    }

    b->Count += itemsCount;
}



#ifdef __cplusplus
}
#endif // __cplusplus
