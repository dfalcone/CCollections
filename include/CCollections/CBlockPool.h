// MIT License - CCollections
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)
#pragma once
#include "CTypes.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>

//=======================================================================
// CBlockPool
// - fixed size block linked list
// - allocation from a pre-allocated free list
// - backed by single memory allocation pool
// - reallocation of mempory pool on growth by power of 2

inline void cblockpoolAlloc(CBlockPool* pool, uint32_t blockStride, uint32_t blockCapacity, uint32_t itemStride)
{
    // alloc memory
    blockStride += sizeof(CBlockHeader);
    if (!CCOLLECTIONS_IS_POW2(blockCapacity))
    {
        CCOLLECTIONS_SET_NEXT_POW2(blockCapacity);
    }
    pool->Base.Count = 0;
    pool->Base.Capacity = blockCapacity;
    pool->Base.Stride = blockStride;
    uint32_t size = blockCapacity * blockStride;
    pool->Base.Data = (uint8_t*)_mm_malloc(size, CCOLLECTIONS_ALIGNMENT);
    pool->Base.Type = CCOLLECTION_TYPE_BLOCKPOOL;
    pool->SubItemStride = itemStride;

    // init free list
    CBlockHeader* block = (CBlockHeader*)pool->Base.Data;
    CBlockHeader* last = (CBlockHeader*)((uint8_t*)block + size - blockStride);
    uint32_t next = blockStride;
    while (block != last)
    {
        block->Count = 0;
        block->Prev = next - blockStride;
        block->Next = next;
        block = (CBlockHeader*)(pool->Base.Data + next);
        next += blockStride;
    }
    last->Count = 0;
    last->Next = 0;
}

inline void cblockpoolFree(CBlockPool* pool)
{
    _mm_free(pool->Base.Data);
}

inline CBlockHeader* cblockpoolBegin(CBlockPool* pool)
{
    return (CBlockHeader*)pool->Base.Data;
}

inline CBlockHeader* cblockpoolLast(CBlockPool* pool)
{
    CBlockHeader* block = (CBlockHeader*)pool->Base.Data + pool->Last;
    return block;

    // w/out last stored:
    //CBlockHeader* block = (CBlockHeader*)pool->Base.Data;
    //while (block->Next)
    //{
    //    block = (CBlockHeader*)(pool->Base.Data + block->Next);
    //}
    //return block;
}

inline CBlockHeader* cblockpoolEnd(CBlockPool* pool)
{
    CBlockHeader* block = cblockpoolLast(pool);
    block = (CBlockHeader*)((uint8_t*)block + pool->Base.Stride);
    return block;
}

inline CBlockHeader* cblockpoolNext(CBlockPool* pool, CBlockHeader* block)
{
    return (CBlockHeader*)(pool->Base.Data + block->Next);
}

// linear block search from begin O(n)
inline CBlockHeader* cblockpoolPrev(CBlockPool* pool, CBlockHeader* block)
{
    CBlockHeader* blockprev = (CBlockHeader*)pool->Base.Data + block->Prev;
    return blockprev;

    // w/out stored on block
    //CBlockHeader* itr = cblockpoolBegin(pool);
    //CBlockHeader* end = (CBlockHeader*)((uint8_t*)itr + (size_t)pool->Base.Capacity * pool->Base.Stride);
    //CBlockHeader* next;
    //while (itr != end)
    //{
    //    next = cblockpoolNext(pool, itr);
    //    if (next == block)
    //        return itr;
    //    itr = next;
    //}
    //return NULL;
}

// O(n) avoid
//inline CBlockHeader* cblockpoolBlockAt(CBlockPool* pool, uint32_t index)
//{
//    CBlockHeader* block = (CBlockHeader*)pool->Base.Data;
//    while (index--)
//    {
//        block = (CBlockHeader*)(pool->Base.Data + block->Next);
//    }
//    return block;
//}

inline CBlockHeader* cblockpoolBlockAtOffset(CBlockPool* pool, uint32_t offset)
{
    return (CBlockHeader*)(pool->Base.Data + offset);
}

inline uint32_t cblockpoolOffsetOfBlock(CBlockPool* pool, CBlockHeader* block)
{
    return (uint32_t)((uint8_t*)block - pool->Base.Data);
}

inline void* cblockpoolItemAtBlock(CBlockPool* pool, CBlockHeader* block, uint32_t index)
{
    uint32_t offset = pool->SubItemStride * index;
    uint8_t* data = (uint8_t*)block + sizeof(CBlockHeader) + offset;
    return data;
}

inline CBlockHeader* cblockpoolAdd(CBlockPool* pool)
{
    CBlockHeader *blockLast, *blockNew;

    // check if need realloc
    //if (pool->Base.Count == pool->Base.Capacity)
    if (pool->Free == 0)
    {
        //...
    }

    // get free block
    blockNew = cblockpoolBlockAtOffset(pool, pool->Free);
    blockNew->Prev = 0;

    if (pool->Base.Count)
    {
        // get last block
        blockLast = cblockpoolLast(pool);
        blockLast->Next = pool->Free;
        blockNew->Prev = pool->Last;
    }

    // promote last and next free
    pool->Last = pool->Free;
    pool->Free = blockNew->Next;

    blockNew->Count = 0;
    blockNew->Next = 0;

    return blockNew;
}

inline void cblockpoolRemove(CBlockPool* pool, uint32_t blockOffset)
{
    CBlockHeader* blockRemove = cblockpoolBlockAtOffset(pool, blockOffset);
    CBlockHeader* blockPrev = cblockpoolBlockAtOffset(pool, blockRemove->Prev);
    
    // update last and prev (should still be ok with 0 offset, everything will just be 0 on the one and only node)
    pool->Last = blockRemove->Prev;
    blockPrev->Next = 0;

    // demote block to free
    blockRemove->Next = pool->Free;
    blockRemove->Prev = 0;
    pool->Free = blockOffset;
}

inline uint32_t cblockpoolAddAt(CBlockPool* pool, uint32_t blockOffset)
{
    // check if need realloc
    if (pool->Base.Count == pool->Base.Capacity)
    {
        //...
    }

    // goto block
    CBlockHeader* block = cblockpoolBlockAtOffset(pool, blockOffset);
    // set next to free block, goto next
    block->Next = pool->Free;
    block = cblockpoolBlockAtOffset(pool, block->Next);
    // bring next free block to front
    pool->Free = block->Next;
    
    block->Count = 0;
    block->Next = 0;
}

inline void* cblockpoolItemAt(CBlockPool* pool, uint32_t blockOffset, uint32_t itemIndex)
{
    // goto block
    CBlockHeader* block = cblockpoolBlockAtOffset(pool, blockOffset);
    
    // normalize index to block
    // seek to block containing item
    uint32_t capacity = pool->Base.Stride / pool->SubItemStride;
    while (itemIndex >= capacity)
    {
        block = cblockpoolNext(pool, block);
        itemIndex -= capacity;
    }

    // retrieve item
    uint8_t* data = (uint8_t*)block + sizeof(CBlockHeader);
    uint8_t* item = data + itemIndex;
    return (void*)item;
}

inline void cblockpoolAddItemAt(CBlockPool* pool, uint32_t blockOffset, void* item)
{
    CBlockHeader* block = cblockpoolBlockAtOffset(pool, blockOffset);
    while (block->Next)
    {
        block = cblockpoolNext(pool, block);
    }

    // check if reached block capacity
    if (block->Count == (pool->Base.Stride / pool->SubItemStride))
    {
        // allocate next block
        block->Next = pool->Free;
        block = cblockpoolNext(pool, block);
        pool->Free = block->Next;
        block->Count = 0;
        block->Next = 0;
    }

    // add item to block
    uint8_t* data = (uint8_t*)block + sizeof(CBlockHeader);
    uint8_t* dst = data + (size_t)pool->SubItemStride * block->Count;
    memcpy(dst, item, pool->SubItemStride);
    ++block->Count;
}

inline void* cblockpoolPopBackItemAt(CBlockPool* pool, uint32_t blockOffset)
{
    CBlockHeader* block = cblockpoolBlockAtOffset(pool, blockOffset);
    if (block->Count == 0)
        return NULL;
    --block->Count;
    uint8_t* item = (uint8_t*)block + sizeof(CBlockHeader) + block->Count;
    return (void*)item;
}

inline void cblockpoolRemoveItemAtIndex(CBlockPool* pool, uint32_t blockOffset, uint32_t index)
{
    CBlockHeader* block = cblockpoolBlockAtOffset(pool, blockOffset);
    size_t movsize = block->Count - index;
    assert(block->Count != 0);
    --block->Count;
    if (block->Count == 0)
    {
        // get prev block
        CBlockHeader* prevBlock = cblockpoolPrev(pool, block);
        // send block back to free
        prevBlock->Next = block->Next;
        block->Next = pool->Free;
        pool->Free = cblockpoolOffsetOfBlock(pool, block);
        block = prevBlock;
    }
    uint8_t* data = (uint8_t*)block + sizeof(CBlockHeader);
    uint8_t* movsrc = data + (size_t)pool->SubItemStride * index;
    uint8_t* movdst = movsrc - (size_t)pool->SubItemStride;
    memmove(movdst, movsrc, movsize);
}

//=======================================================================
// CMultiBlockPool - move to own header, so not to pull in clist
// - Dynamic array of keys for indirrect lookup of blocks in a CBlockPool
// - lookup O(2) = CList O(1) + CBlockPool O(1)

inline CBlockHeader* cmultiblockpoolItemAt(CMultiBlockPool* multipool, uint32_t key)
{
    CBlockHeader* block = cblockpoolBlockAtOffset(&multipool->Pool, key);
    return block;
}

// returns key to new multi block
inline uint32_t cmultiblockpoolAdd(CMultiBlockPool* multipool)
{
    CBlockHeader* block = cblockpoolAdd(&multipool->Pool);
    uint32_t key = cblockpoolOffsetOfBlock(&multipool->Pool, block);
    // add to key container TODO
    return key;
}