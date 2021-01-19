#include "CCollections/CEntityComponentSystem.h"
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

// aligned_alloc
#if defined(_MSC_VER)           // MSVC
    #define ecsAlloc(size,alignment) _aligned_malloc(size,alignment)
    #define ecsRealloc(data,size,alignment) _aligned_realloc(data,size,alignment)
    #define ecsFree _aligned_free
#elif __STDC_VERSION__ >= 201112L || __cplusplus >= 201103L    // C11, C++11
    #include <stdlib.h>
    #define ecsAlloc(size,alignment) aligned_alloc(alignment,size)
    #define ecsFree free
#elif defined(__SSE__) || defined(_M_X64) || defined(_M_IA64) || defined(_M_IX86) || defined(__i386__) || defined(__x86_64__) // Intel Arch. Intrinsics
    #include <xmmintrin.h>
    #define ecsAlloc(size,alignment) _mm_malloc(size,alignment)
    #define ecsFree _mm_free
#endif

#ifndef ecsAlloc
inline void* ecsAlloc(size_t size)
{
    byte* newPtr = (byte*)malloc(size + ECS_ALIGNMENT);
    assert(newPtr);
    byte offset = (byte)ECS_ALIGNMENT - (byte)((size_t)newPtr % ECS_ALIGNMENT);
    newPtr += offset;
    *(newPtr-1) = offset;
    assert( ((size_t)newPtr & ECS_ALIGNMENT) == ECS_ALIGNMENT );
    return newPtr;
}
#endif // !ecsAlloc

#ifndef ecsFree
inline void ecsFree(void* ptr)
{
    byte* oldPtr = (byte*)ptr;
    byte offset = *(oldPtr - 1);
    oldPtr -= offset;
    free(oldPtr);
}
#endif

#ifndef ecsRealloc
inline void* ecsRealloc(void* ptr, size_t size, size_t align)
{
    void* oldPtr = ptr;
    void* newPtr = ecsAlloc(size, align);
    memcpy(ptr, oldPtr, size);
    ecsFree(oldPtr);
    return newPtr;
}
#endif // !ecsRealloc
// !aligned_alloc

inline void ecsSortComponentDescs(const uint count, EcsComponentDesc* descs)
{
    // Implementation of insertion sort algorithm
    EcsComponentDesc temp;
    for (uint32_t i = 1, j; i < count; ++i) 
    {
        temp = descs[i];
        j = i;
        while (j > 0 && descs[j - 1].id > temp.id)
        {
            descs[j] = descs[j - 1];
            --j;
        }
        descs[j] = temp;
    }
}

EcsInstance ecsCreateInstance()
{
    EcsInstance instance;
    memset(&instance, 0, sizeof(EcsInstance));

    instance.EntityContainer.capacity = ECS_DEFAULT_ENTITY_COUNT;
    instance.EntityContainer.entities = (EcsEntity*)ecsAlloc(sizeof(EcsEntity) * ECS_DEFAULT_ENTITY_COUNT, ECS_ALIGNMENT);

    instance.ArchetypeContainer.capacity = ECS_DEFAULT_ARCHETYPE_COUNT;
    instance.ArchetypeContainer.archetypes = (EcsArchetype*)ecsAlloc(sizeof(EcsArchetype) * ECS_DEFAULT_ARCHETYPE_COUNT, ECS_ALIGNMENT);
    instance.ArchetypeContainer.signatures = (EcsArchetypeSignature*)ecsAlloc(sizeof(EcsArchetypeSignature) * ECS_DEFAULT_ARCHETYPE_COUNT, ECS_ALIGNMENT);

    instance.QueryContainer.capacity = ECS_DEFAULT_QUERY_COUNT;
    instance.QueryContainer.queries = (EcsQuery*)ecsAlloc(sizeof(EcsQuery) * ECS_DEFAULT_QUERY_COUNT, ECS_ALIGNMENT);


    return instance;
}

// policy: look for existing matching signiture archetype or create new archetype and move all component data
// adding components is expensive
void ecsAddComponentToEntity(EcsInstance* instance, uint entityId, uint componentId, size_t sizeofComponent)
{
    // get archetype signiture of entity
    EcsEntity* entity = &instance->EntityContainer.entities[entityId];
    EcsArchetypeSignature signature = instance->ArchetypeContainer.signatures[entity->archetypeId];

    // insert componentId into signiture in sorted order - this needs to be a copy!
    {
        uint insId = componentId;
        uint* sigIdItr = signature.componentIds;
        while (*sigIdItr != (uint)-1)
        {
            if (*sigIdItr == insId)
            {
                // already has component, abort
                return;
            }

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

    // check if signiture already exists - (should be hash table? TODO)
    EcsArchetypeSignature* sigBgn = instance->ArchetypeContainer.signatures;
    EcsArchetypeSignature* sigItr = sigBgn;
    EcsArchetypeSignature* sigEnd = sigBgn + instance->ArchetypeContainer.count;
    for (; sigItr != sigEnd; ++sigItr)
    {
        if (memcmp(sigItr, &signature, sizeof(EcsArchetypeSignature)) == 0)
            break;
    }

    // get archetype id from signiture offset - signitures and archetypes are in parallel arrays
    uint archId = (uint)(sigItr - sigBgn);

    // else, create new archetype with new signiture
    if (sigItr == sigEnd)
    {
        // get current archetype for strides
        EcsArchetype* arch = instance->ArchetypeContainer.archetypes + entity->archetypeId;

        // build create archetype info
        EcsComponentDesc componentDescs[ECS_MAX_COMPONENT_TYPES];
        EcsComponentDesc* comDescItr = componentDescs;
        uint* sigIdItr = signature.componentIds;
        uint componentCount = 0;

        // copy the new entity signature
        for (; componentCount < ECS_MAX_COMPONENT_TYPES; ++componentCount, ++sigIdItr, ++comDescItr)
        {
            if (*sigIdItr == (uint)-1)
                break;
            comDescItr->id = *sigIdItr;
            comDescItr->stride = (uint)arch->componentArrays[*sigIdItr].stride;
        }


        assert(componentCount <= ECS_MAX_QUERY_COMPONENTS);
        //EcsCreateArchetypeInfo createInfo = { 0 };
        //createInfo.entitiesInitialCapacity = arch->entityCapacity;
        //createInfo.componentDescsCount = componentCount;
        //createInfo.componentDescs = componentDescs;
        //archId = ecsCreateArchetype(&createInfo);
        archId = ecsCreateArchetype(instance, componentCount, componentDescs, arch->entityCapacity);
    }

    // move entity data to new archetype
    EcsArchetype* newarchetype = ecsGetArchetype(instance, archId);
    EcsArchetype* oldarchetype = ecsGetArchetype(instance, entity->archetypeId);
    uint oldcomponentsid = entity->componentsId;
    assert(oldarchetype->entityIds[oldcomponentsid] == entityId);

    // grow newarchetype if needed - leave space at end for one empty (used for temp swap data)
    if ((newarchetype->entityCount + 2) >= newarchetype->entityCapacity)
    {
        uint newCapacity = newarchetype->entityCapacity * 2;
        for (uint* sigIdItr = signature.componentIds; *sigIdItr != (uint)-1; ++sigIdItr)
        {
            EcsComponentArray* compArray = &newarchetype->componentArrays[*sigIdItr];
            compArray->components = (byte*)ecsRealloc(compArray->components, compArray->stride * newCapacity, ECS_ALIGNMENT);
        }
        newarchetype->entityIds = (uint*)ecsRealloc(newarchetype->entityIds, sizeof(uint) * newCapacity, ECS_ALIGNMENT);
        newarchetype->entityCapacity = newCapacity;
    }
    assert(newarchetype->entityCount < newarchetype->entityCapacity);

    // copy component data to new archetype
    {
        EcsComponentsResultEx componentsDataOld;
        ecsGetComponentsFromEntityIdEx(instance, &componentsDataOld, entityId);
    
        entity->archetypeId = archId;
        entity->componentsId = newarchetype->entityCount;
    
        EcsComponentsResult componentsDataNew;
        ecsGetComponentsFromEntityId(instance, &componentsDataNew, entityId);

        for (uint i = 0, n = componentsDataOld.count; i < n; ++i)
        {
            memcpy(componentsDataNew.components[i], componentsDataOld.descs[i].data, componentsDataOld.descs[i].stride);
        }
    }


    // remove entity from old archetype
    {
        // shift entity data over removed
        byte* mvdst = (byte*)(oldarchetype->entityIds + oldcomponentsid);
        byte* mvsrc = (byte*)(oldarchetype->entityIds + oldcomponentsid + 1);
        byte* mvend = (byte*)(oldarchetype->entityIds + oldarchetype->entityCount);
        size_t mvsize = mvend - mvsrc;
        memmove(mvdst, mvsrc, mvsize);

        --oldarchetype->entityCount;
    }

    assert(newarchetype->entityIds);
    newarchetype->entityIds[newarchetype->entityCount] = entityId;
    ++newarchetype->entityCount;
}

EcsEntity* ecsGetEntity(EcsInstance* instance, uint entityId)
{
    return &instance->EntityContainer.entities[entityId];
}

EcsArchetype* ecsGetArchetype(EcsInstance* instance, uint archetypeId)
{
    return &instance->ArchetypeContainer.archetypes[archetypeId];
}

EcsArchetype* ecsGetArchetypeFromEntity(EcsInstance* instance, const EcsEntity* entity)
{
    return &instance->ArchetypeContainer.archetypes[entity->archetypeId];
}

EcsArchetype* ecsGetArchetypeFromEntityId(EcsInstance* instance, uint entityId)
{
    const EcsEntity* entity = &instance->EntityContainer.entities[entityId];
    return &instance->ArchetypeContainer.archetypes[entity->archetypeId];
}

inline EcsArchetypeSignature* ecsGetArchetypeSignature(EcsInstance* instance, uint archetypeId)
{
    return &instance->ArchetypeContainer.signatures[archetypeId];
}

void* ecsGetComponentFromArchetype(const EcsArchetype* archetype, uint componentTypeId, uint componentIndex)
{
    const EcsComponentArray* componentArray = &archetype->componentArrays[componentTypeId];
    byte* component = &componentArray->components[componentIndex * componentArray->stride];
    return (void*)component;
}

void* ecsGetComponentFromArchetypeId(EcsInstance* instance, uint archetypeId, uint componentTypeId, uint componentIndex)
{
    EcsArchetype* archetype = &instance->ArchetypeContainer.archetypes[archetypeId];
    EcsComponentArray* componentArray = &archetype->componentArrays[componentTypeId];
    byte* component = &componentArray->components[componentIndex * componentArray->stride];
    return (void*)component;
}

void* ecsGetComponentFromEntityId(EcsInstance* instance, uint entityId, uint componentTypeId)
{
    const EcsEntity* entity = &instance->EntityContainer.entities[entityId];
    return ecsGetComponentFromArchetypeId(instance, entity->archetypeId, componentTypeId, entity->componentsId);
}

void ecsGetComponentsFromEntityId(EcsInstance* instance, EcsComponentsResult* dst, uint entityId)
{
    const EcsEntity* entity = &instance->EntityContainer.entities[entityId];
    EcsArchetype* archetype = &instance->ArchetypeContainer.archetypes[entity->archetypeId];
    EcsArchetypeSignature* signature = &instance->ArchetypeContainer.signatures[entity->archetypeId];

    uint componentsId = entity->componentsId;
    uintptr_t* pDstPtr = (uintptr_t*)dst->components;
    uint* pSigComId = signature->componentIds;

    for (uint i = 0; i != ECS_MAX_QUERY_COMPONENTS; ++i)
    {
        if (*pSigComId == 0xFFFFFFFF)
            break;

        EcsComponentArray* pComArray = &archetype->componentArrays[*pSigComId];
        *pDstPtr = (uintptr_t)pComArray->components[pComArray->stride * componentsId];

        ++pSigComId;
        ++pDstPtr;
    }
}

void ecsGetComponentsFromEntityIdEx(EcsInstance* instance, EcsComponentsResultEx* dst, uint entityId)
{
    const EcsEntity* entity = &instance->EntityContainer.entities[entityId];
    EcsArchetype* archetype = &instance->ArchetypeContainer.archetypes[entity->archetypeId];
    EcsArchetypeSignature* signature = &instance->ArchetypeContainer.signatures[entity->archetypeId];

    uint count = 0;
    uint comIdx = entity->componentsId;
    uint* comIdItr = signature->componentIds;
    EcsComponentDescEx* descsItr = dst->descs;
    EcsComponentArray* comArray;
    for (; *comIdItr != (uint)-1; ++comIdItr, ++descsItr, ++count)
    {
        comArray = &archetype->componentArrays[*comIdItr];
        descsItr->id = *comIdItr;
        descsItr->stride = (uint)comArray->stride;
        descsItr->data = &comArray->components[comIdx * comArray->stride];
    }
    dst->count = count;
}

EcsQuery* ecsGetQuery(EcsInstance* instance, uint queryId)
{
    return &instance->QueryContainer.queries[queryId];
}

// args are { id, size }
//void ecsCreateArchetypeSignitureAlt(EcsArchetypeSignature* dst, uint componentCount, ...)
//{
//    memset(dst, 0xFFFFFFFF, sizeof(EcsArchetypeSignature));
//    va_list valist;
//    va_start(valist, componentCount);
//
//    dst->componentIds[0] = va_arg(valist, uint);
//    va_arg(valist, size_t); // skip size param
//
//    for (uint i = 1; i < componentCount; ++i)
//    {
//        uint componentId_a = dst->componentIds[i - 1];
//        uint componentId_b = va_arg(valist, uint);
//        va_arg(valist, size_t); // skip size param
//
//        if (componentId_a < componentId_b)
//        {
//            dst->componentIds[i] = componentId_b;
//        }
//        else
//        {
//            dst->componentIds[i - 1] = componentId_b;
//            dst->componentIds[i] = componentId_a;
//        }
//    }
//    va_end(valist);
//}

static void ecsCreateArchetypeSigniture(EcsInstance* instance, uint archetypeId, uint componentDescsCount, EcsComponentDesc* componentDescs)
{
    ecsSortComponentDescs(componentDescsCount, componentDescs);

    // obtain mem and set all comp ids to invalid
    EcsArchetypeSignature* dst = instance->ArchetypeContainer.signatures + archetypeId;
    memset(dst, -1, sizeof(EcsArchetypeSignature));

    // copy comp ids to sig
    for (uint i = 0; i < componentDescsCount; ++i)
    {
        dst->componentIds[i] = componentDescs[i].id;
    }
}

//uint ecsCreateArchetype2(EcsCreateArchetypeInfo* info)
//{
//    uint archId = ArchetypeContainer.count;
//
//    // allocate archetype capacity
//    if (ArchetypeContainer.count == 0)
//    {
//        ArchetypeContainer.capacity = ECS_DEFAULT_ARCHETYPE_COUNT;
//        ArchetypeContainer.archetypes = (EcsArchetype*)ecsAlloc(sizeof(EcsArchetype) * ECS_DEFAULT_ARCHETYPE_COUNT, ECS_ALIGNMENT);
//        ArchetypeContainer.signatures = (EcsArchetypeSignature*)ecsAlloc(sizeof(EcsArchetypeSignature) * ECS_DEFAULT_ARCHETYPE_COUNT, ECS_ALIGNMENT);
//    }
//    else if (ArchetypeContainer.count == ArchetypeContainer.capacity)
//    {
//        uint newCapacity = ArchetypeContainer.capacity * 2;
//        ArchetypeContainer.archetypes = (EcsArchetype*)ecsRealloc(ArchetypeContainer.archetypes, sizeof(EcsArchetype) * newCapacity, ECS_ALIGNMENT);
//        ArchetypeContainer.signatures = (EcsArchetypeSignature*)ecsRealloc(ArchetypeContainer.signatures, sizeof(EcsArchetypeSignature) * newCapacity, ECS_ALIGNMENT);
//        ArchetypeContainer.capacity = newCapacity;
//    }
//    assert(ArchetypeContainer.count < ArchetypeContainer.capacity);
//
//    ++ArchetypeContainer.count;
//
//    EcsArchetype* arch = ArchetypeContainer.archetypes + archId;
//    assert(arch);
//    memset(arch, 0, sizeof(EcsArchetype));
//    uint entitiesInitialCapacity = info->entitiesInitialCapacity ? info->entitiesInitialCapacity : ECS_DEFAULT_ARCHETYPE_ENTITY_CAPACITY;
//    arch->entityCapacity = entitiesInitialCapacity;
//
//    // allocate components from params signature //TODO: convert to pass in signature ptr as param or one struct params
//    assert(info->componentDescsCount <= ECS_MAX_COMPONENT_TYPES);
//    EcsComponentDesc* comDescItr = info->componentDescs;
//    EcsComponentDesc* comDescEnd = info->componentDescs + info->componentDescsCount;
//    while (comDescItr != comDescEnd)
//    {
//        uint componentId = comDescItr->id;
//        assert(componentId < ECS_MAX_COMPONENT_TYPES);
//        size_t componentSize = (size_t)comDescItr->stride;
//        EcsComponentArray* compArray = arch->componentArrays + componentId;
//        // allocate a component for each entity
//        compArray->components = (byte*)ecsAlloc(componentSize * entitiesInitialCapacity, ECS_ALIGNMENT);
//        compArray->stride = componentSize;
//
//        ++comDescItr;
//    }
//
//    ecsCreateArchetypeSigniture(archId, info->componentDescsCount, info->componentDescs);
//    
//    return archId;
//}

uint ecsCreateArchetype(EcsInstance* instance, uint componentCount, EcsComponentDesc* componentDescs, uint initialCapacity)
{
    uint archId = instance->ArchetypeContainer.count;

    // allocate archetype capacity
    if (instance->ArchetypeContainer.count == instance->ArchetypeContainer.capacity)
    {
        uint newCapacity = instance->ArchetypeContainer.capacity * 2;
        instance->ArchetypeContainer.archetypes = (EcsArchetype*)ecsRealloc(instance->ArchetypeContainer.archetypes, sizeof(EcsArchetype) * newCapacity, ECS_ALIGNMENT);
        instance->ArchetypeContainer.signatures = (EcsArchetypeSignature*)ecsRealloc(instance->ArchetypeContainer.signatures, sizeof(EcsArchetypeSignature) * newCapacity, ECS_ALIGNMENT);
        instance->ArchetypeContainer.capacity = newCapacity;
    }
    assert(instance->ArchetypeContainer.count < instance->ArchetypeContainer.capacity);

    ++instance->ArchetypeContainer.count;


    EcsArchetype* arch = instance->ArchetypeContainer.archetypes + archId;
    assert(arch);
    memset(arch, 0, sizeof(EcsArchetype));
    uint capacity = initialCapacity ? initialCapacity : ECS_DEFAULT_ARCHETYPE_ENTITY_CAPACITY;
    arch->entityCapacity = capacity;

    // allocate entities capacity
    {
        arch->entityIds = (uint*)ecsAlloc(sizeof(uint) * capacity, ECS_ALIGNMENT);
    }

    for (uint i = 0; i < componentCount; ++i)
    {
        EcsComponentDesc comDesc = componentDescs[i];
        assert(comDesc.id < ECS_MAX_COMPONENT_TYPES);
        EcsComponentArray* comArray = &arch->componentArrays[comDesc.id];
        // allocate a component for each entity
        comArray->components = (byte*)ecsAlloc((size_t)comDesc.stride * capacity, ECS_ALIGNMENT);
        comArray->stride = (size_t)comDesc.stride;
    }

    ecsCreateArchetypeSigniture(instance, archId, componentCount, componentDescs);

    return archId;
}

uint ecsCreateQuery(EcsInstance* instance, uint componentCount, uint componentIds, ...)
{
    assert(componentCount <= ECS_MAX_QUERY_COMPONENTS && "componentCount exceeds ECS_MAX_QUERY_COMPONENTS - redefine to next pow2");
    uint queryId = instance->QueryContainer.count;
    
    // allocate capacity
    if (instance->QueryContainer.count == instance->QueryContainer.capacity)
    {
        uint newCapacity = instance->QueryContainer.capacity * 2;
        instance->QueryContainer.queries = (EcsQuery*)ecsRealloc(instance->QueryContainer.queries, sizeof(EcsQuery) * newCapacity, ECS_ALIGNMENT);
        instance->QueryContainer.capacity = newCapacity;
    }
    assert(instance->QueryContainer.count < instance->QueryContainer.capacity);
    assert(instance->QueryContainer.queries);

    EcsQuery* query = &instance->QueryContainer.queries[queryId];
    memset(query, (uint)-1, sizeof(EcsQuery));
    query->archetypeCount = 0;
    query->componentCount = componentCount;
    ++instance->QueryContainer.count;

    va_list args;
    va_start(args, componentCount);
    for (uint i = 0; i < componentCount; ++i)
    {
        query->componentIds[i] = va_arg(args, uint);
    }
    va_end(args);

#if !defined(NDEBUG)
    // check if query already exists
    for (uint i = 0; i < queryId; ++i)
    {
        EcsQuery* queryItr = &instance->QueryContainer.queries[i];
        if (memcmp(query, queryItr->componentIds, sizeof(queryItr->componentIds)) == 0)
        {
            // log warning
            fprintf(stderr, "warning: ecsCreateQuery: attempt to create query that already exists, returning found query");
            --instance->QueryContainer.count;
            return i;
        }
    }
#endif

    EcsArchetypeSignature* sigBgn = instance->ArchetypeContainer.signatures;
    EcsArchetypeSignature* sigItr = sigBgn;
    EcsArchetypeSignature* sigEnd = sigBgn + instance->ArchetypeContainer.count;
    EcsArchetype** archItr = query->archetypes;

    uint bQueryValid = 0;
    for (; sigItr != sigEnd; ++sigItr, ++archItr, ++query->archetypeCount)
    {
        uint bContainsQuery = 1;

        for (uint* qIdItr = query->componentIds, *qIdEnd = qIdItr+componentCount; qIdItr != qIdEnd; ++qIdItr)
        {
            uint bContainsId = 0;
            for (uint* sigIdItr = sigItr->componentIds; *sigIdItr != (uint)-1; ++sigIdItr)
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
        *archItr = &instance->ArchetypeContainer.archetypes[archId];
        bQueryValid = 1;
    }
    assert(bQueryValid && "Invalid query paramaters - no archetypes found");

    return queryId;
}

uint ecsCreateEntity(EcsInstance* instance, uint archetypeId)
{
    uint entityId = instance->EntityContainer.count;

    if (instance->EntityContainer.count == instance->EntityContainer.capacity)
    {
        uint newCapacity = instance->EntityContainer.capacity * 2;
        instance->EntityContainer.entities = (EcsEntity*)ecsRealloc(instance->EntityContainer.entities, sizeof(EcsEntity) * newCapacity, ECS_ALIGNMENT);
        instance->EntityContainer.capacity = newCapacity;
    }
    assert(instance->EntityContainer.count < instance->EntityContainer.capacity);

    EcsEntity* entity = instance->EntityContainer.entities + entityId;
    assert(entity);
    ++instance->EntityContainer.count;

    EcsArchetype* archetype = instance->ArchetypeContainer.archetypes + archetypeId;
    EcsArchetypeSignature* signiture = instance->ArchetypeContainer.signatures + archetypeId;

    entity->archetypeId = archetypeId;
    entity->componentsId = archetype->entityCount;

    // grow archetype if needed - allocates 1 extra at end for temp swap data
    if ((archetype->entityCount + 2) >= archetype->entityCapacity)
    {
        uint newCapacity = archetype->entityCapacity * 2;
        for (uint* sigIdItr = signiture->componentIds; *sigIdItr != 0xFFFFFFFF; ++sigIdItr)
        {
            EcsComponentArray* compArray = &archetype->componentArrays[*sigIdItr];
            compArray->components = (byte*)ecsRealloc(compArray->components, compArray->stride * newCapacity, ECS_ALIGNMENT);
        }
        archetype->entityIds = (uint*)ecsRealloc(archetype->entityIds, sizeof(uint) * newCapacity, ECS_ALIGNMENT);
        archetype->entityCapacity = newCapacity;
    }

    assert(archetype->entityIds);
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

EcsQueryIterator ecsCreateQueryIterator(EcsInstance* instance, uint queryId)
{
    EcsQuery* query = &instance->QueryContainer.queries[queryId];
    EcsQueryIterator out;
    out.query = query;
    out.archIdIndex = 0;
    out.archEntityIndex = -1;
    return out;
}


EcsQueryIterator* ecsIterateQuery(EcsQueryIterator* itr, void** componentsArray)
{
    // initial value is -1, so first call sets to 0
    ++itr->archEntityIndex;

    EcsQuery* query = itr->query;
    EcsArchetype* archetype = query->archetypes[itr->archIdIndex];

    if (itr->archEntityIndex == archetype->entityCount)
    {
        ++itr->archIdIndex;
        // end of query
        if (itr->archIdIndex == query->archetypeCount)
            return NULL;
        archetype = query->archetypes[itr->archIdIndex];
        itr->archEntityIndex = 0;
    }

    for (uint i = 0, n = query->componentCount; i < n; ++i)
    {
        uint comId = query->componentIds[i];
        EcsComponentArray* comArray = &archetype->componentArrays[comId];
        void* comp = &comArray->components[itr->archEntityIndex * comArray->stride];
        componentsArray[i] = comp;
    }

    return itr;
}

EcsQueryIterator* ecsIterateQueryEx(EcsQueryIterator* itr, uint* entityId, void** componentsArray)
{
    // initial value is -1, so first call sets to 0
    ++itr->archEntityIndex;

    EcsQuery* query = itr->query;
    EcsArchetype* archetype = query->archetypes[itr->archIdIndex];

    if (itr->archEntityIndex == archetype->entityCount)
    {
        ++itr->archIdIndex;
        // end of query
        if (itr->archIdIndex == query->archetypeCount)
            return NULL;
        archetype = query->archetypes[itr->archIdIndex];
        itr->archEntityIndex = 0;
    }

    for (uint i = 0, n = query->componentCount; i < n; ++i)
    {
        uint comId = query->componentIds[i];
        EcsComponentArray* comArray = &archetype->componentArrays[comId];
        void* comp = &comArray->components[itr->archEntityIndex * comArray->stride];
        componentsArray[i] = comp;
    }

    *entityId = archetype->entityIds[itr->archEntityIndex];
    
    return itr;
}

void ecsIterateQueryCallback(EcsInstance* instance, uint queryId, EcsQueryCallback callback)
{
    EcsQuery* query = &instance->QueryContainer.queries[queryId];
    uint archCount = query->archetypeCount;
    uint comCount = query->componentCount;
    EcsArchetype* archetype;
    uint entCount;
    EcsComponentArray* comArray;
    void* coms[ECS_MAX_QUERY_COMPONENTS];
    for (uint archIdx = 0; archIdx < archCount; ++archIdx)
    {
        archetype = query->archetypes[archIdx];
        entCount = archetype->entityCount;
        for (uint entIdx = 0; entIdx < entCount; ++entIdx)
        {
            for (uint comIdx = 0; comIdx < comCount; ++comIdx)
            {
                comArray = &archetype->componentArrays[query->componentIds[comIdx]];
                coms[comIdx] = &comArray->components[comArray->stride * entIdx];
            }
            callback(coms);
        }
    }
}

void ecsIterateQueryCallbackEx(EcsInstance* instance, uint queryId, EcsQueryCallbackEx callback)
{
    EcsQuery* query = &instance->QueryContainer.queries[queryId];
    uint archCount = query->archetypeCount;
    uint comCount = query->componentCount;
    EcsArchetype* archetype;
    uint entCount;
    uint entId;
    EcsComponentArray* comArray;
    void* coms[ECS_MAX_QUERY_COMPONENTS];
    for (uint archIdx = 0; archIdx < archCount; ++archIdx)
    {
        archetype = query->archetypes[archIdx];
        entCount = archetype->entityCount;
        for (uint entIdx = 0; entIdx < entCount; ++entIdx)
        {
            entId = archetype->entityIds[entIdx];

            for (uint comIdx = 0; comIdx < comCount; ++comIdx)
            {
                comArray = &archetype->componentArrays[query->componentIds[comIdx]];
                coms[comIdx] = &comArray->components[comArray->stride * entIdx];
            }
            callback(entId, coms);
        }
    }
}


void ecsDestroyEntity(EcsInstance* instance, uint entityId)
{
    EcsEntity* entity = ecsGetEntity(instance, entityId);
    const EcsArchetypeSignature* signature = ecsGetArchetypeSignature(instance, entity->archetypeId);
    EcsArchetype* archetype = ecsGetArchetype(instance, entity->archetypeId);
    const uint comIdA = entity->componentsId; // componentsId is the index of both the entity and components
    const uint comIdB = --archetype->entityCount; // pop
    uint entityIdB = archetype->entityIds[comIdB];

    // move last to removed
    archetype->entityIds[comIdA] = entityIdB;
    *entity = *ecsGetEntity(instance, entityIdB);

    const uint* comIdItr = signature->componentIds;
    EcsComponentArray* componentGroup;
    size_t comStride;
    for (; *comIdItr != (uint)-1; ++comIdItr)
    {
        componentGroup = &archetype->componentArrays[*comIdItr];
        comStride = componentGroup->stride;
        memcpy(&componentGroup->components[comIdA * comStride], &componentGroup->components[comIdB * comStride], comStride);
    }
}



// internal use - for sorting
void ecsSwapEntity(EcsArchetype* archetype, EcsArchetypeSignature* signature, EcsEntity* entityA, EcsEntity* entityB)
{
    assert(entityA->archetypeId == entityB->archetypeId);
    assert(archetype->entityCount < archetype->entityCapacity);

    const uint comIdA = entityA->componentsId;
    const uint comIdB = entityB->componentsId;

    // swap entities
    {
        EcsEntity entityTemp = *entityA;
        *entityA = *entityB;
        *entityB = entityTemp;
    }

    // swap components
    {
        EcsComponentArray* componentGroup;
        size_t comOffsetA, comOffsetB, comStride;
        for (uint* comIdItr = signature->componentIds; *comIdItr != (uint)-1; ++comIdItr)
        {
            componentGroup = &archetype->componentArrays[*comIdItr];
            comStride = componentGroup->stride;
            assert(comStride <= ECS_MAX_COMPONENT_SIZE);
            comOffsetA = comIdA * comStride;
            comOffsetB = comIdB * comStride;
            byte* comTemp = &componentGroup->components[archetype->entityCount]; // empty at end
            memcpy(comTemp, &componentGroup->components[comOffsetA], comStride);
            memcpy(&componentGroup->components[comOffsetA], &componentGroup->components[comIdB], comStride);
            memcpy(&componentGroup->components[comOffsetB], comTemp, comStride);
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