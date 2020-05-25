// MIT License - CCollections
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)

#pragma once
#ifndef CCOLLECTIONS_CTYPES_H
#define CCOLLECTIONS_CTYPES_H

#include <stdint.h>

#ifndef CCOLLECTIONS_ALIGNMENT
// default align to common cache line
// redefine this manually for platforms that differ
#define CCOLLECTIONS_ALIGNMENT 64
#endif // !CCOLLECTIONS_ALIGNMENT

#ifndef CCOLLECTIONS_IS_POW2
#define CCOLLECTIONS_IS_POW2(x) ( (x & (x - 1)) == 0 )
#endif // !CCOLLECTIONS_IS_POW2

#ifndef CCOLLECTIONS_SET_NEXT_POW2
#define CCOLLECTIONS_SET_NEXT_POW2(x) { --x; x|=x>>1; x|=x>>2; x|=x>>4; x|=x>>8; x|=x>>16; ++x; }
#endif // !CCOLLECTIONS_NEXT_POW2

enum CCOLLECTION_TYPE
{
    CCOLLECTION_TYPE_LIST,
    CCOLLECTION_TYPE_ARRAYLIST,
    CCOLLECTION_TYPE_POOL,
    CCOLLECTION_TYPE_BLOCKPOOL,

    CCOLLECTION_ERROR = 0xFFFF,
};

typedef struct CContainer
{
    //! data items contained in the container
    uint8_t* Data;
    //! number of items in the container
    uint32_t Count;
    //! maximum number of items in the container
    uint32_t Capacity;
    //! size in bytes of each item
    uint32_t Stride;
    // the type of container this is - cast to CCOLLECTION_TYPE
    uint16_t Type;
} CContainer;

typedef struct CContainerEx
{
    CContainer Base;

    // for use with multi-dimensional containers
    uint32_t SubItemStride;

    // data offset of next free item - for pool types only
    uint32_t Free;
    // data offset of last item - for pool types only
    uint32_t Last;

    uint32_t Unused;
} CContainerEx;

// dynamic array list of items - interface similar to .NET List
typedef struct CContainer CList;
typedef struct CContainer CArrayList;
// dynamic linked list of fixed size blocks allocated by a free list
typedef struct CContainerEx CBlockPool;


typedef struct CMultiArrayList
{
    CList Keys;
    CArrayList ArrayLists[16];
} CMultiArrayList;

// header for a raw data array at 16 byte offset
typedef struct CArrayHeader
{
    uint32_t Count;
    uint32_t Capacity;
    uint32_t Stride;
    uint32_t UserData;
} CArrayHeader;

// header for a block of data in a linked list
// data at 16 byte offset
typedef struct CBlockHeader
{
    uint32_t Count;
    uint32_t Prev;
    uint32_t Next;
    uint32_t unused;
}CBlockHeader;

// dynamic array of block keys in a CBlockPool - key lookup O(2) (indirrect)
// useful for allocating many buffers of varying sizes and retrieving each by key index
typedef struct CMultiBlockPool
{
    CContainer Keys;
    CBlockPool Pool;
}CMultiBlockPool;

#endif // !CCOLLECTIONS_CTYPES_H