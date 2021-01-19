#pragma once

#include <type_traits>
#include <stdint.h>
#include <unordered_map>
#include <list>
#include <forward_list>
#include "CCollections/CSegmentedArrayList.h"


namespace cyber
{
    /*      pool_allocator
    
        cap     linked list pool
        4       [4]->[4]->[4]
        8       [8]->[8]->[8]
        ...
    
    */

    //template<typename T> struct pool_allocator_node
    //{
    //    union
    //    {
    //        T item;
    //        Node* next_free;
    //    };
    //};


    // a simple pool allocator operating from a free list - strictly one allocation at a time permitted
    template<typename T> struct pool_allocator
    {
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;
        using is_always_equal = std::true_type;


        struct Node
        {
            union
            {
                T item;
                Node* next_free;
            };
        };

        static void* next_free;
        static size_t block_capacity;

        constexpr pool_allocator() noexcept {}
        constexpr pool_allocator(const pool_allocator&) noexcept = default;
        template <class U> constexpr pool_allocator(const pool_allocator<U>&) noexcept {}

        [[nodiscard]] constexpr T* allocate(std::size_t n)
        {
            assert(n == 1);

            if (next_free == nullptr)
            {
                next_free = malloc(sizeof(T) * block_capacity);

                // init free list
                Node* itr = reinterpret_cast<Node*>(next_free);
                Node* last = itr + (block_capacity - 1);
                while(itr != last) itr->next_free = ++itr;
                last->next_free = nullptr;
            }

            T* ret = reinterpret_cast<T*>(next_free);
            next_free = reinterpret_cast<void*>( reinterpret_cast<Node*>(next_free)->next_free );
            return ret;
        }

        constexpr void deallocate(T* p, std::size_t n)
        {
            //const std::size_t value_type_hash = typeid(T).hash_code();
            
            Node* new_free = reinterpret_cast<Node*>(p);
            new_free->next_free = reinterpret_cast<Node*>(next_free);
            next_free = reinterpret_cast<void*>(new_free);
        }

    };

    template<typename T> void* pool_allocator<T>::next_free = nullptr;
    template<typename T> size_t pool_allocator<T>::block_capacity = 8u;

    //template<> struct pool_allocator<void>
    //{
    //    using value_type = void;
    //    using size_type = size_t;
    //    using difference_type = ptrdiff_t;
    //    using propagate_on_container_move_assignment = std::true_type;
    //    using is_always_equal = std::true_type;
    //};

    // CLASS TEMPLATE allocator
    //template <class _Ty>
    //class PoolAllocator {
    //public:
    //    //static_assert(!is_const_v<_Ty>, "The C++ Standard forbids containers of const elements because allocator<const T> is ill-formed."); // type_traits

    //    using _From_primary = PoolAllocator;

    //    void deallocate(_Ty* const _Ptr, const size_t _Count) {
    //        // no overflow check on the following multiply; we assume _Allocate did that check
    //        _Deallocate<_New_alignof<_Ty>>(_Ptr, sizeof(_Ty) * _Count);
    //    }

    //    _NODISCARD _DECLSPEC_ALLOCATOR _Ty* allocate(_CRT_GUARDOVERFLOW const size_t _Count) {
    //        return static_cast<_Ty*>(_Allocate<_New_alignof<_Ty>>(_Get_size_of_n<sizeof(_Ty)>(_Count)));
    //    }


    //};

}