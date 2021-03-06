#include "y_ecs.hpp"
#include <cstdarg>
#include <cstdlib>  // for malloc() and friends
#include <cstring>  // strcmp()

//#include <Windows.h>  // for VirtualAlloc

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
void * g_page_alloc (size_t size) {
    //return ::VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return g_alloc(size);
}
void g_page_dealloc (void * ptr, size_t /*size*/) {
    //::VirtualFree(ptr, 0, MEM_RELEASE);
    g_dealloc(ptr, 0);
}
//======================================================================
//static SizeType EntityType_CalcSize (ComponentCount component_count) {
//    return SizeType(sizeof(EntityType) + component_count * sizeof(/**/ComponentCount/*/EntityType{}.component_seqnums[0]/**/));
//}
//----------------------------------------------------------------------
// Note(yzt): Returns the power, not the actual number, i.e. for 16384, it returns 14.
//static inline SizeType NextPowerOfTwo (SizeType x) {
//    SizeType ret = 0, v = 1;
//    while (v < x) {
//        v *= 2;
//        ret += 1;
//    }
//    return ret;
//}
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
static bool PagedArray_InitEmpty (World::PerEntityComponent * array) {
    *array = {};
    return true;
}
//----------------------------------------------------------------------
static bool PagedArray_InitReserve (World::PerEntityComponent * array, SizeType page_size, SizeType initial_pages) {
    if (initial_pages > 0) {
        array->page_ptrs = static_cast<Byte **>(g_alloc(initial_pages * sizeof(Byte *)));
        if (!array->page_ptrs)
            return false;
        for (SizeType i = 0; i < initial_pages; ++i) {
            array->page_ptrs[i] = static_cast<Byte *>(g_page_alloc(page_size));
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
static bool PagedArray_Clear (World::PerEntityComponent * array, SizeType page_size) {
    bool ret = false;
    if (array) {
        if (array->page_ptrs) {
            assert(array->page_array_size > 0);
            for (SizeType i = 0; i < array->page_array_size && array->page_ptrs[i]; ++i)
                g_page_dealloc(array->page_ptrs[i], page_size);
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
static bool PagedArray_IsClear (World::PerEntityComponent const * array) {  // NOTE(yzt): Different from IsEmpty()!
    bool ret = true;
    if (array) {
        if (array->page_ptrs) {
            assert(array->page_array_size > 0);
            ret = false;
        } else {
            assert(array->page_array_size == 0 && array->pages_allocated == 0 && array->pages_in_use == 0 && array->elements_in_last_page == 0);
        }
    }
    return ret;
}
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
bool TagType_CloseRegisteration (TypeManager * type_manager) {
    bool ret = false;
    if (type_manager && type_manager->initialized && !type_manager->tag_type_registration_closed) {
        type_manager->tag_type_registration_closed = true;
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagType_NameExists (TypeManager const * type_manager, char const * name) {
    return nullptr != TagType_FindByName(type_manager, name);
}
//----------------------------------------------------------------------
bool TagType_IsRegistrationClosed (TypeManager const * type_manager) {
    return !type_manager || !type_manager->initialized || type_manager->tag_type_registration_closed;
}
//----------------------------------------------------------------------
ComponentCount TagType_Count (TypeManager const * type_manager) {
    ComponentCount ret = 0;
    if (type_manager && type_manager->initialized)
        ret = type_manager->tag_type_count;
    return ret;
}
//----------------------------------------------------------------------
TagType const * TagType_GetFirst (TypeManager const * type_manager) {
    TagType const * ret = nullptr;
    if (type_manager && type_manager->initialized)
        ret = type_manager->tag_type_first;
    return ret;
}
//----------------------------------------------------------------------
TagType const * TagType_GetNext (TagType const * tag_type) {
    TagType const * ret = nullptr;
    if (tag_type)
        ret = tag_type->next;
    return ret;
}
//----------------------------------------------------------------------
TagType const * TagType_FindByName (TypeManager const * type_manager, char const * name) {
    TagType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->tag_type_first; ret; ret = ret->next)
            if (0 == ::strcmp(name, ret->name))
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
TagType const * TagType_FindBySeqNum (TypeManager const * type_manager, ComponentCount seqnum) {
    TagType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->tag_type_first; ret; ret = ret->next)
            if (seqnum == ret->seqnum)
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentBitSet_MakeEmpty (ComponentBitSet * out_components) {
    bool ret = false;
    if (out_components) {
        BitSet_ClearAll(out_components->bits);
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, ComponentCount component_seqnums [], ComponentCount component_count) {
    bool ret = false;
    if (out_components && type_manager) {
        BitSet_ClearAll(out_components->bits);
        ret = true;
        for (unsigned i = 0; i < component_count; ++i) {
            if (component_seqnums[i] < type_manager->component_type_count)
                BitSet_SetBit(out_components->bits, component_seqnums[i]);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, char const * component_names [], ComponentCount component_count) {
    bool ret = false;
    if (out_components && type_manager) {
        BitSet_ClearAll(out_components->bits);
        ret = true;
        for (unsigned i = 0; i < component_count; ++i) {
            ComponentType const * type = ComponentType_FindByName(type_manager, component_names[i]);
            if (type)
                BitSet_SetBit(out_components->bits, type->seqnum);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, std::initializer_list<ComponentCount> component_seqnums) {
    bool ret = false;
    if (out_components && type_manager) {
        BitSet_ClearAll(out_components->bits);
        ret = true;
        for (auto seqnum : component_seqnums) {
            if (seqnum < type_manager->component_type_count)
                BitSet_SetBit(out_components->bits, seqnum);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, std::initializer_list<char const *> component_names) {
    bool ret = false;
    if (out_components && type_manager) {
        BitSet_ClearAll(out_components->bits);
        ret = true;
        for (auto name : component_names) {
            ComponentType const * type = ComponentType_FindByName(type_manager, name);
            if (type)
                BitSet_SetBit(out_components->bits, type->seqnum);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentBitSet_MakeFromSeqnums (ComponentBitSet * out_components, TypeManager const * type_manager, ...) {
    bool ret = false;
    if (out_components && type_manager) {
        BitSet_ClearAll(out_components->bits);
        ret = true;
        va_list args;
        va_start(args, type_manager);
        for (;;) {
            int seqnum = va_arg(args, int); // FIXME(yzt): This is shit (because of the "int".) I probably have to remove this function altogether.
            if (seqnum < 0)
                break;
            if (seqnum < type_manager->component_type_count)
                BitSet_SetBit(out_components->bits, seqnum);
            else
                ret = false;
        }
        va_end(args);
    }
    return ret;
}
//----------------------------------------------------------------------
bool ComponentBitSet_MakeFromNames (ComponentBitSet * out_components, TypeManager const * type_manager, ...) {
    bool ret = false;
    if (out_components && type_manager) {
        BitSet_ClearAll(out_components->bits);
        ret = true;
        va_list args;
        va_start(args, type_manager);
        for (;;) {
            char const * name = va_arg(args, char const *);
            if (nullptr == name)
                break;
            ComponentType const * type = ComponentType_FindByName(type_manager, name);
            if (type)
                BitSet_SetBit(out_components->bits, type->seqnum);
            else
                ret = false;
        }
        va_end(args);
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagBitSet_MakeEmpty (TagBitSet * out_tags) {
    bool ret = false;
    if (out_tags) {
        BitSet_ClearAll(out_tags->bits);
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, ComponentCount tag_seqnums [], ComponentCount tag_count) {
    bool ret = false;
    if (out_tags && type_manager) {
        BitSet_ClearAll(out_tags->bits);
        ret = true;
        for (unsigned i = 0; i < tag_count; ++i) {
            if (tag_seqnums[i] < type_manager->tag_type_count)
                BitSet_SetBit(out_tags->bits, tag_seqnums[i]);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, char const * tag_names [], ComponentCount tag_count) {
    bool ret = false;
    if (out_tags && type_manager) {
        BitSet_ClearAll(out_tags->bits);
        ret = true;
        for (unsigned i = 0; i < tag_count; ++i) {
            TagType const * tag_type = TagType_FindByName(type_manager, tag_names[i]);
            if (tag_type)
                BitSet_SetBit(out_tags->bits, tag_type->seqnum);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, std::initializer_list<ComponentCount> tag_seqnums) {
    bool ret = false;
    if (out_tags && type_manager) {
        BitSet_ClearAll(out_tags->bits);
        ret = true;
        for (auto seqnum : tag_seqnums) {
            if (seqnum < type_manager->tag_type_count)
                BitSet_SetBit(out_tags->bits, seqnum);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, std::initializer_list<char const *> tag_names) {
    bool ret = false;
    if (out_tags && type_manager) {
        BitSet_ClearAll(out_tags->bits);
        ret = true;
        for (auto name : tag_names) {
            TagType const * type = TagType_FindByName(type_manager, name);
            if (type)
                BitSet_SetBit(out_tags->bits, type->seqnum);
            else
                ret = false;
        }
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagBitSet_MakeFromSeqnums (TagBitSet * out_tags, TypeManager const * type_manager, ...) {
    bool ret = false;
    if (out_tags && type_manager) {
        BitSet_ClearAll(out_tags->bits);
        ret = true;
        va_list args;
        va_start(args, type_manager);
        for (;;) {
            int seqnum = va_arg(args, int); // FIXME(yzt): This is shit (because of the "int".) I probably have to remove this function altogether.
            if (seqnum < 0)
                break;
            if (seqnum < type_manager->component_type_count)
                BitSet_SetBit(out_tags->bits, seqnum);
            else
                ret = false;
        }
        va_end(args);
    }
    return ret;
}
//----------------------------------------------------------------------
bool TagBitSet_MakeFromNames (TagBitSet * out_tags, TypeManager const * type_manager, ...) {
    bool ret = false;
    if (out_tags && type_manager) {
        BitSet_ClearAll(out_tags->bits);
        ret = true;
        va_list args;
        va_start(args, type_manager);
        for (;;) {
            char const * name = va_arg(args, char const *);
            if (nullptr == name)
                break;
            TagType const * type = TagType_FindByName(type_manager, name);
            if (type)
                BitSet_SetBit(out_tags->bits, type->seqnum);
            else
                ret = false;
        }
        va_end(args);
    }
    return ret;
}
//----------------------------------------------------------------------
EntityType * EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentBitSet components, TagBitSet tags) {
    EntityType * ret = nullptr;
    if (
        type_manager &&
        type_manager->initialized &&
        type_manager->component_type_registration_closed &&
        type_manager->tag_type_registration_closed &&
        !type_manager->entity_type_registration_closed &&
        !BitSet_Empty(components.bits) &&
        name &&
        ::strlen(name) > 0 &&
        ::strlen(name) <= MaxNameLen &&
        !EntityType_NameExists(type_manager, name) &&
        !EntityType_ComponentAndTagSetsExist(type_manager, components, tags)
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

        ret = entity_type;
    }
    return ret;
}
//----------------------------------------------------------------------
EntityType * EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, std::initializer_list<char const *> component_names, std::initializer_list<char const *> tag_names) {
    EntityType * ret = nullptr;
    ComponentBitSet components;
    if (ComponentBitSet_Make(&components, type_manager, component_names)) {
        TagBitSet tags;
        if (TagBitSet_Make(&tags, type_manager, tag_names)) {
            ret = EntityType_Register(type_manager, name, initial_capacity, components, tags);
        }
    }
    return ret;
}
//----------------------------------------------------------------------
//bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentCount component_seqnums [], unsigned component_count) {
//    if (!type_manager)
//        return false;
//    ComponentBitSet components;
//    BitSet_ClearAll(components.bits);
//    for (unsigned i = 0; i < component_count; ++i) {
//        if (component_seqnums[i] >= type_manager->component_type_count)
//            return false;
//        BitSet_SetBit(components.bits, component_seqnums[i]);
//    }
//    return EntityType_Register(type_manager, name, initial_capacity, components);
//}
////----------------------------------------------------------------------
//bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, char const * component_names [], unsigned component_count) {
//    ComponentBitSet components;
//    BitSet_ClearAll(components.bits);
//    for (unsigned i = 0; i < component_count; ++i) {
//        auto comp_type = ComponentType_FindByName(type_manager, component_names[i]);
//        if (!comp_type)
//            return false;
//        BitSet_SetBit(components.bits, comp_type->seqnum);
//    }
//    return EntityType_Register(type_manager, name, initial_capacity, components);
//}
////----------------------------------------------------------------------
//bool EntityType_Register_ByCompSeqnums (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...) {
//    if (!type_manager)
//        return false;
//    ComponentBitSet components;
//    BitSet_ClearAll(components.bits);
//    va_list args;
//    va_start(args, initial_capacity);
//    for (;;) {
//        int seqnum = va_arg(args, int); // FIXME(yzt): This is shit. I probably have to remove this function altogether.
//        if (seqnum < 0)
//            break;
//        if (seqnum >= type_manager->component_type_count) {
//            va_end(args);
//            return false;
//        }
//        BitSet_SetBit(components.bits, seqnum);
//    }
//    va_end(args);
//    return EntityType_Register(type_manager, name, initial_capacity, components);
//}
////----------------------------------------------------------------------
//bool EntityType_Register_ByCompNames (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...) {
//    ComponentBitSet components;
//    BitSet_ClearAll(components.bits);
//    va_list args;
//    va_start(args, initial_capacity);
//    for (;;) {
//        char const * component_name = va_arg(args, char const *);
//        if (nullptr == component_name)
//            break;
//        auto comp_type = ComponentType_FindByName(type_manager, component_name);
//        if (!comp_type) {
//            va_end(args);
//            return false;
//        }
//        BitSet_SetBit(components.bits, comp_type->seqnum);
//    }
//    va_end(args);
//    return EntityType_Register(type_manager, name, initial_capacity, components);
//}
//----------------------------------------------------------------------
bool EntityType_CloseRegisteration (TypeManager * type_manager) {
    bool ret = false;
    if (type_manager && type_manager->initialized && !type_manager->entity_type_registration_closed) {
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
bool EntityType_ComponentAndTagSetsExist (TypeManager const * type_manager, ComponentBitSet const & components, TagBitSet const & tags) {
    return nullptr != EntityType_FindByComponentAndTagSets(type_manager, components, tags);
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
EntityType const * EntityType_FindByComponentAndTagSets (TypeManager const * type_manager, ComponentBitSet const & components, TagBitSet const & tags) {
    EntityType const * ret = nullptr;
    if (type_manager && type_manager->initialized) {
        for (ret = type_manager->entity_type_first; ret; ret = ret->next)
            if (BitSet_Equals(components.bits, ret->components.bits) && BitSet_Equals(tags.bits, ret->tags.bits))
                break;
    }
    return ret;
}
//----------------------------------------------------------------------
bool World_Create (World * out_world, TypeManager const * type_manager, SizeType data_page_size, SizeType max_entity_types, ComponentCount max_component_types, ComponentCount max_tags) {
    bool ret = false;
    if (
        out_world &&
        type_manager &&
        type_manager->component_type_registration_closed &&
        type_manager->tag_type_registration_closed &&
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

        ComponentCount tag_count = type_manager->tag_type_count;
        ComponentCount comp_count = type_manager->component_type_count;
        SizeType entity_count = type_manager->entity_type_count;
        SizeType entity_comp_count = entity_count * comp_count;

        auto    tag_names_mem = static_cast<World::Name *>(g_alloc_zero(tag_count * sizeof(World::Name)));
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

        unsigned i = 0;

        out_world->tag_type_count = type_manager->tag_type_count;
        out_world->tag_type_names = tag_names_mem;
        i = 0;
        for (auto tt = type_manager->tag_type_first; tt; (tt = tt->next), ++i) {
            assert(i == tt->seqnum);
            StrCpy(out_world->tag_type_names[i], tt->name, sizeof(World::Name));
        }

        out_world->component_type_count = type_manager->component_type_count;
        out_world->component_type_names = comp_names_mem;
        out_world->component_types = comp_types_mem;
        i = 0;
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
            out_world->entity_types[i].tags = et->tags;
            out_world->entity_types[i].component_count = et->component_count;
            out_world->entity_types[i].tag_count = et->tag_count;
            out_world->entity_types[i].entity_count = 0;
        }

        auto et = type_manager->entity_type_first;
        i = 0;
        for (unsigned entity_type_idx = 0; entity_type_idx < out_world->entity_type_count; ++entity_type_idx, (et = et->next)) {
            assert(et);
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
        out_world->initialized = true;

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
        g_dealloc(world->entity_component_data, sizeof(World::PerEntityComponent) * world->component_type_count * world->entity_type_count);
        g_dealloc(world->entity_types, world->entity_type_count * sizeof(World::PerEntityType));
        g_dealloc(world->entity_type_names, world->entity_type_count * sizeof(World::Name));
        g_dealloc(world->component_types, world->component_type_count * sizeof(World::PerComponentType));
        g_dealloc(world->component_type_names, world->component_type_count * sizeof(World::Name));
        g_dealloc(world->tag_type_names, world->tag_type_count * sizeof(World::Name));
        *world = {};
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
WorldMemoryStats World_GatherMemoryStats (World const * world) {
    WorldMemoryStats ret = {};
    if (world) {
        auto add_to_overhead_and_total = [&](size_t x){ret.overhead_bytes += x; ret.total_bytes += x;};
        //auto add_to_used = [&](size_t x){ret.used_bytes += x;};
        //auto add_to_usable = [&](size_t x){ret.usable_bytes += x;};
        //auto add_to_unusable = [&](size_t x){ret.unusable_bytes += x;};

        ret.page_size_bytes = world->data_page_size;
        ret.total_component_groups = size_t(world->component_type_count) * world->entity_type_count;

        add_to_overhead_and_total(sizeof(*world));
        add_to_overhead_and_total(sizeof(World::Name) * world->tag_type_count);
        add_to_overhead_and_total(sizeof(World::PerComponentType) * world->component_type_count);
        add_to_overhead_and_total(sizeof(World::Name) * world->component_type_count);
        add_to_overhead_and_total(sizeof(World::PerEntityType) * world->entity_type_count);
        add_to_overhead_and_total(sizeof(World::Name) * world->entity_type_count);
        add_to_overhead_and_total(sizeof(World::PerEntityComponent) * world->component_type_count * world->entity_type_count);

        for (unsigned entity_type_idx = 0, i = 0; entity_type_idx < world->entity_type_count; ++entity_type_idx) {
            auto et = world->entity_types + entity_type_idx;
            for (unsigned comp_type_idx = 0; comp_type_idx < world->component_type_count; ++comp_type_idx, ++i) {
                auto ct = world->component_types + comp_type_idx;
                auto ecd = world->entity_component_data + i;
                if (BitSet_GetBit(et->components.bits, comp_type_idx)) {
                    ret.active_component_groups += 1;
                    add_to_overhead_and_total(ecd->page_array_size * sizeof(*ecd->page_ptrs));
                    ret.total_bytes += size_t(ecd->pages_allocated) * world->data_page_size;
                    ret.used_bytes += size_t(et->entity_count) * ct->size;
                    ret.usable_bytes += (size_t(ecd->pages_allocated) * ct->count_per_page - et->entity_count) * ct->size;
                    ret.unusable_bytes += size_t(ecd->pages_allocated) * (world->data_page_size - ct->count_per_page * ct->size);
                } else {
                    if (!PagedArray_IsClear(ecd))
                        ret.faulty_component_groups += 1;
                }
            }
        }

        //ret.total_bytes = ret.overhead_bytes + ret.used_bytes + ret.usable_bytes + ret.unusable_bytes;
        ret.valid = true;
    }
    return ret;
}
//----------------------------------------------------------------------
//======================================================================
}   // namespace Ex
}   // namespace y
//======================================================================
