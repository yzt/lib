#include "../experimental/y_ecs.hpp"
#include <string>

namespace ex = y::Ex;

struct NameComponent : ex::ComponentBase<NameComponent> {
    static char const * GetComponentName () {return "NameComponent";}

    char name [31 + 1];
};

struct PositionComponent : ex::ComponentBase<PositionComponent> {
    static char const * GetComponentName () {return "PositionComponent";}

    float x, y, z;
};

int main () {
    ex::TypeManager tm;
    ex::TypeManager_Create(&tm);

    ex::ComponentType_Register<NameComponent>(&tm);
    ex::ComponentType_Register<PositionComponent>(&tm);

    ex::TypeManager_Destroy(&tm);
    return 0;
}
