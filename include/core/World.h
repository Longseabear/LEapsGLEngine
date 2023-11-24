#pragma once

#include "entity.h"
#include "Container.h"
#include "Type.h"
#include "System.h"
#include <unordered_map>
#include <queue>
#include <core/CoreSetting.h>

namespace LEapsGL {

    template <typename T, typename = void>
    struct EntityTypeSelector {
        /**
         * @brief Default entity tpye: from option
         */
        using type = typename LEapsGL::BaseEntityType;
    };
    template <typename T>
    struct EntityTypeSelector<T, std::void_t<typename T::entity_type>> {
        /**
        * @brief The selected type, which is ProxyEntity<typename T::proxy_group>.
        */
        using type = typename T::entity_type;
    };
    
    template <typename T>
    using EntityTypeSelector_t = typename EntityTypeSelector<T>::type;


    // [Primary key, pointer] 
    // SubContext must always have a primary key and temperal entity.
    // It must be possible to create a temperal entity using the primary key.
    // You should be able to obtain the desired pointer component using the temperal entity.
    class IContext{
    };

    /*
    For global context (e.g., ShaderManager, SoundManager...)
    */
    class Context : public Singleton<Context> {
    public:
        template <typename CTX>
        static CTX& getGlobalContext() {
            constexpr size_t id = get_type_hash<CTX>();
            auto& ctx = Context::get_instance();
            auto iter = ctx.M.find(id);
            if (iter == ctx.M.end()) iter = ctx.M.emplace(id, new CTX{}).first;
            return *static_cast<CTX*>(iter->second);
        }
        std::unordered_map<size_t, IContext*> M;
    protected:
        Context() {};
    };
    enum class EventPolish {
        DIRECT, AFTER_SYSTEM, AFTER_UPDATE
    };

    namespace __internal {
        class RootWorld : public IContext{
        public:
            ~RootWorld() {};
        };
    }

    template<typename Entity = std::uint64_t, typename Allocator = std::allocator<Entity>>
    class World : public __internal::RootWorld {
    public:
        using ComponentPtr = std::shared_ptr<ContainerBase<Entity>>;

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
        void emplace(const Entity& entt, ComponentTypeSelector_t<std::decay_t<Type>>&& data) {
            this->assure<Type>().emplace(entt, std::forward<ComponentTypeSelector_t<std::decay_t<Type>>>(data));
        }

        template <typename Type>
        bool remove(const Entity& entt) {
            return this->assure<Type>().remove(entt);
        }

        template <typename Type>
        bool contains(const Entity& entt) {
            return this->assure<Type>().contains(entt);
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

        // Query component type
        template <typename Type>
        ComponentTypeSelector_t<std::decay_t<Type>>& query(const Entity & entt) {
            return this->get<Type>()->get(entt);
        }

        template <typename Type>
        auto& assure() {
            constexpr size_t id = get_type_hash<Type>();
            using option_traits = OptionSelector<component_pool_type, Type, Entity>;
            return *option_traits::ToComponentPool(this->assureComponent<Type>());
        }

        template <typename... Types>
        View<typename OptionSelector< component_pool_type, Types, Entity>::template DerivedType...> view() {
            static_assert((std::is_same_v<Entity, EntityTypeSelector_t<Types>> && ...), "All types within the View must be associated with the same entity system.");
            return { &this->assure<std::remove_const_t<Types>>()... };
        }

    private:
        vector<Entity, Allocator> entityList;
        unordered_map<size_t, ComponentPtr> components;
        size_t free_entity_num;
        size_t free_entity_id;
    };

    using ComponentOpt = LEapsGL::opt::ComponentPoolType;
    using BaseWorld = LEapsGL::World<LEapsGL::BaseEntityType>;


    class Universe : public Singleton<Universe> {
    public:
        static inline LEapsGL::BaseWorld& GetBaseWorld() {
            return Universe::get_instance().baseWorld;
        }

        template<typename... Types>
        static auto& GetRelativeWorld() {
            using entity_type = std::common_type_t<EntityTypeSelector_t<Types>...>;
            static_assert((std::is_same_v<entity_type, EntityTypeSelector_t<Types>> && ...), "All entities must share the same entity system to be obtained.");
            return Context::getGlobalContext<World<entity_type>>();
        }

        template<typename... Types>
        static auto View() {
            return Universe::GetRelativeWorld<Types...>().view<Types...>();
        }

        template<typename World>
        static World& GetWorld() {
            return Context::getGlobalContext<World>();
        }

        template<typename Entity, typename Allocator = std::allocator<Entity>>
        static void RegisterSerializableWorld() {
            constexpr size_t id = get_type_hash<Entity>();
            Universe::get_instance().serialized.push_back(&Context::getGlobalContext<World<Entity, Allocator>>());
        }

        /*
                Event System
        */
        template <EventPolish E>
        struct EventPolishWrapper {
            static constexpr EventPolish value = E;
        };
        template <EventPolish T>
        using TO_TYPE = EventPolishWrapper<T>;
        template <class Event>
        struct UniverseDispatcher : BaseDispatcher {
            UniverseDispatcher(const Event e) : event(e) {};

            virtual void send() override
            {
                auto& univ = Universe::get_instance();
                
                constexpr size_t id = get_type_hash<Event>();
                auto iter = univ.subscribers.find(id);

                for (auto* base : iter->second) {
                    auto* sub = reinterpret_cast<EventSubscriber<Event>*>(base);
                    sub->receive(this->event);
                }
            }
            const Event event;
        };

        static void registerSystem(BaseSystem* sys) {
            sys->Configure();
            Universe::get_instance().systemList.push_back(sys);
        }

        static void unregisterSystem(BaseSystem* system)
        {
            auto& univ = Universe::get_instance();
            univ.systemList.erase(std::remove(univ.systemList.begin(), univ.systemList.end(), system), univ.systemList.end());
            system->Unconfigure();
        }

        template<typename Event>
        static void subscribe(EventSubscriber<Event>* subscriber) {
            constexpr size_t id = get_type_hash<Event>();
            auto& univ = Universe::get_instance();
            auto iter = univ.subscribers.find(id);
            if (iter == univ.subscribers.end()) {
                iter = univ.subscribers.emplace(id, vector<__internal::BaseEventSubscriber*>{}).first;
            }
            iter->second.push_back(subscriber);
        }

        template<typename T>
        static void unsubscribe(EventSubscriber<T>* subscriber)
        {
            auto& univ = Universe::get_instance();

            auto index = get_type_hash<T>();
            auto found = univ.subscribers.find(index);
            if (found != univ.subscribers.end())
            {
                found->second.erase(std::remove(found->second.begin(), found->second.end(), subscriber), found->second.end());
                if (found->second.size() == 0)
                {
                    univ.subscribers.erase(found);
                }
            }
        }
        static void unsubscribeAll(void* subscriber)
        {
            auto& univ = Universe::get_instance();

            for (auto& kv : univ.subscribers)
            {
                kv.second.erase(std::remove(kv.second.begin(), kv.second.end(), subscriber), kv.second.end());
                if (kv.second.size() == 0)
                {
                    univ.subscribers.erase(kv.first);
                }
            }
        }

        template <typename Event, EventPolish Polish = EventPolish::DIRECT>
        static void emit(const Event& event) {
            auto& univ = Universe::get_instance();

            if constexpr (Polish == EventPolish::DIRECT) {
                constexpr size_t id = get_type_hash<Event>();
                for (auto* base : univ.subscribers[id]) {
                    auto* sub = reinterpret_cast<EventSubscriber<Event>*>(base);
                    sub->receive(event);
                }
            }
            else {
                auto dispatcher = std::make_shared<UniverseDispatcher<Event>>(event);
                univ.eventQueue.emplace<TO_TYPE<Polish>>(dispatcher);
            }
        }

        // System Releationship
        // 
        // Updates
        static void Update() {
            auto& univ = Universe::get_instance();

            for (auto sys : univ.systemList) {
                sys->Update();
                univ.eventQueue.sendAll<TO_TYPE<EventPolish::AFTER_SYSTEM>>();
            }
            univ.eventQueue.sendAll<TO_TYPE<EventPolish::AFTER_UPDATE>>();
        }
    private:
        LEapsGL::BaseWorld baseWorld;
        std::vector<std::shared_ptr<__internal::RootWorld>> serialized;

        // Systems
        vector<LEapsGL::BaseSystem*> systemList;

        // Event System
        unordered_map<size_t, std::vector<__internal::BaseEventSubscriber*>, std::hash<size_t>> subscribers;
        EventQueue<TO_TYPE<EventPolish::DIRECT>, TO_TYPE<EventPolish::AFTER_SYSTEM>, TO_TYPE<EventPolish::AFTER_UPDATE>> eventQueue;
    };
}


