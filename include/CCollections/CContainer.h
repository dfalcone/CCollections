//// Copyright (c) 2020 Dante Falcone, The MIT License.
//#pragma once
//#ifndef CCONTAINER_H
//#define CCONTAINER_H
//
//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus
//
//
//#include "CTypes.h"
//#include <stdint.h>
//#include <stdbool.h>
//#include <immintrin.h>
//#include <string.h>
//
//
//#ifndef CCOLLECTIONS_ALIGNMENT
//// default align to common cache line
//#define CCOLLECTIONS_ALIGNMENT 64
//#endif // !CCOLLECTIONS_ALIGNMENT
//
//#ifndef CCOLLECTIONS_IS_POW2
//#define CCOLLECTIONS_IS_POW2(x) ( (x & (x - 1)) == 0 )
//#endif // !CCOLLECTIONS_IS_POW2
//
//#ifndef CCOLLECTIONS_SET_NEXT_POW2
//#define CCOLLECTIONS_SET_NEXT_POW2(x) { --x; x|=x>>1; x|=x>>2; x|=x>>4; x|=x>>8; x|=x>>16; ++x; }
//#endif // !CCOLLECTIONS_NEXT_POW2
//
//inline uint8_t* ccontainerBegin(CContainer* ctr)
//{
//    return (uint8_t*)ctr->Data;
//}
//
//inline uint8_t* ccontainerEnd(CContainer* ctr, size_t stride)
//{
//    return (uint8_t*)ctr->Data + stride * ctr->Count;
//}
//
//inline size_t ccontainerMemSizeCapacity(CContainer* ctr, size_t stride)
//{
//    return stride * ctr->Capacity;
//}
//
//inline size_t ccontainerMemSizeData(CContainer* ctr, size_t stride)
//{
//    return stride * ctr->Count;
//}
//
//inline void ccontainerAlloc(CContainer* ctr, size_t stride, uint32_t capacity)
//{
//    if (!CCOLLECTIONS_IS_POW2(capacity))
//    {
//        CCOLLECTIONS_SET_NEXT_POW2(capacity);
//    }
//    size_t size = stride * capacity;
//    ctr->Data = _mm_malloc(size, CCOLLECTIONS_ALIGNMENT);
//    ctr->Count = 0;
//    ctr->Capacity = capacity;
//    
//}
//
//inline void ccontainerFree(CContainer* ctr)
//{
//    _mm_free(ctr->Data);
//    memset(ctr, 0, sizeof(CContainer));
//}
//
//inline void ccontainerRealloc(CContainer* ctr, size_t stride, uint32_t capacity)
//{
//    if (!CCOLLECTIONS_IS_POW2(capacity))
//    {
//        CCOLLECTIONS_SET_NEXT_POW2(capacity);
//    }
//    size_t size = stride * capacity;
//    void* data = ctr->Data;
//    ctr->Data = _mm_malloc(size, CCOLLECTIONS_ALIGNMENT);
//    memcpy(ctr->Data, data, size);
//    _mm_free(data);
//    ctr->Capacity = capacity;
//}
//
//inline void ccontainerClear(CContainer* ctr)
//{
//    ctr->Count = 0;
//}
//
//inline void ccontainerGrow(CContainer* ctr, size_t stride, uint32_t capacity)
//{
//    if (capacity > ctr->Capacity)
//    {
//        ctrrealloc(ctr, capacity, stride);
//    }
//}
//
//inline void ccontainerShrink(CContainer* ctr, size_t stride, uint32_t capacity)
//{
//    if (capacity < ctr->Capacity)
//    {
//        ctrrealloc(ctr, capacity, stride);
//    }
//}
//
//
//
//#ifdef __cplusplus
//}
//#endif // __cplusplus
//#endif // CCONTAINER_H