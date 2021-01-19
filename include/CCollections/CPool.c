#include "CTypes.h"
#include <stdint.h>
#include <malloc.h>
#include <string.h>

// union with object while in free list
typedef struct CFreeHeader
{
    size_t Next;
} CFreeHeader;

typedef CContainerEx CPool;

////////////////////////////////////
// CPool
// - allocates from a free list, re-allocates on capacity exceeded
// - indices are unstable, order is not preserved, caller can obtain changes on relevant function calls

void cpoolAlloc(CPool* pool, uint32_t stride, uint32_t capacity)
{
    if (!CCOLLECTIONS_IS_POW2(capacity))
    {
        CCOLLECTIONS_SET_NEXT_POW2(capacity);
    }
    
    memset(pool, 0, sizeof(CPool));
    pool->Base.Capacity = capacity;
    pool->Base.Stride = stride;
    
    uint32_t size = stride * capacity;
    pool->Base.Data = (uint8_t*)_mm_malloc(size, CCOLLECTIONS_ALIGNMENT);
    pool->Base.Type = CCOLLECTION_TYPE_POOL;

    // init free linked list
    CFreeHeader* itr = (CFreeHeader*)pool->Base.Data;
    CFreeHeader* last = (CFreeHeader*)((uint8_t*)itr + size - stride);
    uint32_t next = stride;
    while (itr != last)
    {
        itr->Next = next;
        itr = (CFreeHeader*)((uint8_t*)itr + stride);
        next += stride;
    }
    last->Next = 0;

}

void cpoolFree(CPool* pool)
{
    _mm_free(pool->Base.Data);
}

uint8_t* cpoolBegin(CPool* pool)
{
    return pool->Base.Data;
}

uint8_t* cpoolEnd(CPool* pool)
{
    uint32_t offset = pool->Base.Capacity * pool->Base.Stride;
    return pool->Base.Data + offset;
}

uint8_t* cpoolLast(CPool* pool)
{
    return pool->Base.Data + pool->Last;
}

uint8_t* cpoolItemAt(CPool* pool, uint32_t index)
{
    uint32_t offset = pool->Base.Stride * index;
    return pool->Base.Data + offset;
}

//void cpoolRemoveAt_Sparse(CPool* pool, uint32_t index)
//{
//    CFreeHeader* remove = (CFreeHeader*)cpoolItemAt(pool, index);
//    CFreeHeader* last = (CFreeHeader*)cpoolLast(pool);
//    // move remove to free
//    remove->Next = pool->Free;
//    pool->Free = (uint32_t)((uint8_t*)remove - pool->Base.Data);
//    --pool->Base.Count;
//}

struct CPoolIndexChange
{
    uint32_t From;
    uint32_t To;
};

void cpoolRemoveLast(CPool* pool)
{
    CFreeHeader* last = (CFreeHeader*)cpoolLast(pool);
    
    // move last to free
    last->Next = pool->Free;
    pool->Free = (uint32_t)((uint8_t*)last - pool->Base.Data);
    
    --pool->Base.Count;
}

void cpoolRemoveAt(CPool* pool, uint32_t index)
{
    CFreeHeader* remove = (CFreeHeader*)cpoolItemAt(pool, index);
    CFreeHeader* last = (CFreeHeader*)cpoolLast(pool);
    
    // copy last to remove and pop
    memcpy(remove, last, pool->Base.Stride);
    cpoolRemoveLast(pool);

    // how to best subsribe to changes? alt function call most likely
    //struct CPoolIndexChange notify;
    //notify.From = pool->Base.Count;
    //notify.To = index;
}

void cpoolAddRange(CPool* pool, uint32_t index)
{

}










