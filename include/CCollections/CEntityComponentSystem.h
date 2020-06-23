#pragma once
#include <stdint.h>

#ifndef ECS_MAX_COMPONENT_TYPES
#define ECS_MAX_COMPONENT_TYPES 256
#endif // !ECS_MAX_COMPONENT_TYPES

#ifndef ECS_MAX_QUERY_COMPONENTS
#define ECS_MAX_QUERY_COMPONENTS 16
#endif // !ECS_MAX_QUERY_COMPONENTS

/* Example Usage:

typedef struct Position { float x, y, z, w; } Position;
typedef struct Attributes { int a, b, c, d; } Attributes;

// component ids are simply an index from 0 to (ECS_MAX_COMPONENT_TYPES - 1)
// it is your responsibility to keep track of component id to component struct mapping
// if you really don't want to use direct access enums etc., you can use std::unordered_map or similar hash table (not recommended)
enum EComponentIds
{
    ePositionId,
    eAttributesId
};

int main()
{
    // store archId for fast entity creation, no searching or hash tables
    uint archId = ecsCreateArchetype(2, ePositionId, sizeof(Position), eAttributesId, sizeof(Attributes));
    
    // store queryId on your system for fast iteration, no searching or hash tables
    uint queryId = ecsCreateQuery(2, ePositionId, eAttributesId);

    // it is also possible to predefine your query ids in an enum, as the ids are sequential
    enum { ePositionAttributeQuery = 0 }; // the first queryId is 0, second is 1 etc.

    // create entities ... (entity ids are also sequential, starting from 0)
    uint entityId = ecsCreateEntity(archId);
    // ...

    // iterate query, appropriate for inside a system tick function
    // each iteration of the while loop updates component pointers for current entity
    Position* ptrPosition;
    Attributes* ptrAttributes;
    EcsIterator itr = ecsIterateQueryInit((uint)eMyQuery, 2);
    while( ecsIterateQuery(&itr, 2, &ptrPosition, &ptrAttributes) )
    {
        ptrPosition->x += ptrAttributes->b;
    }

    // the ecsIterateQueryEx function retrieves the current entity id as well
    uint curEntityId;
    EcsIterator itr = ecsIterateQueryInit((uint)eMyQuery, 2);
    while( ecsIterateQuery(&itr, &curEntityId, 2, &ptrPosition, &ptrAttributes) )
    {
        ptrPosition->x += ptrAttributes->b;
        printf("Processed components for entity %u", curEntityId);
    }

    // TODO: add/remove components, sort order tags, entity flags, bulk creations
}

*/

typedef uint32_t uint;
typedef uint8_t byte;

typedef void (*ecsQueryCallback)(uint componentCount, ...);
typedef void (*ecsQueryCallbackEx)(uint entityId, uint componentCount, ...);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct EcsEntity
{
    uint32_t archetypeId; // index of archetype
    uint32_t componentsId; // index to component in each array
    uint32_t sortOrder;
    uint32_t flags;
} EcsEntity;

typedef struct EcsComponentArray
{
    byte* components;
    size_t stride;
} EcsComponentArray;

typedef struct EcsArchetype
{
    uint* entityIds;
    uint entityCount;
    uint entityCapacity;

    // sparse array for O(1) indexing by componentId
    // entity count/capacity used to maintain dynamic arrays in unison
    struct EcsComponentArray componentArrays[ECS_MAX_COMPONENT_TYPES];
} EcsArchetype;

/// @brief packed array of sorted component ids
typedef struct EcsArchetypeSignature
{
    uint componentIds[ECS_MAX_QUERY_COMPONENTS];
} EcsArchetypeSignature;

/// @brief explicitly define queries that keep track of compatible archetypes
/// use ecsCreateQuery
typedef struct EcsQuery
{
    EcsArchetypeSignature mask;
    uint archetypeIds[ECS_MAX_QUERY_COMPONENTS];
} EcsQuery;

typedef struct EcsIterator
{
    uint* archIdItr;
    uint archEntityIndex;
    uint archEntityCount;
} EcsIterator;

extern inline EcsEntity* ecsGetEntity(uint entityId);
extern inline EcsArchetype* ecsGetArchetype(uint archetypeId);
extern inline EcsArchetype* ecsGetArchetypeFromEntity(const EcsEntity* entity);
extern inline EcsArchetype* ecsGetArchetypeFromEntityId(uint entityId);
extern inline void* ecsGetComponentFromArchetype(const EcsArchetype* archetype, uint componentTypeId, uint componentIndex);
extern inline void* ecsGetComponentFromArchetypeId(uint archetypeId, uint componentTypeId, uint componentIndex);
extern inline void* ecsGetComponentFromEntityId(uint entityId, uint componentTypeId);
extern inline EcsQuery* ecsGetQuery(uint queryId);

/// @brief initialize iteration of a query.
/// ex. EcsIterator itr = ecsIterateQueryInit((uint)eMyQuery, 2);
/// @param queryId: created with ecsCreateQuery
/// @param componentCount: number of components in query
/// @return EcsIterator object for use with ecsIterateQuery
extern inline EcsIterator ecsIterateQueryInit(uint queryId, uint componentCount);

/// @brief iterate all archetypes and components in query. 
/// ex: while( ecsIterateQuery(&itr, 2, &ptrCompA, &ptrCompB) ) ptrCompA->x = ptrCompB->y;
/// @param itr: address of EcsIterator object created from ecsIterateQueryInit
/// @param componentCount: number of components in query
/// @param ...: address of component pointer args - &ptrCompA, &ptrCompB, ...
/// @return 
extern inline EcsIterator* ecsIterateQuery(EcsIterator* itr, uint componentCount, ...);

/// @brief creates a query for iterating components in all applicable archetypes
/// @param componentCount: number of component args in query
/// @param ...: componentId args
/// @return queryId
extern uint ecsCreateQuery(uint componentCount, ...);

/// @brief create an archetype, a collection of components which entities are assigned to
/// @param componentCount: number of component args
/// @param ...: componentId, sizeofComponent, ...
/// @return archetypeId
extern uint ecsCreateArchetype(uint componentCount, ...);

/// @brief create an entity assigned to archetype
/// @param archetypeId: (see ecsCreateArchetype)
/// @return entityId
extern uint ecsCreateEntity(uint archetypeId);

extern uint ecsCreateEntities(uint archetypeId, uint count);

extern void ecsReserveArchetypeCapacity(uint newCapacity);
extern void ecsReserveEntityCapacity(uint newCapacity);
extern void ecsReserveArchetypeEntityCapacity(uint newCapacity);
extern void ecsReserveQueryCapacity(uint newCapacity);

#ifdef __cplusplus
}
#endif // __cplusplus