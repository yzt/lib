#include "y_ecs.hpp"
#include <cstdarg>
#include <cstdlib>  // for malloc() and friends
#include <cstring>  // strcmp()
//======================================================================
namespace y {
namespace Ex {
//======================================================================
auto g_alloc = ::malloc;
auto g_alloc_zero = ::calloc;
auto g_realloc = ::realloc;
auto g_dealloc = ::free;
//======================================================================
//static SizeType EntityType_CalcSize (ComponentCount component_count) {
//    return SizeType(sizeof(EntityType) + component_count * sizeof(/**/ComponentCount/*/EntityType{}.component_seqnums[0]/**/));
//}
//----------------------------------------------------------------------
template <typename T, size_t N>
bool BitSet_Empty (T (&bits) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        if (bits[i] != 0)
            return false;
    return true;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
unsigned BitSet_CountOnes (T (&bits) [N]) {
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
void BitSet_SetBit (T (&bits) [N], unsigned idx) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    if (idx < N * 8 * sizeof(T)) {
        unsigned w = idx / (8 * sizeof(T));
        unsigned b = idx % (8 * sizeof(T));
        bits[w] |= T(1) << b;
    }
}
//----------------------------------------------------------------------
template <typename T, size_t N>
void BitSet_ClearBit (T (&bits) [N], unsigned idx) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    if (idx < N * 8 * sizeof(T)) {
        unsigned w = idx / (8 * sizeof(T));
        unsigned b = idx % (8 * sizeof(T));
        bits[w] &= ~(T(1) << b);
    }
}
//----------------------------------------------------------------------
template <typename T, size_t N>
void BitSet_ClearAll (T (&bits) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        bits[i] = 0;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
bool BitSet_Equals (T (&a) [N], T (&b) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        if (a[i] != b[i])
            return false;
    return true;
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
            g_dealloc(p);   // size is EntityType_CalcSize(p->component_count)
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
        auto entity_type = static_cast<EntityType *>(g_alloc_zero(1, sizeof(EntityType)));
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
//======================================================================
}   // namespace Ex
}   // namespace y
//======================================================================
