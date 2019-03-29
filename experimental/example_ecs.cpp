#include "../experimental/y_ecs.hpp"
#include <cstdio>
#include <chrono>

inline double Now () {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

namespace ex = y::Ex;

struct NameComponent : ex::ComponentBase<NameComponent> {
    static char const * GetComponentName () {return "Name";}

    char name [255 + 1];
};

struct PositionComponent : ex::ComponentBase<PositionComponent> {
    static char const * GetComponentName () {return "Position";}

    float x, y, z;
    char _ [30];
};

struct FlagsComponent : ex::ComponentBase<FlagsComponent> {
    static char const * GetComponentName () {return "Flags";}
    bool flags [3];
    char _ [10];
};

struct DirectionComponent : ex::ComponentBase<DirectionComponent> {
    static char const * GetComponentName () {return "Direction";}

    float x, y, z, w;
    char _ [80];
};

enum class Type : uint8_t {
    F32,
};
#define EX_COMP_ACTUAL_TYPE_OF_F32      float

struct Field {
    Type type;
    size_t offset;
    size_t size;
    char const * name;
    char const * desc;
};

struct Compo {
    char const * name;
    size_t size;
    size_t field_count;
    Field const * fields;
    Compo * next;
};

#define EX_STRUCT_BEG(name_, desc_)                                         \
    struct name_ : ::y::Ex::ComponentBase<name_> {                          \
        static constexpr char const * GetComponentName() {return #name_;}   \
        static Compo const s_compo;                                         \
        static Field const s_fields [];                                     \
    /**/
#define EX_STRUCT_FLD(struct_, type_, name_, desc_)                         \
        EX_COMP_ACTUAL_TYPE_OF_ ## type_ name_;                             \
    /**/
#define EX_STRUCT_ARR(struct_, type_, cnt_, name_, desc_)                   \
        EX_COMP_ACTUAL_TYPE_OF_ ## type_ name_ [cnt_];                      \
    /**/
#define EX_STRUCT_END(name_, version_)                                      \
    };                                                                      \
    /**/

#define EX_STATIX_BEG(name_, desc_)                                         \
    inline Field const name_::s_fields [] = {                               \
    /**/
#define EX_STATIX_FLD(struct_, type_, name_, desc_)                         \
        {Type::type_, offsetof(struct_, name_),                             \
            sizeof(struct_::name_), #name_, desc_},                         \
    /**/
#define EX_STATIX_ARR(struct_, type_, cnt_, name_, desc_)                   \
        {Type::type_, offsetof(struct_, name_),                             \
            cnt_ * sizeof(struct_::name_), #name_, desc_},                  \
    /**/
#define EX_STATIX_END(name_, version_)                                      \
    };                                                                      \
    inline Compo const name_::s_compo {                                     \
        #name_, sizeof(name_),                                              \
        sizeof(name_::s_fields) / sizeof(name_::s_fields[0]),               \
        name_::s_fields, nullptr                                            \
    };                                                                      \
    /**/

#define DeclareNewComponent(definition_)                                    \
    definition_(EX_STRUCT_BEG, EX_STRUCT_FLD, EX_STRUCT_ARR, EX_STRUCT_END) \
    definition_(EX_STATIX_BEG, EX_STATIX_FLD, EX_STATIX_ARR, EX_STATIX_END) \
    /**/

struct YYY {
    static Compo const compo;
    static Field const fields [];

    float x, y, z;
};

inline Field const YYY::fields [] = {
    {Type::F32, offsetof(YYY, x), sizeof(YYY::x), "x", "X Coordinate"},
    {Type::F32, offsetof(YYY, y), sizeof(YYY::y), "y", "Y Coordinate"},
    {Type::F32, offsetof(YYY, z), sizeof(YYY::z), "z", "Z Coordinate"},
};
inline Compo const YYY::compo {"YYY", sizeof(YYY), sizeof(YYY::fields) / sizeof(YYY::fields[0]), YYY::fields, nullptr};

//
//inline bool StaticRegisterStructField (size_t offset, size_t size, char const * name, Type type, Field * fields, unsigned * count, unsigned max_count) {
//}
//
//struct XXX {
//    static inline unsigned i11n_count = 0;
//    static Field i11n_fields [10];
//
//    float x, y, z;
//
//    static inline bool __1 = StaticRegisterStructField(offsetof(XXX, x), sizeof(x), "x", Type::F32, i11n_fields, &i11n_count, 10);
//    static inline bool __2 = StaticRegisterStructField(offsetof(XXX, y), sizeof(y), "y", Type::F32, i11n_fields, &i11n_count, 10);
//    static inline bool __3 = StaticRegisterStructField(offsetof(XXX, z), sizeof(z), "z", Type::F32, i11n_fields, &i11n_count, 10);
//
//};

#define COMPONENT_DATA__ZZZ(begin, field, array, end)                       \
    begin(ZZZ, "Just a useless component that resembles 3D position.")      \
        field(ZZZ, F32, x, "X Coordinate")                                  \
        field(ZZZ, F32, y, "Y Coordinate")                                  \
        field(ZZZ, F32, z, "Z Coordinate")                                  \
    end(ZZZ, 1)                                                             \
    /**/
DeclareNewComponent(COMPONENT_DATA__ZZZ)
#undef COMPONENT_DATA__ZZZ

int main () {
    ex::TypeManager tm_;
    auto tm = &tm_;
    ex::TypeManager_Create(tm);

    ex::ComponentType_Register<NameComponent>(tm);
    ex::ComponentType_Register<PositionComponent>(tm);
    ex::ComponentType_Register<FlagsComponent>(tm);
    ex::ComponentType_Register<DirectionComponent>(tm);
    ex::ComponentType_CloseRegisteration(tm);

    ex::EntityType_Register_ByCompNames(tm, "A", 10'000, "Flags", "Position", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "B", 90'000, "Name", "Position", "Direction", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "C", 0, "Name", "Flags", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "D", 500'000, "Position", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "E", 1, "Name", "Position", "Direction", "Flags", nullptr);
    ex::EntityType_CloseRegisteration(tm);

    for (unsigned sz = 1024; sz < 500'000; sz *= 4) {
        ex::World world_;
        auto world = &world_;
        ex::WorldMemoryStats ms;

        auto t0 = 1'000 * Now();
        ex::World_Create(world, tm, sz);
        auto t1 = 1'000 * Now();
        ms = ex::World_GatherMemoryStats(world);
        auto t2 = 1'000 * Now();
        ::printf("%6zu, %zu,%zu,%zu, %8zu(%zd),%6zu,%zu,%8zu,%5zu"
            , ms.page_size_bytes
            , ms.total_component_groups, ms.active_component_groups, ms.faulty_component_groups
            , ms.total_bytes
            , ms.total_bytes - ms.overhead_bytes - ms.used_bytes - ms.usable_bytes - ms.unusable_bytes
            , ms.overhead_bytes, ms.used_bytes, ms.usable_bytes, ms.unusable_bytes
        );
        auto t3 = 1'000 * Now();
        ex::World_Destroy(world);
        auto t4 = 1'000 * Now();
        ::printf ("\t\ttimes in ms: %8.3f, %8.3f, %8.3f\n", t1 - t0, t2 - t1, t4 - t3);
    }

    ex::TypeManager_Destroy(tm);
    ZZZ z = {};
    return int(z.x);
}
