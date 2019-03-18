#include "y_ecs.hpp"
#include <cassert>
#include <cstdarg>
#include <cstdlib>  // for malloc() and friends
#include <cstring>  // strcmp()
//======================================================================
namespace y {
namespace Ex {
//======================================================================
auto g_alloc = ::malloc;
//auto g_alloc_zero = ::calloc;
auto g_realloc = ::realloc;
//auto g_dealloc = ::free;
void * g_alloc_zero (size_t size) {
    void * ret = g_alloc(size);
    if (ret)
        ::memset(ret, 0, size);
    return ret;
}
void g_dealloc (void * ptr, size_t /*size*/) {
    ::free(ptr);
}
//======================================================================
//static SizeType EntityType_CalcSize (ComponentCount component_count) {
//    return SizeType(sizeof(EntityType) + component_count * sizeof(/**/ComponentCount/*/EntityType{}.component_seqnums[0]/**/));
//}
//----------------------------------------------------------------------
// Note(yzt): Returns the power, not the actual number, i.e. for 16384, it returns 14.
static inline SizeType NextPowerOfTwo (SizeType x) {
    SizeType ret = 0, v = 1;
    while (v < x) {
        v *= 2;
        ret += 1;
    }
    return ret;
}
//----------------------------------------------------------------------
static inline SizeType StrCpy (char * dst, char const * src, SizeType dst_size) {
    SizeType ret = 0;
    if (dst && src && dst_size > 0) {
        while (ret < dst_size) {
            char ch = *src++;
            *dst++ = ch;
            ret += 1;
            if ('\0' == ch)
                break;
        }
        *(dst - 1) = '\0';
    }
    return ret;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline bool BitSet_Empty (T (&bits) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        if (bits[i] != 0)
            return false;
    return true;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline unsigned BitSet_CountOnes (T (&bits) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    unsigned ret = 0;
    for (size_t i = 0; i < N; ++i) {
        T mask = 1;
        for (unsigned b = 8 * sizeof(T); b != 0; --b, mask <<= 1)
            if (bits[i] & mask)
                ret += 1;
    }
    return ret;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline void BitSet_SetBit (T (&bits) [N], unsigned idx) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    if (idx < N * 8 * sizeof(T)) {
        unsigned w = idx / (8 * sizeof(T));
        unsigned b = idx % (8 * sizeof(T));
        bits[w] |= T(1) << b;
    }
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline void BitSet_ClearBit (T (&bits) [N], unsigned idx) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    if (idx < N * 8 * sizeof(T)) {
        unsigned w = idx / (8 * sizeof(T));
        unsigned b = idx % (8 * sizeof(T));
        bits[w] &= ~(T(1) << b);
    }
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline void BitSet_ClearAll (T (&bits) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        bits[i] = 0;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline bool BitSet_Equals (T (&a) [N], T (&b) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        if (a[i] != b[i])
            return false;
    return true;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline T BitSet_GetBit (T (&a) [N], unsigned idx) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    assert(idx < N * 8 * sizeof(T));
    unsigned w = idx / (8 * sizeof(T));
    unsigned b = idx % (8 * sizeof(T));
    return (bits[w] >> b) & 1;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
bool PagedArray_InitEmpty (World::PerEntityComponent * array) {
    *array = {};
    return true;
}
//----------------------------------------------------------------------
bool PagedArray_InitReserve (World::PerEntityComponent * array, SizeType page_size, SizeType initial_pages) {
    if (initial_pages > 0) {
        array->page_ptrs = static_cast<Byte **>(g_alloc(initial_pages * sizeof(Byte *)));
        if (!array->page_ptrs)
            return false;
        for (SizeType i = 0; i < initial_pages; ++i) {
            array->page_ptrs[i] = static_cast<Byte *>(g_alloc_zero(page_size));
            if (!array->page_ptrs[i])
                return false;
        }
        array->page_array_size = initial_pages;
        array->pages_allocated = initial_pages;
        array->pages_in_use = 0;
        array->elements_in_last_page = 0;
        return true;
    } else {
        return PagedArray_InitEmpty(array);
    }
}
//----------------------------------------------------------------------
bool PagedArray_Clear (World::PerEntityComponent * array, SizeType page_size) {
    bool ret = false;
    if (array) {
        if (array->page_ptrs) {
            assert(array->page_array_size > 0);
            for (SizeType i = 0; i < array->page_array_size && array->page_ptrs[i]; ++i)
                g_dealloc(array->page_ptrs[i], page_size);
            g_dealloc(array->page_ptrs, array->page_array_size * sizeof(Byte *));
            PagedArray_InitEmpty(array);
        } else {
            assert(
                0 == array->page_array_size &&
                0 == array->pages_allocated &&
                0 == array->pages_in_use &&
                0 == array->elements_in_last_page
            );
        }
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
//======================================================================
bool TypeManager_Create (TypeManager * out_type_manager) {
    bool ret = false;
    if (out_type_manager) {
        *out_type_manager = {};
        out_type_manager->initialized = true;
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
void TypeManager_Destroy (TypeManager * type_manager) {
    if (type_manager && type_manager->initialized) {
        auto p = type_manager->entity_type_first;
        while (p) {
            auto q = p->next;
            g_dealloc(p, sizeof(EntityType));   // size is EntityType_CalcSize(p->component_count)
            p = q;
        }
        *type_manager = {};
    }
}
//----------------------------------------------------------------------
bool ComponentType_CloseRegisteration (TypeManager * type_manager) {
    bool ret = false;
    if (type_manager && type_manager->initialized && !type_manager->component_type_registration_closed) {
        type_manager->component_type_registration_closed = true;
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentType_NameExists (TypeManager const * type_manager, char const * name) {
    return nullptr != ComponentType_FindByName(type_manager, name);
}
//----------------------------------------------------------------------
bool ComponentType_IsRegistrationClosed (TypeManager const * type_manager) {
    return !type_manager || !type_manager->initialized || type_manager->component_type_registration_closed;
}
//----------------------------------------------------------------------
ComponentCount ComponentType_Count (TypeManager const * type_manager) {
    ComponentCount ret = 0;
    if (type_manager && type_manager->initialized)
        ret = type_manager->component_type_count;
    return ret;
}
//----------------------------------------------------------------------
ComponentType const * ComponentType_GetFirst (TypeManager const * type_manager) {
    ComponentType const * ret = nullptr;
    if (type_manager && type_manager->initialized)
        ret = type_manager->component_type_first;
    return ret;
}
//----------------------------------------------------------------------
ComponentType const * ComponentType_GetNext (ComponentType const * component_type) {
    ComponentType const * ret = nullptr;
    if (component_type)
        ret = component_type->next;
    return ret;
}
//----------------------------------------------------------------------
ComponentType const * ComponentType_FindByName (TypeManager const * type_manager, char const * name) {
    ComponentType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->component_type_first; ret; ret = ret->next)
            if (0 == ::strcmp(name, ret->name))
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
ComponentType const * ComponentType_FindBySeqNum (TypeManager const * type_manager, ComponentCount seqnum) {
    ComponentType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->component_type_first; ret; ret = ret->next)
            if (seqnum == ret->seqnum)
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentBitSet components) {
    bool ret = false;
    if (
        type_manager &&
        type_manager->initialized &&
        type_manager->component_type_registration_closed &&
        !type_manager->entity_type_registration_closed &&
        !BitSet_Empty(components.bits) &&
        name &&
        ::strlen(name) > 0 &&
        ::strlen(name) <= MaxNameLen &&
        !EntityType_NameExists(type_manager, name) &&
        !EntityType_ComponentSetExists(type_manager, components)
    ) {
        auto entity_type = static_cast<EntityType *>(g_alloc_zero(sizeof(EntityType)));
        entity_type->seqnum = type_manager->entity_type_count;
        type_manager->entity_type_count += 1;
        entity_type->initial_capacity = initial_capacity;
        entity_type->component_count = static_cast<ComponentCount>(BitSet_CountOnes(components.bits));
        entity_type->components = components;
        entity_type->owner = type_manager;

        unsigned i = 0;
        for (; i < sizeof(entity_type->name) - 1 && name[i]; ++i)
            entity_type->name[i] = name[i];
        entity_type->name[i] = '\0';

        if (type_manager->entity_type_last)
            type_manager->entity_type_last->next = entity_type;
        type_manager->entity_type_last = entity_type;
        if (!type_manager->entity_type_first)
            type_manager->entity_type_first = entity_type;
        entity_type->next = nullptr;
        //entity_type->registered = true;

        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentCount component_seqnums [], unsigned component_count) {
    if (!type_manager)
        return false;
    ComponentBitSet components;
    BitSet_ClearAll(components.bits);
    for (unsigned i = 0; i < component_count; ++i) {
        if (component_seqnums[i] >= type_manager->component_type_count)
            return false;
        BitSet_SetBit(components.bits, component_seqnums[i]);
    }
    return EntityType_Register(type_manager, name, initial_capacity, components);
}
//----------------------------------------------------------------------
bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, char const * component_names [], unsigned component_count) {
    ComponentBitSet components;
    BitSet_ClearAll(components.bits);
    for (unsigned i = 0; i < component_count; ++i) {
        auto comp_type = ComponentType_FindByName(type_manager, component_names[i]);
        if (!comp_type)
            return false;
        BitSet_SetBit(components.bits, comp_type->seqnum);
    }
    return EntityType_Register(type_manager, name, initial_capacity, components);
}
//----------------------------------------------------------------------
bool EntityType_Register_ByCompSeqnums (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...) {
    if (!type_manager)
        return false;
    ComponentBitSet components;
    BitSet_ClearAll(components.bits);
    va_list args;
    va_start(args, initial_capacity);
    for (;;) {
        int seqnum = va_arg(args, int); // FIXME(yzt): This is shit. I probably have to remove this function altogether.
        if (seqnum < 0)
            break;
        if (seqnum >= type_manager->component_type_count) {
            va_end(args);
            return false;
        }
        BitSet_SetBit(components.bits, seqnum);
    }
    va_end(args);
    return EntityType_Register(type_manager, name, initial_capacity, components);
}
//----------------------------------------------------------------------
bool EntityType_Register_ByCompNames (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...) {
    ComponentBitSet components;
    BitSet_ClearAll(components.bits);
    va_list args;
    va_start(args, initial_capacity);
    for (;;) {
        char const * component_name = va_arg(args, char const *);
        if (nullptr == component_name)
            break;
        auto comp_type = ComponentType_FindByName(type_manager, component_name);
        if (!comp_type) {
            va_end(args);
            return false;
        }
        BitSet_SetBit(components.bits, comp_type->seqnum);
    }
    va_end(args);
    return EntityType_Register(type_manager, name, initial_capacity, components);
}
//----------------------------------------------------------------------
bool EntityType_CloseRegisteration (TypeManager * type_manager) {
    bool ret = false;
    if (type_manager && type_manager->initialized && !type_manager->component_type_registration_closed) {
        type_manager->entity_type_registration_closed = true;
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
bool EntityType_NameExists (TypeManager const * type_manager, char const * name) {
    return nullptr != EntityType_FindByName(type_manager, name);
}
//----------------------------------------------------------------------
bool EntityType_ComponentSetExists (TypeManager const * type_manager, ComponentBitSet const & components) {
    return nullptr != EntityType_FindByComponentSet(type_manager, components);
}
//----------------------------------------------------------------------
bool EntityType_IsRegistrationClosed (TypeManager const * type_manager) {
    return !type_manager || !type_manager->initialized || type_manager->entity_type_registration_closed;
}
//----------------------------------------------------------------------
SizeType EntityType_Count (TypeManager const * type_manager) {
    SizeType ret = 0;
    if (type_manager && type_manager->initialized)
        ret = type_manager->entity_type_count;
    return ret;
}
//----------------------------------------------------------------------
EntityType const * EntityType_GetFirst (TypeManager const * type_manager) {
    EntityType const * ret = nullptr;
    if (type_manager && type_manager->initialized)
        ret = type_manager->entity_type_first;
    return ret;
}
//----------------------------------------------------------------------
EntityType const * EntityType_GetNext (EntityType const * entity_type) {
    EntityType const * ret = nullptr;
    if (entity_type)
        ret = entity_type->next;
    return ret;
}
//----------------------------------------------------------------------
EntityType const * EntityType_FindByName (TypeManager const * type_manager, char const * name) {
    EntityType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->entity_type_first; ret; ret = ret->next)
            if (0 == ::strcmp(name, ret->name))
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
EntityType const * EntityType_FindBySeqNum (TypeManager const * type_manager, SizeType seqnum) {
    EntityType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->entity_type_first; ret; ret = ret->next)
            if (seqnum == ret->seqnum)
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
EntityType const * EntityType_FindByComponentSet (TypeManager const * type_manager, ComponentBitSet const & components) {
    EntityType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->entity_type_first; ret; ret = ret->next)
            if (BitSet_Equals(components.bits, ret->components.bits))
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
bool World_Create (World * out_world, TypeManager * type_manager, SizeType data_page_size) {
    bool ret = false;
    if (
        out_world &&
        type_manager &&
        type_manager->component_type_registration_closed &&
        type_manager->entity_type_registration_closed &&
        type_manager->component_type_count > 0 &&
        type_manager->entity_type_count > 0
    ) {
        SizeType max_component_size = 0;
        for (auto ct = ComponentType_GetFirst(type_manager); ct; ct = ComponentType_GetNext(ct))
            if (ct->size > max_component_size)
                max_component_size = ct->size;
        if (data_page_size < 4 * max_component_size)
            data_page_size = 4 * max_component_size;
        //SizeType page_size_shift = NextPowerOfTwo(data_page_size);
        //data_page_size = (SizeType(1) << page_size_shift);

        ComponentCount comp_count = type_manager->component_type_count;
        SizeType entity_count = type_manager->entity_type_count;
        SizeType entity_comp_count = entity_count * comp_count;

        auto   comp_names_mem = static_cast<World::Name *>(g_alloc_zero(comp_count * sizeof(World::Name)));
        auto   comp_types_mem = static_cast<World::PerComponentType *>(g_alloc_zero(comp_count * sizeof(World::PerComponentType)));
        auto entity_names_mem = static_cast<World::Name *>(g_alloc_zero(entity_count * sizeof(World::Name)));
        auto entity_types_mem = static_cast<World::PerEntityType *>(g_alloc_zero(entity_count * sizeof(World::PerEntityType)));
        auto entity_comps_mem = static_cast<World::PerEntityComponent *>(g_alloc_zero(entity_comp_count * sizeof(World::PerEntityComponent)));
        assert(comp_names_mem && comp_types_mem && entity_names_mem && entity_types_mem && entity_comps_mem);

        *out_world = {};
        out_world->type_manager = type_manager;
        out_world->data_page_size = data_page_size; //SizeType(1) << page_size_shift;
        //out_world->data_page_shift = page_size_shift;
        //out_world->data_page_index_mask = data_page_size - 1;
        out_world->entity_component_data = entity_comps_mem;

        out_world->component_type_count = type_manager->component_type_count;
        out_world->component_type_names = comp_names_mem;
        out_world->component_types = comp_types_mem;
        unsigned i = 0;
        for (auto ct = type_manager->component_type_first; ct; (ct = ct->next), ++i) {
            assert(i == ct->seqnum);
            StrCpy(out_world->component_type_names[i], ct->name, sizeof(World::Name));
            out_world->component_types[i].size = ct->size;
            out_world->component_types[i].count_per_page = data_page_size / ct->size;
        }

        out_world->entity_type_count = type_manager->entity_type_count;
        out_world->entity_type_names = entity_names_mem;
        out_world->entity_types = entity_types_mem;
        i = 0;
        for (auto et = type_manager->entity_type_first; et; (et = et->next), ++i) {
            assert(i == et->seqnum);
            StrCpy(out_world->entity_type_names[i], et->name, sizeof(World::Name));
            out_world->entity_types[i].components = et->components;
            out_world->entity_types[i].component_count = et->component_count;
            out_world->entity_types[i].entity_count = 0;
        }

        auto et = type_manager->entity_type_first;
        for (unsigned entity_type_idx = 0, i = 0; entity_type_idx < out_world->entity_type_count; ++entity_type_idx, (et = et->next)) {
            for (unsigned comp_type_idx = 0; comp_type_idx < out_world->component_type_count; ++comp_type_idx, ++i) {
                if (BitSet_GetBit(out_world->entity_types[entity_type_idx].components.bits, comp_type_idx)) {
                    auto comps_per_page = out_world->component_types[comp_type_idx].count_per_page;
                    PagedArray_InitReserve(
                        out_world->entity_component_data + i,
                        out_world->data_page_size,
                        (et->initial_capacity + comps_per_page - 1) / comps_per_page
                    );
                }
            }
        }

        ret = true;
    }

    return ret;
}
//----------------------------------------------------------------------
bool World_Destroy (World * world) {
    bool ret = false;
    if (world) {
        for (SizeType i = 0, n = world->component_type_count * world->entity_type_count; i < n; ++i)
            PagedArray_Clear(world->entity_component_data + i, world->data_page_size);
        g_dealloc(world->entity_component_data, world->component_type_count * world->entity_type_count * sizeof(World::PerEntityComponent));
        g_dealloc(world->entity_types, world->entity_type_count * sizeof(World::PerEntityType));
        g_dealloc(world->entity_type_names, world->entity_type_count * sizeof(World::Name));
        g_dealloc(world->component_types, world->component_type_count * sizeof(World::PerComponentType));
        g_dealloc(world->component_type_names, world->component_type_count * sizeof(World::Name));
        *world = {};
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
bool World_GatherMemoryStats (World const * world, size_t * out_total_bytes, size_t * out_overhead_bytes, size_t * out_used_bytes, size_t * out_usable_bytes, size_t * out_unusable_bytes) {
    bool ret = false;
    if (world) {
        size_t total = 0, overhead = 0, used = 0, usable = 0, unusable = 0;
        auto add_to_overhead = [&](size_t x){overhead += x; total += x;};
        auto add_to_used = [&](size_t x){used += x; total += x;};
        auto add_to_usable = [&](size_t x){usable += x; total += x;};
        auto add_to_unusable = [&](size_t x){unusable += x; total += x;};

        add_to_overhead(sizeof(*world));
        add_to_overhead(world->component_type_count * sizeof(World::PerComponentType));
        add_to_overhead(world->component_type_count * sizeof(World::Name));
        add_to_overhead(world->entity_type_count * sizeof(World::PerEntityType));
        add_to_overhead(world->entity_type_count * sizeof(World::Name));
        add_to_overhead(world->component_type_count * world->entity_type_count * sizeof(World::PerEntityComponent));

        //......
    }
    return ret;
}
//----------------------------------------------------------------------
//======================================================================
}   // namespace Ex
}   // namespace y
//======================================================================
