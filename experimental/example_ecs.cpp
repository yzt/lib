#include "../experimental/y_ecs.hpp"

namespace ex = y::Ex;

struct NameComponent : ex::ComponentBase<NameComponent> {
    static char const * GetComponentName () {return "Name";}

    char name [255 + 1];
};

struct PositionComponent : ex::ComponentBase<PositionComponent> {
    static char const * GetComponentName () {return "Position";}

    float x, y, z;
};

struct FlagsComponent : ex::ComponentBase<FlagsComponent> {
    static char const * GetComponentName () {return "Flags";}
    bool flags [3];
};

struct DirectionComponent : ex::ComponentBase<DirectionComponent> {
    static char const * GetComponentName () {return "Direction";}

    float x, y, z, w;
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

    ex::TypeManager_Destroy(tm);
    return 0;
}
