#pragma once

#include <cstddef>      // for size_t
#include <cstdint>
#include <type_traits>  // for checks in TypeManager_RegisterComponentType<T>

namespace y {

using Byte = uint8_t;

}   // namespace y

namespace y {
namespace Ex {

using EntityID = uint32_t;
using ComponentCount = uint16_t;
using SizeType = uint32_t;

constexpr SizeType MaxNameLen = 63;
constexpr ComponentCount MaxComponentsPerEntityType = 128;

struct TypeManager;

struct ComponentBitSet {
    using Word = uint64_t;
    static constexpr SizeType Count = (MaxComponentsPerEntityType + 8 * sizeof(Word) - 1) / (8 * sizeof(Word));

    Word bits [Count];
};

struct ComponentType {
    bool registered = false;
    ComponentCount seqnum = ComponentCount(~0);
    SizeType size = 0;

    TypeManager * owner = nullptr;
    char name [MaxNameLen + 1] = {0};
    ComponentType * next = nullptr;
    //ComponentType * prev = nullptr;
};

struct EntityType {
    SizeType seqnum = SizeType(~SizeType(0));
    SizeType initial_capacity = 0;
    ComponentCount component_count = 0;
    ComponentBitSet components;
    
    TypeManager * owner = nullptr;
    char name [MaxNameLen + 1] = {0};
    EntityType * next = nullptr;
    //EntityType * prev = nullptr;
};

// Note(yzt): You probably need exactly one of these at a time (singleton.)
struct TypeManager {
    bool initialized;

    bool component_type_registration_closed;
    ComponentCount component_type_count;
    ComponentType * component_type_first;
    ComponentType * component_type_last;

    bool entity_type_registration_closed;
    SizeType entity_type_count;
    EntityType * entity_type_first;
    EntityType * entity_type_last;
};

struct World {
    using Name = char [MaxNameLen + 1];

    bool initialized;
    TypeManager * type_manager;
    SizeType data_page_size;    // Note(yzt): Treat as constant, if you value your sanity! Also, set to a power of two (e.g. 16K.)
    //SizeType data_page_shift;
    //SizeType data_page_index_mask;
    
    SizeType total_entity_count;

    ComponentCount component_type_count;
    Name * component_type_names;
    struct PerComponentType {
        SizeType size;
        SizeType count_per_page;
    } * component_types;

    SizeType entity_type_count;
    Name * entity_type_names;
    struct PerEntityType {
        ComponentBitSet components;
        ComponentCount count;
    } * entity_types;

    struct PerEntityComponent {
        Byte ** page_ptrs;
        /** IDIOT **/ //SizeType page_calc_shift;   // If page size is e.g. 16384, this will be 14
        /** IDIOT **/ //SizeType page_calc_mask;    // If page size is e.g. 16384, this will be 0x00003FFF
        SizeType page_array_size;
        SizeType pages_allocated;   // FIXME(yzt): Do we need this? The pointers being nullptr might be enough.
        SizeType pages_in_use;  // FIXME(yzt): Should I change this to something like "full_pages"?
        SizeType elements_in_last_page;
    } * entity_component_data;
};

// Note(yzt): Component types (e.g. MyComponent) must inherit from 
//      Ex::ComponentBase<MyComponent>
// Note(yzt): Also, you must declare a "static char const * GetComponentName()"
//      public function that returns a unique name (you probably can/should
//      use either the qualified, or unqualified type name.)
//      In practice, writing it like this is best:
//          static char const * GetComponentName () {return "MyComponent";}
template <typename T>
struct ComponentBase {
public:
    static ComponentType const & GetComponentType () {
        return s_component_type;
    }
private:
    template <typename T>
    friend bool ComponentType_Register (TypeManager *);
    
    static ComponentType s_component_type;
};
template <typename T>
ComponentType ComponentBase<T>::s_component_type;

//----------------------------------------------------------------------
// Functions:
//----------------------------------------------------------------------

bool TypeManager_Create (TypeManager * out_type_manager);
void TypeManager_Destroy (TypeManager * type_manager);

template <typename T>
bool ComponentType_Register (TypeManager * type_manager);
bool ComponentType_CloseRegisteration (TypeManager * type_manager);
bool ComponentType_NameExists (TypeManager const * type_manager, char const * name);
bool ComponentType_IsRegistrationClosed (TypeManager const * type_manager);
ComponentCount ComponentType_Count (TypeManager const * type_manager);
ComponentType const * ComponentType_GetFirst (TypeManager const * type_manager);
ComponentType const * ComponentType_GetNext (ComponentType const * component_type);
ComponentType const * ComponentType_FindByName (TypeManager const * type_manager, char const * name);
ComponentType const * ComponentType_FindBySeqNum (TypeManager const * type_manager, ComponentCount seqnum);

bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentBitSet components);   // This is the main one, but you should use one of the other, more convenient functions.
bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentCount component_seqnums [], unsigned component_count);
bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, char const * component_names [], unsigned component_count);
bool EntityType_Register_ByCompSeqnums (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...); // End with -1
bool EntityType_Register_ByCompNames (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...); // end with nullptr
bool EntityType_CloseRegisteration (TypeManager * type_manager);
bool EntityType_NameExists (TypeManager const * type_manager, char const * name);
bool EntityType_ComponentSetExists (TypeManager const * type_manager, ComponentBitSet const & components);
bool EntityType_IsRegistrationClosed (TypeManager const * type_manager);
SizeType EntityType_Count (TypeManager const * type_manager);
EntityType const * EntityType_GetFirst (TypeManager const * type_manager);
EntityType const * EntityType_GetNext (EntityType const * entity_type);
EntityType const * EntityType_FindByName (TypeManager const * type_manager, char const * name);
EntityType const * EntityType_FindBySeqNum (TypeManager const * type_manager, SizeType seqnum);
EntityType const * EntityType_FindByComponentSet (TypeManager const * type_manager, ComponentBitSet const & components);

bool World_Create (World * out_world, TypeManager const * type_manager, SizeType data_page_size);

//----------------------------------------------------------------------

}   // namespace Ex
}   // namespace y

//======================================================================
// Implementations:
//----------------------------------------------------------------------

namespace y {
namespace Ex {

//----------------------------------------------------------------------
template <typename T>
bool ComponentType_Register (TypeManager * type_manager) {
    static_assert(std::is_pod_v<T>, "A component type should be a POD.");
    static_assert(std::is_trivially_default_constructible_v<T>, "A component type should be a trivially default-constructible.");
    static_assert(std::is_base_of_v<ComponentBase<T>, T>, "A component type should inherit from Ex::ComponentBase<T>.");
    //static_assert(std::is_same_v<std::invoke_result_t<T::GetComponentName>, char const *>, "A component type should have a \"static char const * GetComponentName()\" method.");

    bool ret = false;

    char const * const & name = T::GetComponentName();
    SizeType name_len = 0;
    if (name) {for (auto p = name; *p; ++p, ++name_len);}

    ComponentType * comp_type = &T::s_component_type;
    if (
        type_manager &&
        type_manager->initialized &&
        !type_manager->component_type_registration_closed &&
        !comp_type->registered &&
        !comp_type->owner &&
        !comp_type->next &&
        //!comp_type->prev &&
        nullptr != name &&
        name_len > 0 &&
        name_len <= MaxNameLen &&
        !ComponentType_NameExists(type_manager, name)
    ) {
        comp_type->seqnum = type_manager->component_type_count;
        type_manager->component_type_count += 1;
        comp_type->size = SizeType(sizeof(T));
        comp_type->owner = type_manager;

        unsigned i = 0;
        for (; i < sizeof(comp_type->name) - 1 && name[i]; ++i)
            comp_type->name[i] = name[i];
        comp_type->name[i] = '\0';

        if (type_manager->component_type_last)
            type_manager->component_type_last->next = comp_type;
        type_manager->component_type_last = comp_type;
        if (!type_manager->component_type_first)
            type_manager->component_type_first = comp_type;
        comp_type->next = nullptr;
        comp_type->registered = true;

        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------

}   // namespace Ex
}   // namespace y

//======================================================================
