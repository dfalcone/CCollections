#include "CCollections/CEntityComponentSystem.h"
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#define ECS_MAX_SIGNITURE_PERMUTATIONS ((ECS_MAX_COMPONENT_TYPES*ECS_MAX_COMPONENT_TYPES+ECS_MAX_COMPONENT_TYPES)/2)

#define ECS_DEFAULT_ARCHETYPE_COUNT 512
#define ECS_DEFAULT_QUERY_COUNT 256
#define ECS_DEFAULT_ENTITY_COUNT 0x10000
#define ECS_DEFAULT_COMPONENT_COUNT 256
#define ECS_ALIGNMENT 64

// aligned_alloc
#if __STDC_VERSION__ >= 201112L // C11 (some compilers may not define this)
    #include <stdlib.h>
    #undef aligned_free
    #define aligned_free free
#elif defined(_MSC_VER)         // MSVC
    #undef aligned_alloc
    #undef aligned_free
    #define aligned_alloc _aligned_malloc
    #define aligned_free _aligned_free
#elif defined(__SSE__)          // Intel Arch. SSE Intrinsics
    #include <xmmintrin.h>
    #undef aligned_alloc
    #undef aligned_free
    #define aligned_alloc _mm_malloc
    #define aligned_free _mm_free
#else                           // fallback to malloc (no alignment)
    #undef aligned_alloc
    #undef aligned_free
    #define aligned_alloc(a,b) malloc(a)
    #define aligned_free free
#endif
// !aligned_alloc

static inline aligned_realloc(void* ptr, size_t size, size_t align)
{
    void* oldPtr = ptr;
    ptr = aligned_alloc(size, align);
    memcpy(ptr, oldPtr, size);
    aligned_free(oldPtr);
}

struct
{
    EcsArchetypeSignature* signatures;
    EcsArchetype* archetypes;
    uint archetypeCount;
    uint archetypeCapacity;
} static ArchetypeContainer = { 0 };

struct
{
    EcsEntity* entities;
    uint entityCount;
    uint entityCapacity;
} static EntityContainer = { 0 };

struct
{
    EcsQuery* queries;
    uint queryCount;
    uint queryCapacity;
} static QueryContainer = { 0 };

void cecsAddComponentToEntity(uint entityId, uint componentId, size_t sizeofComponent)
{
    // get archetype signiture of entity
    EcsEntity entity = EntityContainer.entities[entityId];
    EcsArchetypeSignature* signature = &ArchetypeContainer.signatures[entity.archetypeId];

    // OR componentId with current signiture
    //signature.componentMask[componentId / 8] |= componentId % 8;

    // insert componentId into signiture in sorted order
    {
        uint insId = componentId;
        uint* sigIdItr = signature->componentIds;
        while (*sigIdItr != 0xFFFFFFFF)
        {
            if (insId < *sigIdItr)
            {
                uint tmp = *sigIdItr;
                *sigIdItr = insId;
                insId = tmp;
            }
            ++sigIdItr;
        }
        *sigIdItr = insId;
    }

    // check if signiture already exists - (should be hash table TODO)
    EcsArchetypeSignature* sigBgn = ArchetypeContainer.signatures;
    EcsArchetypeSignature* sigItr = sigBgn;
    EcsArchetypeSignature* sigEnd = ArchetypeContainer.signatures + ArchetypeContainer.archetypeCount;
    for (; sigItr != sigEnd; ++sigItr)
    {
        if (memcmp(sigItr, &signature, sizeof(signature)) == 0)
            break;
    }

    uint archId = (uint)(sigItr - sigBgn);

    // else, add archetype and assign new signiture
    if (sigItr == sigEnd)
    {
        // get current archetype for strides
        EcsArchetype* arch = ArchetypeContainer.archetypes + entity.archetypeId;

        // build componentId and sizeofComponent arg list
        char valist[sizeof(EcsArchetypeSignature) + sizeof(size_t)* ECS_MAX_QUERY_COMPONENTS];
        char* valistItr = valist;
        uint* sigIdItr = signature->componentIds;
        uint componentCount = 0;
        while (*sigIdItr != 0xFFFFFFFF)
        {
            *(uint*)valistItr = *sigIdItr;
            valistItr += sizeof(uint);
            *(size_t*)valistItr = arch->componentArrays[*sigIdItr].stride;
            valistItr += sizeof(size_t);

            ++sigIdItr;
            ++componentCount;
        }

        assert(componentCount <= ECS_MAX_QUERY_COMPONENTS);
        archId = ecsCreateArchetype(componentCount, valist);
    }

    // move entity data to new archetype

    // how to find what index each active bit is
}

EcsEntity* ecsGetEntity(uint entityId)
{
    return &EntityContainer.entities[entityId];
}

EcsArchetype* ecsGetArchetype(uint archetypeId)
{
    return &ArchetypeContainer.archetypes[archetypeId];
}

EcsArchetype* ecsGetArchetypeFromEntity(const EcsEntity* entity)
{
    return &ArchetypeContainer.archetypes[entity->archetypeId];
}

EcsArchetype* ecsGetArchetypeFromEntityId(uint entityId)
{
    const EcsEntity* entity = &EntityContainer.entities[entityId];
    return &ArchetypeContainer.archetypes[entity->archetypeId];
}

void* ecsGetComponentFromArchetype(const EcsArchetype* archetype, uint componentTypeId, uint componentIndex)
{
    const EcsComponentArray* componentArray = &archetype->componentArrays[componentTypeId];
    byte* component = &componentArray->components[componentIndex * componentArray->stride];
    return (void*)component;
}

void* ecsGetComponentFromArchetypeId(uint archetypeId, uint componentTypeId, uint componentIndex)
{
    EcsArchetype* archetype = &ArchetypeContainer.archetypes[archetypeId];
    EcsComponentArray* componentArray = &archetype->componentArrays[componentTypeId];
    byte* component = &componentArray->components[componentIndex * componentArray->stride];
    return (void*)component;
}

void* ecsGetComponentFromEntityId(uint entityId, uint componentTypeId)
{
    const EcsEntity* entity = &EntityContainer.entities[entityId];
    return ecsGetComponentFromArchetypeId(entity->archetypeId, componentTypeId, entity->componentsId);
}

EcsQuery* ecsGetQuery(uint queryId)
{
    return &QueryContainer.queries[queryId];
}


// args are { id }
void ecsCreateArchetypeSigniture(EcsArchetypeSignature* dst, uint componentCount, ...)
{
    assert(componentCount <= ECS_MAX_QUERY_COMPONENTS);

    memset(dst, 0xFFFFFFFF, sizeof(EcsArchetypeSignature));
    va_list valist;
    va_start(valist, componentCount);

    dst->componentIds[0] = va_arg(valist, uint);
    for (uint i = 1; i < componentCount; ++i)
    {
        uint componentId_a = dst->componentIds[i - 1];
        uint componentId_b = va_arg(valist, uint);

        if (componentId_a < componentId_b)
        {
            dst->componentIds[i] = componentId_b;
        }
        else
        {
            dst->componentIds[i - 1] = componentId_b;
            dst->componentIds[i] = componentId_a;
        }

        //// to bitmask
        //uint byteOffset = componentId / 8;
        //uint bitOffset = componentId % 8;
        //byte* qb = (byte*)&dst->componentMask + byteOffset;
        //*qb |= 1 << bitOffset;
    }
    va_end(valist);
}

// args are { id, size }
void ecsCreateArchetypeSignitureAlt(EcsArchetypeSignature* dst, uint componentCount, ...)
{
    memset(dst, 0xFFFFFFFF, sizeof(EcsArchetypeSignature));
    va_list valist;
    va_start(valist, componentCount);

    dst->componentIds[0] = va_arg(valist, uint);
    size_t ignoreSize = va_arg(valist, size_t);

    for (uint i = 1; i < componentCount; ++i)
    {
        uint componentId_a = dst->componentIds[i - 1];
        uint componentId_b = va_arg(valist, uint);
        size_t ignoreSize_ = va_arg(valist, size_t);

        if (componentId_a < componentId_b)
        {
            dst->componentIds[i] = componentId_b;
        }
        else
        {
            dst->componentIds[i - 1] = componentId_b;
            dst->componentIds[i] = componentId_a;
        }
    }
    va_end(valist);
}

uint ecsCreateArchetype(uint componentCount, ...)
{
    uint archId = ArchetypeContainer.archetypeCount;
    
    // allocate capacity
    if (ArchetypeContainer.archetypeCount == ArchetypeContainer.archetypeCapacity)
    {
        if (ArchetypeContainer.archetypeCount == 0)
        {
            ArchetypeContainer.archetypeCapacity = ECS_DEFAULT_ARCHETYPE_COUNT;
            ArchetypeContainer.archetypes = (EcsArchetype*)aligned_alloc(sizeof(EcsArchetype) * ECS_DEFAULT_ARCHETYPE_COUNT, ECS_ALIGNMENT);
            ArchetypeContainer.signatures = (EcsArchetypeSignature*)aligned_alloc(sizeof(EcsArchetypeSignature) * ECS_DEFAULT_ARCHETYPE_COUNT, ECS_ALIGNMENT);
        }
        else
        {
            uint newCapacity = ArchetypeContainer.archetypeCapacity << 1;
            aligned_realloc(ArchetypeContainer.archetypes, sizeof(EcsArchetype) * newCapacity, ECS_ALIGNMENT);
            aligned_realloc(ArchetypeContainer.signatures, sizeof(EcsArchetypeSignature) * newCapacity, ECS_ALIGNMENT);
            ArchetypeContainer.archetypeCapacity = newCapacity;
        }

        // allocate new component arrays
        uint newCount = ArchetypeContainer.archetypeCapacity - ArchetypeContainer.archetypeCount;
        for (uint i = 0; i != newCount; ++i)
        {
            EcsArchetype* arch = ArchetypeContainer.archetypes + ArchetypeContainer.archetypeCount + newCount;
            va_list valist;
            va_start(valist, componentCount * 2);
            for (uint i = 0; i != componentCount * 2; ++i)
            {
                uint componentId = va_arg(valist, uint);
                size_t componentSize = va_arg(valist, size_t);
                EcsComponentArray* compArray = arch->componentArrays + componentId;
                compArray->components = aligned_alloc(componentSize * ECS_DEFAULT_COMPONENT_COUNT, ECS_ALIGNMENT);
                compArray->stride = componentSize;
            }
            va_end(valist);
        }

    }

    EcsArchetype* arch = ArchetypeContainer.archetypes + archId;
    ++ArchetypeContainer.archetypeCount;
    memset(arch, 0, sizeof(EcsArchetype));
    
    EcsArchetypeSignature* sig = ArchetypeContainer.signatures + archId;
    va_list valist;
    va_start(valist, componentCount);
    ecsCreateArchetypeSignitureAlt(sig, componentCount, valist);
    va_end(valist);

    return archId;
}

uint ecsCreateQuery(uint componentCount, ...)
{
    uint queryId = QueryContainer.queryCount;
    
    // allocate capacity
    if (QueryContainer.queryCount == QueryContainer.queryCapacity)
    {
        if (QueryContainer.queryCount == 0)
        {
            QueryContainer.queryCapacity = ECS_DEFAULT_QUERY_COUNT;
            QueryContainer.queries = (EcsQuery*)aligned_alloc(sizeof(EcsQuery) * ECS_DEFAULT_QUERY_COUNT, ECS_ALIGNMENT);
        }
        else
        {
            uint newCapacity = QueryContainer.queryCapacity << 1;
            aligned_realloc(QueryContainer.queries, sizeof(EcsQuery) * newCapacity, ECS_ALIGNMENT);
            QueryContainer.queryCapacity = newCapacity;
        }
    }

    EcsQuery* query = QueryContainer.queries + queryId;
    ++QueryContainer.queryCount;

    va_list valist;
    va_start(valist, componentCount);
    ecsCreateArchetypeSigniture(&query->mask, componentCount, valist);
    va_end(valist);

    EcsArchetypeSignature* sigBgn = ArchetypeContainer.signatures;
    EcsArchetypeSignature* sigItr = sigBgn;
    EcsArchetypeSignature* sigEnd = sigBgn + ArchetypeContainer.archetypeCount;
    uint* archIdItr = query->archetypeIds;
    for (; sigItr != sigEnd; ++sigItr, ++archIdItr)
    {
        //uint notEqual = 0;
        //for (uint si = 0; si != (ECS_MAX_COMPONENT_TYPES / (sizeof(size_t) * 8)); ++si)
        //{
        //    size_t m1 = *(size_t*)&query->mask + si;
        //    size_t m2 = *(size_t*)&sigItr->componentMask + si;
        //    notEqual += (m1 & m2) ^ m1;
        //}
        //
        //if (notEqual)
        //    continue;

        uint bContainsQuery = 0;
        for (uint* qIdItr = query->mask.componentIds; *qIdItr != 0xFFFFFFFF; ++qIdItr)
        {
            uint bContainsId = 0;
            for (uint* sigIdItr = sigItr->componentIds; *sigIdItr != 0xFFFFFFFF; ++sigIdItr)
            {
                bContainsId += *qIdItr == *sigIdItr;
            }

            if (bContainsId == 0)
            {
                bContainsQuery = 0;
                break;
            }
        }

        if (bContainsQuery == 0)
            continue;

        uint archId = (uint)(sigItr - sigBgn);
        *archIdItr = archId;
    }
    assert(query.archetypeIdCount != 0 && "Invalid query paramaters - no archetypes found");

    return queryId;
}

uint ecsCreateEntity(uint archetypeId)
{
    uint entityId = EntityContainer.entityCount;

    if (EntityContainer.entityCount == EntityContainer.entityCapacity)
    {
        if (EntityContainer.entityCount == 0)
        {
            EntityContainer.entityCapacity = ECS_DEFAULT_ENTITY_COUNT;
            EntityContainer.entities = (EcsEntity*)aligned_alloc(sizeof(EcsEntity) * ECS_DEFAULT_ENTITY_COUNT, ECS_ALIGNMENT);
        }
        else
        {
            uint newCapacity = EntityContainer.entityCapacity << 1;
            aligned_realloc(EntityContainer.entities, sizeof(EcsEntity) * newCapacity, ECS_ALIGNMENT);
            EntityContainer.entityCapacity = newCapacity;
        }
    }

    EcsEntity* entity = EntityContainer.entities + entityId;
    ++EntityContainer.entityCount;

    EcsArchetype* archetype = ArchetypeContainer.archetypes + archetypeId;
    EcsArchetypeSignature* signiture = ArchetypeContainer.signatures + archetypeId;

    entity->archetypeId = archetypeId;
    entity->componentsId = archetype->entityCount;

    if (archetype->entityCount == archetype->entityCapacity)
    {
        uint newCapacity = archetype->entityCapacity << 1;
        for (uint* sigIdItr = signiture->componentIds; *sigIdItr != 0xFFFFFFFF; ++sigIdItr)
        {
            EcsComponentArray* compArray = &archetype->componentArrays[*sigIdItr];
            aligned_realloc(compArray->components, compArray->stride * newCapacity, ECS_ALIGNMENT);
        }
        aligned_realloc(archetype->entityIds, sizeof(uint) * newCapacity, ECS_ALIGNMENT);
        archetype->entityCapacity = newCapacity;
    }

    archetype->entityIds[archetype->entityCount] = entityId;
    ++archetype->entityCount;

    return entityId;
}

//typedef void (*ecsQueryCallback1)(uint entityId, void* component);
//typedef void (*ecsQueryCallback2)(uint entityId, void* component[2]);
//typedef void (*ecsQueryCallback3)(uint entityId, void* component[3]);
//typedef void (*ecsQueryCallback4)(uint entityId, void* component[4]);
//typedef void (*ecsQueryCallback5)(uint entityId, void* component[5]);
//typedef void (*ecsQueryCallback6)(uint entityId, void* component[6]);
//typedef void (*ecsQueryCallback7)(uint entityId, void* component[7]);
//typedef void (*ecsQueryCallback8)(uint entityId, void* component[8]);

EcsIterator ecsIterateQueryInit(uint queryId, uint componentCount)
{
    EcsQuery* query = &QueryContainer.queries[queryId];
    EcsIterator out;
    out.archIdItr = query->archetypeIds;
    out.archEntityIndex = 0;
    out.archEntityCount = ArchetypeContainer.archetypes[*out.archIdItr].entityCount;
    return out;
}


EcsIterator* ecsIterateQuery(EcsIterator* itr, uint componentCount, ...)
{
    EcsArchetype* archetype = &ArchetypeContainer.archetypes[*itr->archIdItr];
    if (itr->archEntityIndex == archetype->entityCount)
    {
        ++itr->archIdItr;
        // end of query
        if (*itr->archIdItr == 0xFFFFFFFF)
            return NULL;
        archetype = &ArchetypeContainer.archetypes[*itr->archIdItr];
        itr->archEntityIndex = 0;
        itr->archEntityCount = archetype->entityCount;
    }
    
    va_list args;
    va_start(args, componentCount);
    for (; componentCount != 0; --componentCount)
    {
        EcsComponentArray* compArray = &archetype->componentArrays[*itr->archIdItr];
        byte* comp = &compArray->components[itr->archEntityIndex * compArray->stride];
        *va_arg(args, void**) = (void*)comp;
    }
    va_end(args);

    ++itr->archEntityIndex;
    return itr;
}

EcsIterator* ecsIterateQueryEx(EcsIterator* itr, uint* entityId, uint componentCount, ...)
{
    EcsArchetype* archetype = &ArchetypeContainer.archetypes[*itr->archIdItr];
    if (itr->archEntityIndex == archetype->entityCount)
    {
        ++itr->archIdItr;
        // end of query
        if (*itr->archIdItr == 0xFFFFFFFF)
            return NULL;
        archetype = &ArchetypeContainer.archetypes[*itr->archIdItr];
        itr->archEntityIndex = 0;
        itr->archEntityCount = archetype->entityCount;
    }

    *entityId = archetype->entityIds[itr->archEntityIndex];

    va_list args;
    va_start(args, componentCount);
    for (; componentCount != 0; --componentCount)
    {
        EcsComponentArray* compArray = &archetype->componentArrays[*itr->archIdItr];
        byte* comp = &compArray->components[itr->archEntityIndex * compArray->stride];
        *va_arg(args, void**) = (void*)comp;
    }
    va_end(args);

    ++itr->archEntityIndex;
    return itr;
}

void ecsIterateQueryCallback(uint queryId, uint componentCount, ecsQueryCallback callback)
{
    EcsQuery* query = &QueryContainer.queries[queryId];
    byte* comps[ECS_MAX_QUERY_COMPONENTS];
    byte** compItr = comps;
    for (uint* archIdItr = query->archetypeIds; *archIdItr != 0xFFFFFFFF; ++archIdItr)
    {
        EcsArchetype* archetype = &ArchetypeContainer.archetypes[*archIdItr];
        for (uint entityIndex = 0, entityCount = archetype->entityCount; entityIndex != entityCount; ++entityIndex)
        {
            for (uint* compIdItr = query->mask.componentIds, *compIdEnd = compIdItr + componentCount; compIdItr != compIdEnd; ++compIdItr, ++compItr)
            {
                EcsComponentArray* compArray = &archetype->componentArrays[*compIdItr];
                *compItr = compArray->components + compArray->stride * entityIndex;
            }
            callback(componentCount, comps);
        }
    }
}

void ecsIterateQueryCallbackEx(uint queryId, uint componentCount, ecsQueryCallbackEx callback)
{
    EcsQuery* query = &QueryContainer.queries[queryId];
    byte* comps[ECS_MAX_QUERY_COMPONENTS];
    byte** compItr = comps;
    for (uint* archIdItr = query->archetypeIds; *archIdItr != 0xFFFFFFFF; ++archIdItr)
    {
        EcsArchetype* archetype = &ArchetypeContainer.archetypes[*archIdItr];
        for (uint entityIndex = 0, entityCount = archetype->entityCount; entityIndex != entityCount; ++entityIndex)
        {
            uint entityId = archetype->entityIds[entityIndex];
            for (uint* compIdItr = query->mask.componentIds, *compIdEnd = compIdItr + componentCount; compIdItr != compIdEnd; ++compIdItr, ++compItr)
            {
                EcsComponentArray* compArray = &archetype->componentArrays[*compIdItr];
                *compItr = compArray->components + compArray->stride * entityIndex;
            }
            callback(entityId, componentCount, comps);
        }
    }
}








//
//size_t ecsComputeSignitureHashTableIndex(EcsArchetypeSignature* signiture)
//{
//    size_t hash;
//
//#if UINTPTR_MAX == 0xFFFFFFFF
//    // 32 bit
//
//    const size_t fnv_offset_basis = 2166136261U;
//    const size_t fnv_prime = 16777619U;
//    hash = fnv_offset_basis;
//
//    #if ECS_MAX_COMPONENT_TYPES == 256
//        // unrolled for default case speedup
//        size_t mask0 = *(size_t*)signiture->componentMask;
//        size_t mask1 = *(size_t*)signiture->componentMask + 1;
//        size_t mask2 = *(size_t*)signiture->componentMask + 2;
//        size_t mask3 = *(size_t*)signiture->componentMask + 3;
//        size_t mask4 = *(size_t*)signiture->componentMask + 4;
//        size_t mask5 = *(size_t*)signiture->componentMask + 5;
//        size_t mask6 = *(size_t*)signiture->componentMask + 6;
//        size_t mask7 = *(size_t*)signiture->componentMask + 7;
//
//        hash ^= mask0;
//        hash *= fnv_prime;
//        hash ^= mask1;
//        hash *= fnv_prime;
//        hash ^= mask2;
//        hash *= fnv_prime;
//        hash ^= mask3;
//        hash *= fnv_prime;
//        hash ^= mask4;
//        hash *= fnv_prime;
//        hash ^= mask5;
//        hash *= fnv_prime;
//        hash ^= mask6;
//        hash *= fnv_prime;
//        hash ^= mask7;
//        hash *= fnv_prime;
//
//    #else
//        // user defined max component types, consider implementing your own custom unrolled version here
//        static_assert(ECS_MAX_COMPONENT_TYPES % 32 == 0, "ECS_MAX_COMPONENT_TYPES must be a multiple of 32");
//        for (size_t i = 0; i != (ECS_MAX_COMPONENT_TYPES / 32); ++i)
//        {
//            size_t maski = *(size_t*)signiture->componentMask + i;
//            hash ^= maski;
//            hash *= fnv_prime;
//        }
//    #endif //ECS_MAX_COMPONENT_TYPES == 256
//#else
//    // 64 bit
//    
//    const size_t fnv_offset_basis = 14695981039346656037ULL;
//    const size_t fnv_prime = 1099511628211ULL;
//    hash = fnv_offset_basis;
//
//    #if ECS_MAX_COMPONENT_TYPES == 256
//        // unrolled for default case speedup
//        size_t mask0 = *(size_t*)signiture->componentMask;
//        size_t mask1 = *(size_t*)signiture->componentMask + 1;
//        size_t mask2 = *(size_t*)signiture->componentMask + 2;
//        size_t mask3 = *(size_t*)signiture->componentMask + 3;
//
//        hash ^= mask0;
//        hash *= fnv_prime;
//        hash ^= mask1;
//        hash *= fnv_prime;
//        hash ^= mask2;
//        hash *= fnv_prime;
//        hash ^= mask3;
//        hash *= fnv_prime;
//
//    #else
//        // user defined max component types, consider implementing your own custom unrolled version here
//        static_assert(ECS_MAX_COMPONENT_TYPES % 64 == 0, "ECS_MAX_COMPONENT_TYPES must be a multiple of 64");
//        for (size_t i = 0; i != (ECS_MAX_COMPONENT_TYPES / 64); ++i)
//        {
//            size_t maski = *(size_t*)signiture->componentMask + i;
//            hash ^= maski;
//            hash *= fnv_prime;
//        }
//    #endif // ECS_MAX_COMPONENT_TYPES == 256
//
//#endif // UINTPTR_MAX == 0xFFFFFFFF
//
//    hash %= ECS_MAX_SIGNITURE_PERMUTATIONS;
//    return hash;
//}