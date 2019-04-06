#pragma once

#include <cstddef>      // for size_t
#include <cstdint>
#include <initializer_list>
#include <tuple>
#include <type_traits>  // for checks in TypeManager_RegisterComponentType<T>

namespace y {

using Byte = uint8_t;

template <unsigned BitCount>
struct BitSet {
    using Word = uint64_t;
    static constexpr unsigned Count = (BitCount + 8 * sizeof(Word) - 1) / (8 * sizeof(Word));

    Word bits [Count] = {0};
};

#pragma region BitSet Implementation
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
static inline T BitSet_GetBit (T (&bits) [N], unsigned idx) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    //assert(idx < N * 8 * sizeof(T));
    unsigned w = idx / (8 * sizeof(T));
    unsigned b = idx % (8 * sizeof(T));
    return (bits[w] >> b) & 1;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline unsigned BitSet_FindOne (T (&bits) [N], unsigned start_idx) {
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
static inline bool BitSet_ContainsAll (T const (&super) [N], T const (&sub) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        if ((super[i] & sub[i]) != sub[i])
            return false;
    return true;
}
//----------------------------------------------------------------------
template <typename T, size_t N>
static inline bool BitSet_ContainsAny (T const (&super) [N], T const (&sub) [N]) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && N > 0);
    for (size_t i = 0; i < N; ++i)
        if ((super[i] & sub[i]) != 0)
            return true;
    return false;
}
//----------------------------------------------------------------------
#pragma endregion

}   // namespace y


// TODO(yzt): Add the concept of "tags".
// TODO(yzt): Add a "roster" or "directory" that contains all entities to the "World" (a.k.a. EntityManager functionality)
// TODO(yzt): Add a series of default component types and tag types, and probably auto register them when creating the TypeManager (e.g. a MyEntityID component, or "Active", "Dynamic", "Prefab", "ReadOnly" tags.) That MyEntityID component probably should be added to all entity types anyways.

namespace y {
namespace Ex {

//using EntityID = uint32_t;
struct EntityID {
    uint32_t generation : 8;
    uint32_t index : 24;
};
static_assert(sizeof(EntityID) == 4);

using ComponentCount = uint16_t;
using SizeType = uint32_t;

constexpr SizeType MaxNameLen = 63;
constexpr ComponentCount MaxComponentTypes = 128;
constexpr ComponentCount MaxTagTypes = 64;
constexpr SizeType MaxEntityTypes = 1'000;

struct TypeManager;

using ComponentBitSet = BitSet<MaxComponentTypes>;
using TagBitSet = BitSet<MaxTagTypes>;
using EntityTypeBitSet = BitSet<MaxEntityTypes>;

struct ComponentType {
    bool registered = false;
    ComponentCount seqnum = ComponentCount(~0);
    SizeType size = 0;
    TypeManager * owner = nullptr;
    char name [MaxNameLen + 1] = {0};
    ComponentType * next = nullptr;
    //ComponentType * prev = nullptr;
};

struct TagType {
    bool registered = false;
    ComponentCount seqnum = ComponentCount(~0);
    TypeManager * owner = nullptr;
    TagType * next = nullptr;
    //TagType * prev = nullptr;
    char name [MaxNameLen + 1] = {0};
};

struct EntityType {
    SizeType seqnum = SizeType(~SizeType(0));
    SizeType initial_capacity = 0;
    ComponentCount component_count = 0;
    ComponentCount tag_count = 0;
    ComponentBitSet components;
    TagBitSet tags;
    TypeManager * owner = nullptr;
    EntityType * next = nullptr;
    //EntityType * prev = nullptr;
    char name [MaxNameLen + 1] = {0};
};

// Note(yzt): You probably need exactly one of these at a time (singleton.)
struct TypeManager {
    bool initialized;

    bool component_type_registration_closed;
    ComponentCount component_type_count;
    ComponentType * component_type_first;
    ComponentType * component_type_last;

    bool tag_type_registration_closed;
    ComponentCount tag_type_count;
    TagType * tag_type_first;
    TagType * tag_type_last;

    bool entity_type_registration_closed;
    SizeType entity_type_count;
    EntityType * entity_type_first;
    EntityType * entity_type_last;
};

struct World {
    using Name = char [MaxNameLen + 1];

    bool initialized;
    TypeManager const * type_manager;
    SizeType data_page_size;    // Note(yzt): Treat as constant, if you value your sanity! Also, set to a power of two (e.g. 16K.)
    //SizeType data_page_shift;
    //SizeType data_page_index_mask;
    
    SizeType total_entity_count;

    ComponentCount tag_type_count;

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
        TagBitSet tags;
        ComponentCount component_count;
        ComponentCount tag_count;
        SizeType entity_count;
    } * entity_types;

    struct PerEntityComponent {
        Byte ** page_ptrs;
        /** IDIOT **/ //SizeType page_calc_shift;   // If page size is e.g. 16384, this will be 14
        /** IDIOT **/ //SizeType page_calc_mask;    // If page size is e.g. 16384, this will be 0x00003FFF
        SizeType page_array_size;
        SizeType pages_allocated;   // FIXME(yzt): Do we need this? The pointers being nullptr might be enough.
        SizeType pages_in_use;      // FIXME(yzt): Should I change this to something like "full_pages"?
        SizeType elements_in_last_page;
    } * entity_component_data;      // NOTE(yzt): The ComponentTypes of each EntityType are laid together, i.e. element 0 is the 1st component type of the 1st entity type, element 1 is the 2nd component type of the 1st entity type, etc.
};

//---------------------------------------------------------------------

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
    static ComponentType const & GetTypeInfo () {return s_component_type;}
private:
    static ComponentType s_component_type;
    template <typename>
    friend bool ComponentType_Register (TypeManager *);
};
template <typename T>
ComponentType ComponentBase<T>::s_component_type;

template <typename T>
struct TagBase {
public:
    static TagType const & GetTypeInfo () {return s_tag_type;}
private:
    static TagType s_tag_type;
    template <typename>
    friend bool TagType_Register (TypeManager *);
};

struct WorldMemoryStats {
    bool valid;
    
    size_t page_size_bytes;
    size_t total_component_groups;
    size_t active_component_groups;
    size_t faulty_component_groups;

    size_t total_bytes;
    size_t overhead_bytes;
    size_t used_bytes;
    size_t usable_bytes;
    size_t unusable_bytes;
};

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

template <typename T>
bool TagType_Register (TypeManager * type_manager);
bool TagType_CloseRegisteration (TypeManager * type_manager);
bool TagType_NameExists (TypeManager const * type_manager, char const * name);
bool TagType_IsRegistrationClosed (TypeManager const * type_manager);
ComponentCount TagType_Count (TypeManager const * type_manager);
TagType const * TagType_GetFirst (TypeManager const * type_manager);
TagType const * TagType_GetNext (TagType const * tag_type);
TagType const * TagType_FindByName (TypeManager const * type_manager, char const * name);
TagType const * TagType_FindBySeqNum (TypeManager const * type_manager, ComponentCount seqnum);

bool ComponentBitSet_MakeEmpty (ComponentBitSet * out_components);
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, ComponentCount component_seqnums [], ComponentCount component_count);
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, char const * component_names [], ComponentCount component_count);
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, std::initializer_list<ComponentCount> component_seqnums);
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager, std::initializer_list<char const *> component_names);
//bool ComponentBitSet_MakeFromSeqnums (ComponentBitSet * out_components, TypeManager const * type_manager, ...); // NOTE(yzt): End the list of (int) seqnums with -1 (or any negative integer.)
//bool ComponentBitSet_MakeFromNames (ComponentBitSet * out_components, TypeManager const * type_manager, ...);   // NOTE(yzt): End the list of (char const *) names with nullptr.
template <typename PackOfComponentTypes>
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager);

bool TagBitSet_MakeEmpty (TagBitSet * out_tags);
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, ComponentCount tag_seqnums [], ComponentCount tag_count);
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, char const * tag_names [], ComponentCount tag_count);
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, std::initializer_list<ComponentCount> tag_seqnums);
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager, std::initializer_list<char const *> tag_names);
//bool TagBitSet_MakeFromSeqnums (TagBitSet * out_tags, TypeManager const * type_manager, ...);   // NOTE(yzt): End the list of (int) seqnums with -1 (or any negative integer.)
//bool TagBitSet_MakeFromNames (TagBitSet * out_tags, TypeManager const * type_manager, ...);     // NOTE(yzt): End the list of (char const *) names with nullptr.
template <typename PackOfTagTypes>
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager);

EntityType * EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentBitSet components, TagBitSet tags);   // This is the main one, but you should use one of the other, more convenient functions.
EntityType * EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, std::initializer_list<char const *> component_names, std::initializer_list<char const *> tag_names);
//bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, ComponentCount component_seqnums [], unsigned component_count);
//bool EntityType_Register (TypeManager * type_manager, char const * name, SizeType initial_capacity, char const * component_names [], unsigned component_count);
//bool EntityType_Register_ByCompSeqnums (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...); // End with -1
//bool EntityType_Register_ByCompNames (TypeManager * type_manager, char const * name, SizeType initial_capacity, ...); // end with nullptr
bool EntityType_CloseRegisteration (TypeManager * type_manager);
bool EntityType_NameExists (TypeManager const * type_manager, char const * name);
bool EntityType_ComponentAndTagSetsExist (TypeManager const * type_manager, ComponentBitSet const & components, TagBitSet const & tags);
bool EntityType_IsRegistrationClosed (TypeManager const * type_manager);
SizeType EntityType_Count (TypeManager const * type_manager);
EntityType const * EntityType_GetFirst (TypeManager const * type_manager);
EntityType const * EntityType_GetNext (EntityType const * entity_type);
EntityType const * EntityType_FindByName (TypeManager const * type_manager, char const * name);
EntityType const * EntityType_FindBySeqNum (TypeManager const * type_manager, SizeType seqnum);
EntityType const * EntityType_FindByComponentAndTagSets (TypeManager const * type_manager, ComponentBitSet const & components, TagBitSet const & tags);

bool World_Create (World * out_world, TypeManager const * type_manager, SizeType data_page_size, SizeType max_entity_types = MaxEntityTypes, ComponentCount max_component_types = MaxComponentTypes, ComponentCount max_tags = MaxTagTypes);
bool World_Destroy (World * world);
WorldMemoryStats World_GatherMemoryStats (World const * world);

//======================================================================

template <typename T>
struct IsComponent {
    static constexpr bool value = 
        std::is_pod_v<T> && 
        std::is_trivially_default_constructible_v<T> && 
        std::is_trivially_destructible_v<T> && 
        std::is_trivially_copyable_v<T> && 
        std::is_trivially_default_constructible_v<T> && 
        std::is_base_of_v<ComponentBase<T>, T>;
        //std::is_same_v<std::invoke_result_t<T::GetComponentName>, char const *>
};

template <typename T>
inline constexpr bool IsComponentV = IsComponent<T>::value;

template <typename T>
struct IsTag {
    static constexpr bool value = 
        std::is_empty_v<T> && 
        std::is_trivially_default_constructible_v<T> && 
        std::is_base_of_v<TagBase<T>, T>;
        //std::is_same_v<std::invoke_result_t<T::GetComponentName>, char const *>
};

template <typename T>
inline constexpr bool IsTagV = IsTag<T>::value;

template <typename ... Ts>
struct ComponentTypePack;

template <>
struct ComponentTypePack<> {
    static constexpr bool IsComponentTypePack = true;
    static constexpr auto Count = 0;
    using ValueTupleType = std::tuple<>;
    using PointerTupleType = std::tuple<>;
    using ConstPointerTupleType = std::tuple<>;

    using HeadType = void;

    static ComponentBitSet GetBitSet () {
        return ComponentBitSet{};
    }
};

template <typename T0, typename ... Ts>
struct ComponentTypePack<T0, Ts...> {
    static constexpr bool IsComponentTypePack = true;
    static constexpr auto Count = 1 + sizeof...(Ts);
    using ValueTupleType = std::tuple<T0, Ts...>;
    using PointerTupleType = std::tuple<T0 *, Ts * ...>;
    using ConstPointerTupleType = std::tuple<T0 const *, Ts const * ...>;

    using HeadType = T0;
    using TailType = ComponentTypePack<Ts...>;

    static ComponentBitSet GetBitSet () {
        auto ret = TailType::GetBitSet();
        auto seqnum = HeadType::GetTypeInfo().seqnum;
        auto constexpr word_bits = sizeof(decltype(ret)::Word) * 8;
        ret.bits[seqnum / word_bits] |= (decltype(ret)::Word)(1) << (seqnum % word_bits);
        return ret;
    }

    static_assert(IsComponentV<HeadType>, "All types in a ComponentTypePack must be \"component\" types.");
};

template <typename ... Ts>
struct TagTypePack;

template <>
struct TagTypePack<> {
    static constexpr bool IsTagTypePack = true;
    static constexpr auto Count = 0;

    using HeadType = void;

    static TagBitSet GetBitSet () {
        return TagBitSet{};
    }
};

template <typename T0, typename ... Ts>
struct TagTypePack<T0, Ts...> {
    static constexpr bool IsTagTypePack = true;
    static constexpr auto Count = 1 + sizeof...(Ts);

    using HeadType = T0;
    using TailType = TagTypePack<Ts...>;

    static TagBitSet GetBitSet () {
        auto ret = TailType::GetBitSet();
        auto seqnum = HeadType::GetTypeInfo().seqnum;
        auto constexpr word_bits = sizeof(decltype(ret)::Word) * 8;
        ret.bits[seqnum / word_bits] |= (decltype(ret)::Word)(1) << (seqnum % word_bits);
        return ret;
    }

    static_assert(IsTagV<HeadType>, "All types in a TagTypePack must be \"tag\" types.");
};

// NOTE(yzt): Remember that the template parameters are in alphabetical order, and they are comprised of component/tag, essential/excluded/optional, and full-access/read-only.
template <
    typename T_ComponentsEssentialFullaccess,
    typename T_ComponentsEssentialReadonly,
    typename T_ComponentsExcluded,
    typename T_ComponentsOptionalFullaccess,
    typename T_ComponentsOptionalReadonly,
    typename T_TagsEssential,
    typename T_TagsExcluded
>
struct QueryParams {
    static constexpr bool IsQueryParams = true;

    using ComponentsEssentialFullaccess = T_ComponentsEssentialFullaccess;
    using ComponentsEssentialReadonly = T_ComponentsEssentialReadonly;
    using ComponentsExcluded = T_ComponentsExcluded;
    using ComponentsOptionalFullaccess = T_ComponentsOptionalFullaccess;
    using ComponentsOptionalReadonly = T_ComponentsOptionalReadonly;
    using TagsEssential = T_TagsEssential;
    using TagsExcluded = T_TagsExcluded;
    
    struct ResultIterationValue {
        typename ComponentsEssentialFullaccess::PointerTupleType fullaccess;
        typename ComponentsEssentialReadonly::ConstPointerTupleType readonly;
        typename ComponentsOptionalFullaccess::PointerTupleType opt_fullaccess;
        typename ComponentsOptionalReadonly::ConstPointerTupleType opt_readonly;
    };

    static_assert(ComponentsEssentialFullaccess::IsComponentTypePack, "");
    static_assert(ComponentsEssentialReadonly::IsComponentTypePack, "");
    static_assert(ComponentsExcluded::IsComponentTypePack, "");
    static_assert(ComponentsOptionalFullaccess::IsComponentTypePack, "");
    static_assert(ComponentsOptionalReadonly::IsComponentTypePack, "");
    static_assert(TagsEssential::IsTagTypePack, "");
    static_assert(TagsExcluded::IsTagTypePack, "");
    static_assert(ComponentsEssentialFullaccess::Count + ComponentsEssentialReadonly::Count > 0, "You must specify some components!");
};

template <typename QueryParamsType>
struct Query {
    using ParamsType = QueryParamsType;
    
    World * world;
    SizeType entity_type_count;
    EntityTypeBitSet entity_types_set;
    //SizeType * entity_types;

    static_assert(QueryParamsType::IsQueryParams);
};

template <typename QueryParamsType>
struct QueryResult {
    Query<QueryParamsType> * query;
    //SizeType query_list_index;
    SizeType type_index;
    SizeType entity_index;

    static_assert(QueryParamsType::IsQueryParams);
};

template <typename QueryParamsType>
bool Query_Create (Query<QueryParamsType> * out_query, World * world);

//template <typename QueryParamsType>
//bool World_DestroyQuery (Query<QueryParamsType> * query);


//bool World_CreateQuery (
//    EntityQuery * out_entity_query,
//    World const * world,
//    ComponentBitSet const & required_components_fullaccess,
//    ComponentBitSet const & required_components_readonly,
//    ComponentBitSet const & excluded_components,
//    TagBitSet const & required_tags,
//    TagBitSet const & excluded_tags,
//    ComponentBitSet const & optional_components_fullaccess,
//    ComponentBitSet const & optional_components_readonly
//);
//bool World_DestroyQuery (EntityQuery * query);

//void World_RunQuery (EntityQuery *);

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
    static_assert(std::is_base_of_v<ComponentBase<T>, T>, "A component type T should inherit from Ex::ComponentBase<T>.");
    //static_assert(std::is_same_v<std::invoke_result_t<T::GetComponentName>, char const *>, "A component type should have a \"static char const * GetComponentName()\" method.");
    static_assert(IsComponentV<T>);

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
template <typename T>
bool TagType_Register (TypeManager * type_manager) {
    static_assert(std::is_empty_v<T>, "A tag type should be empty.");
    static_assert(std::is_base_of_v<TagBase<T>, T>, "A tag type T should inherit from Ex::TagBase<T>.");
    static_assert(IsTagV<T>);

    bool ret = false;

    char const * const & name = T::GetTagName();
    SizeType name_len = 0;
    if (name) {for (auto p = name; *p; ++p, ++name_len);}

    TagType * tag_type = &T::s_tag_type;
    if (
        type_manager &&
        type_manager->initialized &&
        !type_manager->component_type_registration_closed &&
        !tag_type->registered &&
        !tag_type->owner &&
        !tag_type->next &&
        //!tag_type->prev &&
        nullptr != name &&
        name_len > 0 &&
        name_len <= MaxNameLen &&
        !TagType_NameExists(type_manager, name)
    ) {
        tag_type->seqnum = type_manager->tag_type_count;
        type_manager->tag_type_count += 1;
        tag_type->owner = type_manager;

        unsigned i = 0;
        for (; i < sizeof(tag_type->name) - 1 && name[i]; ++i)
            tag_type->name[i] = name[i];
        tag_type->name[i] = '\0';

        if (type_manager->tag_type_last)
            type_manager->tag_type_last->next = tag_type;
        type_manager->tag_type_last = tag_type;
        if (!type_manager->tag_type_first)
            type_manager->tag_type_first = tag_type;
        tag_type->next = nullptr;
        tag_type->registered = true;

        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
template <typename PackOfComponentTypes>
bool ComponentBitSet_Make (ComponentBitSet * out_components, TypeManager const * type_manager) {
    static_assert(PackOfComponentTypes::IsComponentTypePack);
    static_assert(PackOfComponentTypes::Count > 0);

    bool ret = false;
    if (out_components && type_manager) {
        //TODO(yzt): Make sure all the components are from the same type manager.
        *out_components = PackOfComponentTypes::GetBitSet();
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
template <typename PackOfTagTypes>
bool TagBitSet_Make (TagBitSet * out_tags, TypeManager const * type_manager) {
    static_assert(PackOfTagTypes::IsTagTypePack);
    static_assert(PackOfTagTypes::Count > 0);

    bool ret = false;
    if (out_tags && type_manager) {
        //TODO(yzt): Make sure all the tags are from the same type manager.
        *out_tags = PackOfTagTypes::GetBitSet();
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
template <typename QueryParamsType>
bool Query_Create (Query<QueryParamsType> * out_query, World * world) {
    static_assert(QueryParamsType::IsQueryParams, "");

    bool ret = false;
    if (out_query && world) {
        auto cef = QueryParamsType::ComponentsEssentialFullaccess::GetBitSet();
        auto cer = QueryParamsType::ComponentsEssentialReadonly::GetBitSet();
        auto cx = QueryParamsType::ComponentsExcluded::GetBitSet();
        auto cof = QueryParamsType::ComponentsOptionalFullaccess::GetBitSet();
        auto cor = QueryParamsType::ComponentsOptionalReadonly::GetBitSet();
        auto te = QueryParamsType::TagsEssential::GetBitSet();
        auto tx = QueryParamsType::TagsExcluded::GetBitSet();
        
        *out_query = {};
        out_query->world = world;
        out_query->entity_type_count = 0;
        for (unsigned entity_type_idx = 0; entity_type_idx < world->entity_type_count; ++entity_type_idx) {
            auto const * et = &world->entity_types[entity_type_idx];

            if (BitSet_ContainsAll(et->components.bits, cef.bits) &&
                BitSet_ContainsAll(et->components.bits, cer.bits) &&
                !BitSet_ContainsAny(et->components.bits, cx.bits) &&
                BitSet_ContainsAll(et->tags.bits, te.bits) &&
                !BitSet_ContainsAny(et->tags.bits, tx.bits)
            ) {
                BitSet_SetBit(out_query->entity_types_set.bits, entity_type_idx);
                out_query->entity_type_count += 1;
            }
        }
        ret = true;
    }
    return ret;
}
//----------------------------------------------------------------------
//template <typename QueryParamsType>
//bool World_DestroyQuery (Query<QueryParamsType> * query) {
//    bool ret = false;
//    if (query) {
//        ::free(query->entity_types);    // size is: sizeof(*query->entity_types) * query->entity_type_count
//        *query = {};
//        ret = true;
//    }
//    return ret;
//}
//----------------------------------------------------------------------
//======================================================================

}   // namespace Ex
}   // namespace y

//======================================================================
