#pragma once
#include <iostream>
#include <unordered_map>
#include <core/World.h>
#include <core/entity.h>

#define PROXY_SPECIFICATION_DEBUG_LOG_ON true

#ifdef PROXY_SPECIFICATION_DEBUG_LOG_ON
#define PROXY_SPECIFICATION_DEBUG_LOG(message) \
    std::cout << message << "\n";
#else
#define PROXY_SPECIFICATION_DEBUG_LOG(message)
#endif


namespace LEapsGL{
   
    /*
    A proxy object can be used for anything, but when using pointers, it is advisable to implement the rule of five.
    Note: The same proxy_group is assigned to the same world. If instance_output is the same, it is allocated to the same component pool.

        struct ProxyObjectExample {};
    struct SpecificationExample : public LEapsGL::ProxyRequestSpecification<MyObject> {
    public:
        // Required::
        // ---------------------------------------------
        using instance_type = Texture2D; // Type of object to create
        // ---------------------------------------------

        int sample = 0;

        virtual instance_type generateInstance() const {
            // Implement an object creation method for a given specification
        }
        virtual size_t hash() override
        {
            // Implement an object creation method for a given specification
            return std::hash{};
        }
    };
    */

    // --------------------
    // 1) <entity_type, instance_type>::cache...

    template <typename ComponentType>
    struct ProxyRequestSpecification{
        using instance_type = traits::to_instance_t<ComponentType>;
        using BaseSpec = ProxyRequestSpecification<ComponentType>;
        using baseSpecPtr = std::unique_ptr<BaseSpec>;
        static inline std::unordered_map<size_t, std::pair<baseSpecPtr, int>> Counter;

        static inline size_t totalVersion = 0;

        inline static void increasement(const size_t x) {
            BaseSpec::Counter[x].second++;
            PROXY_SPECIFICATION_DEBUG_LOG("Hire: ID: " + std::to_string(x) + " Count: " + std::to_string(BaseSpec::Counter[x].second));
        }
        inline static bool decrementEraseAndCheckIfZero(const size_t x) {
            if (--BaseSpec::Counter[x].second == 0) {
                BaseSpec::Counter.erase(x);
                PROXY_SPECIFICATION_DEBUG_LOG("Fire: ID: " + std::to_string(x) + " Count: 0");
                return true;
            }
            else {
                PROXY_SPECIFICATION_DEBUG_LOG("Fire: ID: " + std::to_string(x) + " Count: " + std::to_string(BaseSpec::Counter[x].second));
                return false;
            }
        }

        inline static instance_type GenerateInstance(size_t x) {
            return BaseSpec::Counter[x].first->generateInstance();
        }

    private:
        virtual size_t hash() = 0;
        virtual instance_type generateInstance() const = 0;
    };

    class Proxy;

    namespace __internal {
        class ProxyEntityBase {
        public:
            using entity_type = std::uint32_t;

            constexpr ProxyEntityBase(const entity_type& d) : id(d) {};
            ProxyEntityBase(const ProxyEntityBase& d) : id(d.id) {};
            uint32_t id;

        };
    }
    template<class T>
    class ProxyEntity : public __internal::ProxyEntityBase {
    public:
        using super = __internal::ProxyEntityBase;
        using entity_type = std::uint32_t;

        constexpr ProxyEntity() : super(null_entity{}) {};
        constexpr ProxyEntity(const entity_type& d) : super(d) {};
        ProxyEntity(const ProxyEntity& entt) : super(entt.id) {};
        ProxyEntity(const ProxyEntityBase& entt) : super(entt.id) {};

        operator entity_type() const {
            return static_cast<entity_type>(id);
        }
    };

    template <typename ComponentType>
    class ProxyRequestor;

    class ProxyTraits{
    public:
        template<typename Specification>
        static inline auto Get(Specification spec) {
            using Specification_type = Specification;
            using component_type = typename Specification::component_type;
            using instance_type = typename traits::to_instance_t<component_type>;
            using BasePtrType = ProxyRequestSpecification<component_type>;

            auto& Counter = BasePtrType::Counter;

            size_t h = spec.hash();
            auto iter = Counter.find(h);
            if (iter == Counter.end()) {
                iter = Counter.emplace(h, std::make_pair(std::make_unique<Specification>(spec), 0)).first;
                ProxyRequestor<component_type>::cachedEntity[h] = null_entity{};
            }

            return ProxyRequestor<component_type>(h, 0);
        }
    };

    /**
     * @brief ProxyRequestor performs the following tasks:
     *
     * 1. If the held `entt` is valid, it returns the entity as is.
     * 2. If the held `entt` is not valid, it retrieves the entity using the hash value of the specification.
     * 3. If the hash value of the specification is also unavailable, it reconstructs the entity using the specification.
     */
    template <typename ComponentType>
    struct ProxyRequestor {
    public:
        using proxy_entity_type = typename traits::to_entity_t<ComponentType>; 
        using instance_type = typename traits::to_instance_t<ComponentType>;
        using BaseSpecType = ProxyRequestSpecification<ComponentType>;

        friend void swap(ProxyRequestor& lhs, ProxyRequestor& rhs) noexcept {
            using std::swap;
            swap(lhs.entt, rhs.entt);
            swap(lhs.packedObject, rhs.packedObject);
            swap(lhs.version, rhs.version);
        }
        const ProxyRequestor(const ProxyRequestor& rhs) : entt(rhs.entt), packedObject(rhs.packedObject), version(rhs.version) {
            BaseSpecType::increasement(packedObject);
        }
        const ProxyRequestor(ProxyRequestor&& rhs) noexcept : ProxyRequestor() {
            swap(*this, rhs);
        }
        
        const ProxyRequestor& operator=(ProxyRequestor other) {
            swap(*this, other);
            return *this;
        }
        virtual ~ProxyRequestor() {
            if (BaseSpecType::decrementEraseAndCheckIfZero(packedObject)) ProxyRequestor::cachedEntity.erase(packedObject);
        }

        // move constructor not need (swap and idiom)

        inline size_t getHash() const {
            size_t h = LEapsGL::PROXY_SEED;
            LEapsGL::hash_combine(h, packedObject, version);
            return h;
        }

        bool operator==(const ProxyRequestor& rhs) {
            return getHash() == rhs.getHash();
        }
        bool operator!=(const ProxyRequestor& rhs) {
            return !operator==(rhs);
        }
        bool operator<(const ProxyRequestor& rhs) {
            return getHash() < rhs.getHash();
        }
        bool operator>(const ProxyRequestor& rhs) {
            return getHash() > rhs.getHash();
        }

    private:
        friend Proxy;
        friend ProxyTraits;

        // Cache
        static inline std::unordered_map<size_t, proxy_entity_type> cachedEntity;

        const ProxyRequestor(size_t packed, uint32_t ver) : entt(LEapsGL::null_entity{}), packedObject(packed), version(ver){
            BaseSpecType::increasement(packedObject);
        }
        void setVersion(size_t ver) {
            this->version = ver;
            this->entt = null_entity{};
        }

        inline instance_type generateInstance() const {
            return std::move(BaseSpecType::GenerateInstance(packedObject));
        }

        ProxyRequestor() : entt(null_entity{}), packedObject(0), version(0) {
            BaseSpecType::increasement(packedObject);
        };

        // mutable
        mutable proxy_entity_type entt;
        uint32_t version;
        size_t packedObject;
    };

    


    // Change Specification to entity.
    class Proxy : public IContext{
    private:
        /**
         * @brief Ensures the following for the given requestor:
         *        - The associated cached Entt (requestor.getHash()) exists: Requestor::cachedEntt[requestor.getHash]
         *        - The Counter object exists: ProxyRequestSpecification::Counter[requestor.getHash]
         */
        template<typename ComponentType>
        static void update_requestor(const ProxyRequestor<ComponentType>& requestor, bool shouldCreate = true) {
            auto& M = ProxyRequestor<ComponentType>::cachedEntity;
            auto& world = Universe::GetWorld<traits::to_world_t<ComponentType>>();

            if (world.contains<ComponentType>(requestor.entt)) return;

            const size_t h = requestor.getHash();
            requestor.entt = M[h];

            // 3) from world
            if (shouldCreate && !world.contains<ComponentType>(requestor.entt)) {
                // creation instance
                requestor.entt = M[h] = world.Create();
            }
        }
    public:
        using SpecificationToEnttMap = std::unordered_map<size_t, __internal::ProxyEntityBase>;

        template<typename ComponentType>
        static typename traits::to_instance_t<ComponentType>& assure(const ProxyRequestor<ComponentType>& requestor) {
            Proxy::update_requestor(requestor);
            auto& world = Universe::GetWorld<typename traits::to_world_t<ComponentType>>();
            if (!world.contains<ComponentType>(requestor.entt)) world.emplace<ComponentType>(requestor.entt, requestor.generateInstance());
            return world.query<ComponentType>(requestor.entt);
        }

        template<typename ComponentType>
        static typename traits::to_instance_t<ComponentType>* try_get(const ProxyRequestor<ComponentType>& requestor) {
            Proxy::update_requestor(requestor, false);
            auto& world = Universe::GetWorld<typename traits::to_world_t<ComponentType>>();
            traits::to_instance_t<ComponentType>* out = nullptr;
            if (world.contains<ComponentType>(requestor.entt)) out = &world.query<ComponentType>(requestor.entt);
            return out;
        }

        /**
         * @brief Removes an entity associated with the given ProxyRequestor from the specified map and EnTT world.
         *
         * @tparam ProxyEntityType The type of the entity managed by the ProxyRequestor.
         * @tparam InstanceType The type of the instance associated with the entity.
         *
         * @param requestor The ProxyRequestor representing the entity to be removed.
         *
         * @return true if the removal is successful, false otherwise.
         *
         * @details
         * 1) This function removes the entity's data from the EnTT world using the specified ProxyRequestor.
         * 2) It also removes the cached entity from the associated map.
         * 3) If the entity is not found or removal from the EnTT world fails, false is returned.
         * 4) After removal, the ProxyRequestor's entt is set to `null_entity{}`.
         */
        template<typename ComponentType>
        static typename bool remove(const ProxyRequestor<ComponentType>& requestor) {
            bool res = false;
            auto& M = ProxyRequestor<ComponentType>::cachedEntity;
            auto& world = Universe::GetWorld<typename traits::to_world_t<ComponentType>>();

            // assure() returns an updated requestor
            Proxy::update_requestor(requestor);

            res |= world.remove<ComponentType>(requestor.entt);
            requestor.entt = M[requestor.getHash()] = null_entity{};
            return res;
        }

        /**
         * @brief Updates the component value associated with the given ProxyRequestor.
         *
         * @tparam ProxyEntityType The type of the entity managed by the ProxyRequestor.
         * @tparam InstanceType The type of the instance associated with the entity.
         *
         * @param requestor The ProxyRequestor representing the entity to be updated.
         * @return A reference to the updated component value.
         *
         * @details
         * This function reevaluates the component value assigned to the entity specified by requestor.entt.
         * The updated value is obtained by invoking requestor.generateInstance().
         *
         * @note
         * The name 'update' is appropriate as it clearly conveys the intention of modifying the component value.
         */
        template<typename ComponentType>
        static typename traits::to_instance_t<ComponentType>& update(const ProxyRequestor<ComponentType>& requestor) {
            return Proxy::assure(requestor) = requestor.generateInstance();
        }

        template<typename ComponentType>
        static ProxyRequestor<ComponentType> prototype(const ProxyRequestor<ComponentType>& requestor) {
            auto newRequestor(requestor);
            newRequestor.setVersion(++ProxyRequestSpecification<ComponentType>::totalVersion);

            // Copy constructor & to rvalue
            Proxy::assure(newRequestor) = std::move(traits::to_instance_t<ComponentType>(Proxy::assure(requestor)));
            // Avoid unnecessary object creation at the point of generating the requestor, thus the object creation is commented out.
//            newRequestor.entt = proxy.emplace<ProxyEntityType, InstanceType>(this->assure<InstanceType>(), newRequestor.getHash(), instance);
            return newRequestor;
        }
 
        template<typename ComponentType>
        static auto& GetWorld(const ProxyRequestor<ComponentType>& requestor) {
            auto& world = Universe::GetWorld<typename traits::to_world_t<ComponentType>>();
            return world;
        }

        template <typename ComponentType>
        static auto& GetWorld() {
            auto& world = Universe::GetWorld<typename traits::to_world_t<ComponentType>>();
            return world;
        }
    };
}
