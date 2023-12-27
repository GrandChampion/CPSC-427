#pragma once

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional>
#include <typeindex>
#include <assert.h>
#include <iostream>

class Entity {
    unsigned int id;
    static unsigned int id_count;

public:
    Entity() {
        id = id_count++;
    }

    operator unsigned int() { return id; }
};

struct ContainerInterface {
    virtual void clear() = 0;

    virtual size_t size() = 0;

    virtual void remove(Entity e) = 0;

    virtual bool has(Entity entity) = 0;
};

template<typename Component>
class ComponentContainer : public ContainerInterface {
private:
    std::unordered_map<unsigned int, unsigned int> map_entity_componentID;
    bool registered = false;
public:
    std::vector<Component> components;
    std::vector<Entity> entities;

    ComponentContainer() {
    }

    inline Component &insert(Entity e, Component c, bool check_for_duplicates = true) {
        assert(!(check_for_duplicates && has(e)) && "Entity already contained in ECS registry");

        map_entity_componentID[e] = (unsigned int) components.size();
        components.push_back(std::move(c));
        entities.push_back(e);
        return components.back();
    };

    template<typename... Args>
    Component &emplace(Entity e, Args &&... args) {
        return insert(e, Component(std::forward<Args>(args)...));
    };

    template<typename... Args>
    Component &emplace_with_duplicates(Entity e, Args &&... args) {
        return insert(e, Component(std::forward<Args>(args)...), false);
    };

    Component &get(Entity e) {
        assert(has(e) && "Entity not contained in ECS registry");
        return components[map_entity_componentID[e]];
    }

    bool has(Entity entity) {
        return map_entity_componentID.count(entity) > 0;
    }

    void remove(Entity e) {
        if (has(e)) {
            int cID = map_entity_componentID[e];
            components[cID] = std::move(components.back());
            entities[cID] = entities.back();
            map_entity_componentID[entities.back()] = cID;
            map_entity_componentID.erase(e);
            components.pop_back();
            entities.pop_back();
        }
    };

    void clear() {
        map_entity_componentID.clear();
        components.clear();
        entities.clear();
    }

    size_t size() {
        return components.size();
    }

    template<class Compare>
    void sort(Compare comparisonFunction) {
        std::sort(entities.begin(), entities.end(), comparisonFunction);
        std::vector<Component> components_new;
        components_new.reserve(components.size());
        std::transform(entities.begin(), entities.end(), std::back_inserter(components_new), [&](Entity e) {
            return std::move(get(e));
        });
        components = std::move(
                components_new);
        for (unsigned int i = 0; i < entities.size(); i++)
            map_entity_componentID[entities[i]] = i;
    }
};
