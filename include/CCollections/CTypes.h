// MIT License
// Copyright(c) 2020 Dante Falcone (dantefalcone@gmail.com)

#pragma once
#ifndef CCOLLECTIONS_CTYPES_H
#define CCOLLECTIONS_CTYPES_H

#include <stdint.h>

enum CCOLLECTION_ENUMS
{
    CCOLLECTION_TYPE_LIST   = 0x01,
    //CCOLLECTION_TYPE_SET    = 0x02,

    CCOLLECTION_ERROR = 0xFFFF,
};

//typedef struct CContainer
//{
//    void* Data;
//    uint32_t Count;
//    uint32_t Capacity;
//} CContainer;

typedef struct CContainer
{
    void* Items;
    uint32_t Count;
    uint32_t Capacity;
    
    uint32_t Stride;
    uint32_t Type;
    uint32_t _pad[2];
} CContainer;

typedef struct CContainer CList;
typedef struct CContainer CSet;

#endif // !CCOLLECTIONS_CTYPES_H