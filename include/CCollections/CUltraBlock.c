#include <stdint.h>
#include "CBlockPool.h"
#include "CList.h"

// KEY: (invalidated on buffer pool switch)
// -- poolIndex
// -- blockOffset

typedef struct CUltraBuffer
{
    CList Keys;
    CBlockPool Pools[16];
}CUltraBuffer;

// ultrabuffer

void cultrabufferAlloc(CUltraBuffer* ultra, uint32_t blockStride, uint32_t blockCapacity, uint32_t itemStride, uint32_t keyCapacity)
{
    clistAlloc(&ultra->Keys, sizeof(uint32_t), keyCapacity);

    for (uint32_t i = 0; i != 16; ++i)
    {
        CBlockPool* pool = &ultra->Pools[i];
        cblockpoolAlloc(pool, blockStride, blockCapacity, itemStride);
    }
}

void cultrabufferFree(CUltraBuffer* ultra)
{
    clistFree(&ultra->Keys);
    for (uint32_t i = 0; i != 16; ++i)
    {
        CBlockPool* pool = &ultra->Pools[i];
        cblockpoolFree(pool);
    }
}

uint32_t cultrabufferPoolIndexFromItemCount(CUltraBuffer* ultra, uint32_t itemCount)
{
    // could try bit math later
    uint32_t itemsSize = ultra->Pools[0].SubItemStride * itemCount;
    for (uint32_t i = 0; i != 16; ++i)
    {
        if (itemsSize <= ultra->Pools[i].Base.Stride)
            return i;
    }
    return -1;
}

uint64_t cultrabufferKeyGen(CUltraBuffer* ultra, uint32_t itemCount)
{
    uint64_t poolIndex = cultrabufferPoolIndexFromItemCount(ultra, itemCount);
    CBlockPool* pool = &ultra->Pools[poolIndex];
    CBlockHeader* block = cblockpoolAdd(pool);
    uint64_t blockOffset = cblockpoolOffsetOfBlock(pool, block);
    uint64_t key = poolIndex << 32 | blockOffset;
    return key;
}

CBlockHeader* cultrabufferBlockAt(CUltraBuffer* ultra, uint64_t key)
{
    uint32_t poolIndex = key >> 32;
    uint32_t blockOffset = (uint32_t)key;
    CBlockPool* pool = &ultra->Pools[poolIndex];
    CBlockHeader* block = cblockpoolBlockAtOffset(pool, blockOffset);
    return block;
}

void cultrabufferAddNew(CUltraBuffer* ultra, void* item)
{
    // keygen will allocate a new block if last is at capacity
    uint64_t key = cultrabufferKeyGen(ultra, 1);
    clistAdd(&ultra->Keys, &key);
    CBlockHeader* block = cultrabufferBlockAt(ultra, key);
    size_t itemStride = (size_t)ultra->Pools[key >> 32].SubItemStride;
    uint8_t* dst = (uint8_t*)block + sizeof(CBlockHeader) + itemStride * block->Count;
    memcpy(dst, item, itemStride);
}

void cultrabufferAdd(CUltraBuffer* ultra, void* buffer, uint32_t count)
{
    uint64_t key = cultrabufferKeyGen(ultra, count);
    clistAdd(&ultra->Keys, &key);
    CBlockHeader* block = cultrabufferBlockAt(ultra, key);
    size_t itemStride = (size_t)ultra->Pools[key >> 32].SubItemStride;
    uint8_t* dst = (uint8_t*)block + sizeof(CBlockHeader) + itemStride * block->Count;
    memcpy(dst, buffer, itemStride);
}

void cultrabufferAddAt(CUltraBuffer* ultra, uint32_t keyIndex, void* buffer, uint32_t itemCount)
{
    uint32_t* key = (uint32_t*)clistItemAt(&ultra->Keys, keyIndex);
    uint32_t blockOffset = key[0];
    uint32_t poolIndex = key[1];

    CBlockPool* pool = &ultra->Pools[poolIndex];
    CBlockHeader* block = cblockpoolBlockAtOffset(pool, blockOffset);

    uint32_t newItemsCount = block->Count + itemCount;
    uint32_t newItemsSize = newItemsCount * pool->SubItemStride;
    if (newItemsSize > pool->Base.Stride)
    {
        // move to a bigger block pool
        uint32_t newPoolIndex = cultrabufferPoolIndexFromItemCount(ultra, newItemsCount);
        CBlockPool* newPool = &ultra->Pools[newPoolIndex];
        CBlockHeader* newBlock = cblockpoolAdd(newPool);

        uint8_t* src = (uint8_t*)block + sizeof(CBlockHeader);
        uint8_t* dst = (uint8_t*)newBlock + sizeof(CBlockHeader);
        uint32_t oldSize = block->Count * pool->SubItemStride;
        memcpy(dst, src, oldSize);
        newBlock->Count = block->Count;

        cblockpoolRemove(pool, blockOffset);

        // update key
        key[0] = cblockpoolOffsetOfBlock(newPool, newBlock);
        key[1] = newPoolIndex;

        poolIndex = newPoolIndex;
        pool = newPool;
        block = newBlock;
    }
    // append
    {
        uint8_t* dst = (uint8_t*)block + sizeof(CBlockHeader);
    }

}

void* cultrabufferItemAt(CUltraBuffer* ultra, uint64_t key, uint32_t index)
{
    CBlockHeader* block = cultrabufferBlockAt(ultra, key);
    CBlockPool* pool = &ultra->Pools[key >> 32];
    void* item = cblockpoolItemAtBlock(pool, block, index);
    return item;
}

void* cultrabuferrBegin(CUltraBuffer* ultra, uint64_t key)
{
    CBlockHeader* block = cultrabufferBlockAt(ultra, key);
    uint8_t* ptr = (uint8_t*)block + sizeof(CBlockHeader);
    return ptr;
}

void* cultrabufferEnd(CUltraBuffer* ultra, uint64_t key)
{
    CBlockHeader* block = cultrabufferBlockAt(ultra, key);
    uint8_t* ptr = (uint8_t*)block + (size_t)ultra->Pools[key >> 32].Base.Stride;
    return ptr;
}

void ultrabuffer(uint32_t stride)
{
    for (uint32_t arrayCount = 64; arrayCount != 64 << 15; arrayCount = arrayCount << 1)
    {

    }


}
