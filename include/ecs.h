#pragma once

#include "entity.h"
#include "Container.h"
#include <unordered_map>

namespace LEapsGL {

    template<typename Entity, typename Allocator = std::allocator<Entity>>
    class World {
    public:
        using ComponentPtr = std::shared_ptr<sparse_array<Entity, Allocator>>;

    public:
        World() = default;

        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;

        using alloc_traits = std::allocator_traits<Allocator>;

        using component_pool_type = LEapsGL::opt::ComponentPoolType;

        size_t size() const{
            return entityList.size() - free_entity_num;
        }

        const Entity Create() {
            if (free_entity_num == 0) {
                entityList.emplace_back(traits_type::construct(entityList.size(), 0));
                return entityList.back();
            }
            auto next_removed_idx = traits_type::to_entity(entityList[free_entity_id]);
            Entity entt = entityList[free_entity_id] = traits_type::construct(
                free_entity_id,
                traits_type::to_version(entityList[free_entity_id])
            );
            free_entity_num--;
            free_entity_id = next_removed_idx;
            return entt;
        }
        void Destroy(const Entity& entt){
            for (auto& iter : components) {
                iter.second->remove(entt);
            }

            if (free_entity_num++ > 0) {
                entityList[traits_type::to_entity(entt)] = traits_type::construct(
                    free_entity_id,
                    traits_type::to_version(entt));
            }
            entityList[traits_type::to_entity(entt)] =
                traits_type::next_version(entityList[traits_type::to_entity(entt)]);
            free_entity_id = traits_type::to_entity(entt);
        }

        template <typename Type>
        void emplace(const Entity& entt, const Type & data) {
            get<Type>()->emplace(entt, data);
        }

        template <typename Type>
        bool remove(const Entity& entt) {
            return get<Type>()->remove(entt);
        }

        template <typename Type>
        bool contains(const Entity& entt) {
            return get<Type>()->contains(entt);
        }

        static const version_type getEntityVersion(const Entity& entt)  {
            return traits_type::to_version(entt);
        }
        static const entity_type getEntityID(const Entity& entt) {
            return traits_type::to_entity(entt);
        }

        template <typename Type>
        auto& assureComponent() {
            constexpr size_t id = get_type_hash<Type>();

            auto iter = components.find(id);
            if (iter != components.end()) return iter->second;

            using option_traits = OptionSelector<component_pool_type, Type, Entity>;
            return components[id] = option_traits::CreateShared();
        }

        template <typename Type>
        auto* get() {
            constexpr size_t id = get_type_hash<Type>();
            using option_traits = OptionSelector<component_pool_type, Type, Entity>;
            return option_traits::ToComponentPool(components[id]);
        }

        template <typename Type>
        auto& assure() {
            constexpr size_t id = get_type_hash<Type>();
            using option_traits = OptionSelector<component_pool_type, Type, Entity>;
            return *option_traits::ToComponentPool(components[id]);
        }

        template <typename... Types>
        View<typename OptionSelector< component_pool_type, Types, Entity>::template DerivedType...> view() {
            return { get<std::remove_const_t<Types>>()... };
        }

    private:
        vector<Entity, Allocator> entityList;

        unordered_map<size_t, ComponentPtr> components;

        size_t free_entity_num;
        size_t free_entity_id;
    };
}