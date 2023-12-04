#pragma once

#include <unordered_map>
#include <queue>

#include <core/Core.h>
#include <core/entity.h>
#include <core/Container.h>
#include <core/Type.h>
#include <core/System.h>
#include <core/CoreSetting.h>

namespace LEapsGL {
    /*-----------------------------------------------------------------------*/
    // Component Example..
    //struct ComponentExample {
    //    using entity_type = uint64_t; // Dependency<T>, ProxyEntity<T>
    //    using container_type = ...; /*Defualt: Dynamic Component Pool ...*/ 
    //          // => ContainerType::Dynamic, ContainerType::Flag, ContainerType::MemoryOptimized, 
    //    using instance_type = ...; // real instance type
    // };
    /*-----------------------------------------------------------------------*/
    struct ContainerType {
        struct ContainerTypeBase {};
        struct MemoryOptimized : public ContainerTypeBase{};
        struct Flag : public ContainerTypeBase {};
        struct Default  : public ContainerTypeBase {};
        struct Dynamic  : public ContainerTypeBase {};
    };
    namespace __internal {
        template <typename T, typename = void>
        struct CEntitySelector {
            using type = LEapsGL::BaseEntityType;
        };

        template <typename T>
        struct CEntitySelector<T, std::void_t<typename T::entity_type>> {
            using type = typename T::entity_type;
        };

        template <typename T, typename = void>
        struct CContainerMetaSelector {
            using type = ContainerType::Default;
        };

        template <typename T>
        struct CContainerMetaSelector<T, std::void_t<typename T::container_type>> {
            using type = typename T::container_type;
        };

        template <typename T, typename = void>
        struct IsDefineContainerType : std::false_type {};

        template <typename T>
        struct IsDefineContainerType<T, std::void_t<typename T::container_type>> : std::true_type {};

        template<typename T>
        using CEntity_t = typename CEntitySelector<T>::type;





        template <typename T, typename = void>
        struct CContainerSelector {
            static_assert (!IsDefineContainerType<T>::value, "Check the container_type failure; ensure that the container type is properly configured.");
            using type = DefaultComponentPool<T, CEntity_t<T>>;
        };

        template <typename T>
        struct CContainerSelector<T, std::enable_if_t<std::is_same_v<typename T::container_type, ContainerType::Default>>> {
            using type = DefaultComponentPool<T, CEntity_t<T>>;
        };
        template <typename T>
        struct CContainerSelector<T, std::enable_if_t<std::is_same_v<typename T::container_type, ContainerType::Dynamic>>> {
            using type = DefaultComponentPool<T, CEntity_t<T>>;
        };
        template <typename T>
        struct CContainerSelector<T, std::enable_if_t<std::is_same_v<typename T::container_type, ContainerType::MemoryOptimized>>> {
            using type = MemoryOptimizedComponentPool<T, CEntity_t<T>>;
        };
        template <typename T>
        struct CContainerSelector<T, std::enable_if_t<std::is_same_v<typename T::container_type, ContainerType::Flag>>> {
            using type = FlagComponentPool<T, CEntity_t<T>>;
        };
    }

    //template <typename ComponentType>
    //struct component_traits {
    //    using entity_type = typename __internal::CEntitySelector<ComponentType>::type;
    //    using instance_type = typename __internal::CInstanceTypeSelector<ComponentType>::type;
    //};

    namespace traits {
        template <typename ComponentType>
        using to_container_t = typename __internal::CContainerSelector<ComponentType>::type;
        //(e.g.,) LEapsGL::ComponentTraits::to_container_t<ComponentType>;

        template <typename ComponentType>
        using to_container_meta_t = typename __internal::CContainerMetaSelector<ComponentType>::type;


        template <typename ComponentType>
        using to_entity_t = typename __internal::CEntitySelector<ComponentType>::type;
        /*
        template<typename Type, typename Entity>
        [[nodiscard]] static constexpr decltype(auto) ToComponentPool(const std::shared_ptr<ContainerBase<to_entity_t<ComponentType>>>& ptr) noexcept {
            return static_cast<DerivedType*>(ptr.get());
        }*/
        //template <typename ComponentType>
        //struct ContainerPoolTratis{
        //    using BaseType = ContainerBase<traits::to_entity_t<ComponentType>>;
        //    using DerivedType = traits::to_container_t<ComponentType>;

        //    [[nodiscard]] static constexpr std::shared_ptr<BaseType> CreateShared() noexcept {
        //        return std::make_shared<DerivedType>();
        //    }
        //    [[nodiscard]] static constexpr decltype(auto) ToComponentPool(const std::shared_ptr<BaseType>& ptr) noexcept {
        //        return static_cast<DerivedType*>(ptr.get());
        //    }
        //};
    }



    // ------------------------------------------------------------------------

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
        void emplace(const Entity& entt) {            
            static_assert(std::is_same_v<typename traits::to_container_meta_t<Type>, ContainerType::Flag>, "The emplace operation is supported only in FlagContainerType. Please ensure that the container type is set to FlagContainerType."  __MY_PRETTY_FUNCTION_SIGNITURE);
            this->assure<Type>().emplace(entt);
        }

        template <typename Type>
        void emplace(const Entity & entt, traits::to_instance_t<std::decay_t<Type>>&& data) {
            this->assure<Type>().emplace(entt, std::forward<traits::to_instance_t<std::decay_t<Type>>>(data));
        }
        template <typename Type>
        void emplace(const Entity& entt, const traits::to_instance_t<std::decay_t<Type>>& data) {
            // Copy construct & to_rvalue;
            this->emplace<Type>(entt, std::move(traits::to_instance_t<std::decay_t<Type>>(data)));
        }

        void clear() {
            for (auto& entt : entityList) Destroy(entt);
            entityList.clear();
            free_entity_num = 0;
            free_entity_id = 0;
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

            return components[id] = std::make_shared<traits::to_container_t<Type>>();
        }

        template <typename Type>
        auto* get() {
            constexpr size_t id = get_type_hash<Type>();            
            return static_cast<traits::to_container_t<Type>*>(components[id].get());
        }

        // Query component type
        template <typename Type>
        traits::to_instance_t<std::decay_t<Type>>& query(const Entity & entt) {
            return this->get<Type>()->get(entt);
        }

        template <typename Type>
        auto& assure() {
            constexpr size_t id = get_type_hash<Type>();
            return *static_cast<traits::to_container_t<Type>*>(this->assureComponent<Type>().get());
        }

        
        template <typename... Types, typename... FilterType>
        View<W_ComponentPool<traits::to_container_t<Types>...>, Filter<traits::to_container_t<FilterType>...>> view(Filter<FilterType...> tmp = Filter<>{}) {
            static_assert((std::is_same_v<Entity, traits::to_entity_t<Types>> && ...), ": All types within the View must be associated with the same entity system." 
                ">>>Class Information: "
                __FUNCTION__
                ">>>Function Information "
                __MY_PRETTY_FUNCTION_SIGNITURE);
            return { &this->assure<std::remove_const_t<Types>>()... ,  &this->assure<std::remove_const_t<FilterType>>()... };
        }

    private:
        vector<Entity, Allocator> entityList;
        unordered_map<size_t, ComponentPtr> components;
        size_t free_entity_num;
        size_t free_entity_id;
    };

    using BaseWorld = LEapsGL::World<LEapsGL::BaseEntityType>;

    namespace traits {
        template <typename ComponentType>
        using to_world_t = typename LEapsGL::World<traits::to_entity_t<ComponentType>>;
    }


    class Universe : public Singleton<Universe> {
    public:
        using BaseEntityType = LEapsGL::BaseEntityType;

        static inline LEapsGL::BaseWorld& GetBaseWorld() {
            return Universe::get_instance().baseWorld;
        }

        template<typename... Types>
        static auto& GetRelativeWorld() {
            using entity_type = std::common_type_t<traits::to_entity_t<Types>...>;
            static_assert((std::is_same_v<entity_type, traits::to_entity_t<Types>> && ...), "All entities must share the same entity system to be obtained.");
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


