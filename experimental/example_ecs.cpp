#include "../experimental/y_ecs.hpp"
#include <cstdio>

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

int main () {
    ex::TypeManager tm_;
    auto tm = &tm_;
    ex::TypeManager_Create(tm);

    ex::ComponentType_Register<NameComponent>(tm);
    ex::ComponentType_Register<PositionComponent>(tm);
    ex::ComponentType_Register<FlagsComponent>(tm);
    ex::ComponentType_Register<DirectionComponent>(tm);
    ex::ComponentType_CloseRegisteration(tm);

    ex::EntityType_Register_ByCompNames(tm, "A", 100, "Flags", "Position", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "B", 10'000, "Name", "Position", "Direction", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "C", 0, "Name", "Flags", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "D", 10, "Position", nullptr);
    ex::EntityType_Register_ByCompNames(tm, "E", 1, "Name", "Position", "Direction", "Flags", nullptr);
    ex::EntityType_CloseRegisteration(tm);

    for (unsigned sz = 1024; sz < 500'000; sz *= 4) {
        ex::World world_;
        auto world = &world_;
        ex::WorldMemoryStats ms;

        ex::World_Create(world, tm, sz);
        ms = ex::World_GatherMemoryStats(world);
        ::printf("%6zu, %zu,%zu,%zu, %8zu(%zd),%6zu,%zu,%8zu,%5zu\n"
            , ms.page_size_bytes
            , ms.total_component_groups, ms.active_component_groups, ms.faulty_component_groups
            , ms.total_bytes
            , ms.total_bytes - ms.overhead_bytes - ms.used_bytes - ms.usable_bytes - ms.unusable_bytes
            , ms.overhead_bytes, ms.used_bytes, ms.usable_bytes, ms.unusable_bytes
        );
        ex::World_Destroy(world);
    }

    ex::TypeManager_Destroy(tm);
    return 0;
}
