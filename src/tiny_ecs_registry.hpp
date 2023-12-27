#pragma once

#include <vector>
#include "tiny_ecs.hpp"
#include "components.hpp"

class ECSRegistry
{
    std::vector<ContainerInterface *> registry_list;

public:
    ComponentContainer<SmokeParticle> smoke_trail;
    ComponentContainer<Player> players;
    ComponentContainer<Motion> motions;
    ComponentContainer<Collision> collisions;
    ComponentContainer<Missile> missiles;
    ComponentContainer<Planet> planets;
    ComponentContainer<Mesh *> meshPtrs;
    ComponentContainer<RenderRequest> renderRequests;
    ComponentContainer<ScreenState> screenStates;
    ComponentContainer<DebugComponent> debugComponents;
    ComponentContainer<vec3> colors;
    ComponentContainer<Animation> animations;
    ComponentContainer<Sun> suns;
    ComponentContainer<HUDComponent> huds;
    ComponentContainer<IgnorePhysics> ignore_physics;
    ComponentContainer<Timer> timers;
    ComponentContainer<Phase> phases;
    ComponentContainer<Asteroid> asteroids;
    ComponentContainer<AngularMotion> angular_motions;
    ComponentContainer<SpeedUp> speed_up;
    ComponentContainer<Wormhole> wormholes;
    ComponentContainer<PlanetName> planet_names;
    ComponentContainer<Hide> hidden;

    ECSRegistry() {
        registry_list.push_back(&smoke_trail);
        registry_list.push_back(&players);
        registry_list.push_back(&motions);
        registry_list.push_back(&collisions);
        registry_list.push_back(&missiles);
        registry_list.push_back(&meshPtrs);
        registry_list.push_back(&renderRequests);
        registry_list.push_back(&screenStates);
        registry_list.push_back(&debugComponents);
        registry_list.push_back(&colors);
        registry_list.push_back(&animations);
        registry_list.push_back(&suns);
        registry_list.push_back(&huds);
        registry_list.push_back(&ignore_physics);
        registry_list.push_back(&timers);
        registry_list.push_back(&planets);
        registry_list.push_back(&phases);
        registry_list.push_back(&asteroids);
        registry_list.push_back(&angular_motions);
        registry_list.push_back(&speed_up);
        registry_list.push_back(&planet_names);
        registry_list.push_back(&hidden);
    }

    void clear_all_components()
    {
        for (ContainerInterface *reg : registry_list)
            reg->clear();
    }

    void list_all_components()
    {
        printf("Debug info on all registry entries:\n");
        for (ContainerInterface *reg : registry_list)
            if (reg->size() > 0)
                printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
    }

    void list_all_components_of(Entity e)
    {
        printf("Debug info on components of entity %u:\n", (unsigned int)e);
        for (ContainerInterface *reg : registry_list)
            if (reg->has(e))
                printf("type %s\n", typeid(*reg).name());
    }

    void remove_all_components_of(Entity e)
    {
        for (ContainerInterface *reg : registry_list)
            reg->remove(e);
    }
};

extern ECSRegistry registry;