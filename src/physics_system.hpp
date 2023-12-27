#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"

class PhysicsSystem {
public:
    void step(float timeSpentFromLastUpdate);
};

extern PhysicsSystem physics_system;
