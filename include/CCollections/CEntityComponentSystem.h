#pragma once
#include <stdint.h>

#ifndef ECS_MAX_COMPONENT_TYPES
#define ECS_MAX_COMPONENT_TYPES 255
#endif // !ECS_MAX_COMPONENT_TYPES

#ifndef ECS_MAX_COMPONENT_SIZE
// max byte size of a component, required for efficient sorting
#define ECS_MAX_COMPONENT_SIZE 0x2000
#endif // !ECS_MAX_COMPONENT_SIZE

#ifndef ECS_MAX_QUERY_COMPONENTS
#define ECS_MAX_QUERY_COMPONENTS 15
#endif // !ECS_MAX_QUERY_COMPONENTS

#ifndef ECS_MAX_QUERY_ARCHETYPES
#define ECS_MAX_QUERY_ARCHETYPES 503
#endif // !ECS_MAX_QUERY_ARCHETYPES

#ifndef ECS_DEFAULT_ENTITY_COUNT 
#define ECS_DEFAULT_ENTITY_COUNT 0x10000
#endif // !ECS_DEFAULT_ENTITY_COUNT 

#ifndef ECS_DEFAULT_ARCHETYPE_COUNT
#define ECS_DEFAULT_ARCHETYPE_COUNT 512
#endif // !ECS_DEFAULT_ARCHETYPE_COUNT

#ifndef ECS_DEFAULT_ARCHETYPE_ENTITY_CAPACITY
#define ECS_DEFAULT_ARCHETYPE_ENTITY_CAPACITY 256
#endif // !ECS_DEFAULT_ARCHETYPE_ENTITY_CAPACITY

#ifndef ECS_DEFAULT_QUERY_COUNT
#define ECS_DEFAULT_QUERY_COUNT 256
#endif // !ECS_DEFAULT_QUERY_COUNT

#ifndef ECS_ALIGNMENT 
#define ECS_ALIGNMENT 4096
#endif // !ECS_ALIGNMENT 


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

typedef void (*EcsQueryCallback)(void** components);
typedef void (*EcsQueryCallbackEx)(uint entityId, void** components);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct EcsEntity
{
    uint32_t archetypeId; // index of archetype
    uint32_t componentsId; // unified index to all components data within archetype, also to entityId index

    // --- not sure if need below --- could be important to keep size small
    uint32_t sortOrder;
    uint32_t flags;
} EcsEntity;

typedef struct EcsComponentArray
{
    byte* components;
    size_t stride;
} EcsComponentArray;

/// @brief packed array of sorted component ids
/// the array is terminated by a value of -1
typedef struct EcsArchetypeSignature
{
    uint componentIds[ECS_MAX_COMPONENT_TYPES + 1];
} EcsArchetypeSignature;
//int sizeofSignature = sizeof(EcsArchetypeSignature); // default 1024

/// @brief the defining type of an entity - similar to class
/// stores all component data and keeps track of entities assigned
/// get signiture of component ids from 
typedef struct EcsArchetype
{
    uint* entityIds;
    uint entityCount;
    uint entityCapacity;

    // sparse array for O(1) indexing by componentId
    // entity count/capacity used to maintain dynamic arrays in unison
    // note: todo: inspect behavior of accessing invalid component
    EcsComponentArray componentArrays[ECS_MAX_COMPONENT_TYPES];
} EcsArchetype;
//int sizeofArchetype = sizeof(EcsArchetype); // default 4096

/// @brief explicitly define queries that keep track of compatible archetypes
/// use ecsCreateQuery
typedef struct EcsQuery
{
    uint componentCount;
    uint archetypeCount;
    uint componentIds[ECS_MAX_QUERY_COMPONENTS];
    EcsArchetype* archetypes[ECS_MAX_QUERY_ARCHETYPES];
} EcsQuery;
//int sizeofQuery = sizeof(EcsQuery); // default 4096

typedef struct EcsQueryResult
{
    void* components[ECS_MAX_QUERY_COMPONENTS+1];
} EcsQueryResult;
//int sizeofQueryResult = sizeof(EcsQueryResult); // default 128

typedef struct EcsQueryIterator
{
    EcsQuery* query;

    uint archIdIndex;
    uint archEntityIndex;

} EcsQueryIterator;

typedef struct EcsComponentDesc
{
    uint id;
    uint stride;
} EcsComponentDesc;

typedef struct EcsComponentDescEx
{
    uint id;
    uint stride;
    void* data;
} EcsComponentDescEx;

typedef struct EcsComponentsResult
{
    uint count;
    void* components[ECS_MAX_COMPONENT_TYPES];
} EcsComponentsResult;
//int sizeofComponentsResult = sizeof(EcsComponentsResult); // default 2048

typedef struct EcsComponentsResultEx
{
    uint count;
    EcsComponentDescEx descs[ECS_MAX_COMPONENT_TYPES];
} EcsComponentsResultEx;
//int sizeofComponentsResultEx = sizeof(EcsComponentsResultEx); // default 4088

typedef struct EcsInstance
{
    struct ArchetypeContainer_T
    {
        EcsArchetypeSignature* signatures;
        EcsArchetype* archetypes;
        uint count;
        uint capacity;
    } ArchetypeContainer;

    struct EntityContainer_T
    {
        EcsEntity* entities;
        uint count;
        uint capacity;
    } EntityContainer;

    struct QueryContainer_T
    {
        EcsQuery* queries;
        uint count;
        uint capacity;
    } QueryContainer;

    struct QueryIteratorContainer_T
    {
        EcsQueryIterator* iterators;
        uint count;
        uint capacity;
    } QueryIteratorContainer;

} EcsInstance;

EcsEntity* ecsGetEntity(EcsInstance* instance, uint entityId);
EcsArchetype* ecsGetArchetype(EcsInstance* instance, uint archetypeId);
EcsArchetype* ecsGetArchetypeFromEntity(EcsInstance* instance, const EcsEntity* entity);
EcsArchetype* ecsGetArchetypeFromEntityId(EcsInstance* instance, uint entityId);

void* ecsGetComponentFromArchetype(const EcsArchetype* archetype, uint componentTypeId, uint componentIndex);
void* ecsGetComponentFromArchetypeId(EcsInstance* instance, uint archetypeId, uint componentTypeId, uint componentIndex);
void* ecsGetComponentFromEntityId(EcsInstance* instance, uint entityId, uint componentTypeId);
void ecsGetComponentsFromEntityId(EcsInstance* instance, EcsComponentsResult* dst, uint entityId);
void ecsGetComponentsFromEntityIdEx(EcsInstance* instance, EcsComponentsResultEx* dst, uint entityId);
EcsQuery* ecsGetQuery(EcsInstance* instance, uint queryId);

EcsInstance ecsCreateInstance();

/// @brief creates a query for iterating components in all applicable archetypes
/// @param componentCount: number of component args in query
/// @param ...: componentId args
/// @return queryId
uint ecsCreateQuery(EcsInstance* instance, uint componentCount, uint componentIds, ...);


/// @brief initialize iteration of a query.
/// ex. EcsIterator itr = ecsIterateQueryInit((uint)eMyQuery);
/// @param queryId: created with ecsCreateQuery
/// @return EcsIterator object for use with ecsIterateQuery
EcsQueryIterator ecsCreateQueryIterator(EcsInstance* instance, uint queryId);

/// @brief iterate all archetypes and components in query. 
/// ex: while( ecsIterateQuery(&itr, &coms) ) { coms.pA->x = 0; }
/// @param itr: address of EcsIterator object created from ecsIterateQueryInit
/// @param componentsArray: destination array of component pointers for the current iterated entity
/// @return EcsQueryIterator*: the valid iterator pointer, or NULL when the query has ended
EcsQueryIterator* ecsIterateQuery(EcsQueryIterator* itr, void** componentsArray);
EcsQueryIterator* ecsIterateQueryEx(EcsQueryIterator* itr, uint* entityId, void** componentsArray);

/// @brief iterate all entities that match query, executing callback per entity
/// @param queryId: created with ecsCreateQuery
/// @param callback function to execute per entity for components in query
void ecsIterateQueryCallback  (EcsInstance* instance, uint queryId, EcsQueryCallback   callback);
void ecsIterateQueryCallbackEx(EcsInstance* instance, uint queryId, EcsQueryCallbackEx callback);

/// @brief create an archetype, a collection of components which entities are assigned to
/// @param componentCount: number of component args
/// @param ...: componentId, sizeofComponent, ...
/// @return archetypeId
uint ecsCreateArchetype(EcsInstance* instance, uint componentCount, EcsComponentDesc* componentDescs, uint initialCapacity);
//uint ecsCreateArchetype2(EcsCreateArchetypeInfo* info);

/// @brief create an entity assigned to archetype
/// @param archetypeId: (see ecsCreateArchetype)
/// @return entityId
uint ecsCreateEntity(EcsInstance* instance, uint archetypeId);

void ecsDestroyEntity(EcsInstance* instance, uint entityId);

/// @brief add a component to entity - if new signiture, results in allocating new archetype and moving data
/// @param instance
/// @param entityId
/// @param componentId 
/// @param sizeofComponent 
void ecsAddComponentToEntity(EcsInstance* instance, uint entityId, uint componentId, size_t sizeofComponent);


// TODO -- below
void ecsRemoveComponentFromEntity(EcsInstance* instance, uint entityId, uint componentId);
uint ecsCreateEntities(uint archetypeId, uint count);
void ecsReserveArchetypeCapacity(uint newCapacity);
void ecsReserveEntityCapacity(uint newCapacity);
void ecsReserveArchetypeEntityCapacity(uint newCapacity);
void ecsReserveQueryCapacity(uint newCapacity);

#ifdef __cplusplus
}
#endif // __cplusplus
