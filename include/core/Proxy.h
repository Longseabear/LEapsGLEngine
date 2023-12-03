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
        using proxy_group = TextureGroup; // If a group is specified, it is stored in the same world. (default: Use the default entity, which is dedicated for self-only usage)
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

    template <typename instance_type>
    struct ProxyRequestSpecification{
        using BaseSpec = ProxyRequestSpecification<instance_type>;
        using baseSpecPtr = std::unique_ptr<BaseSpec>;
        static inline std::unordered_map<size_t, std::pair<baseSpecPtr, int>> Counter;

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


    /**
     * @brief Template struct to select the proxy group type based on the presence of `proxy_group` member.
     *
     * If `T` has a member type named `proxy_group`, it is used as the selected type.
     * Otherwise, the type is selected as `ProxyEntity<typename T::instance_type>`.
     *
     * @tparam T The type for which to select the proxy group type.
     * @tparam VoidType (Internal) Used for SFINAE to check the existence of `T::proxy_group`.
     */
    template <typename T, typename = void>
    struct ProxyGroupTypeSelector {
        /**
         * @brief The selected type, which is ProxyEntity<typename T::instance_type>.
         */
        using type = typename ProxyEntity<typename T::instance_type>;
    };

    /**
     * @brief Specialization of ProxyGroupTypeSelector when T::proxy_group is present.
     *
     * In this case, `T::proxy_group` is used as the selected type.
     *
     * @tparam T The type for which to select the proxy group type.
     */
    template <typename T>
    struct ProxyGroupTypeSelector<T, std::void_t<typename T::proxy_group>> {
        /**
        * @brief The selected type, which is ProxyEntity<typename T::proxy_group>.
        */
        using type = ProxyEntity<typename T::proxy_group>;
    };


    template <typename ProxyEntityType, typename InstanceType>
    class ProxyRequestor;

    class ProxyTraits{
    public:
        template<typename Specification>
        static inline auto Get(Specification spec) {
            using Specification_type = Specification;
            using instance_type = typename Specification::instance_type;
            using proxy_entity_type = typename ProxyGroupTypeSelector<Specification>::type;
            using BasePtrType = ProxyRequestSpecification<instance_type>;

            auto& Counter = BasePtrType::Counter;

            size_t h = spec.hash();
            auto iter = Counter.find(h);
            if (iter == Counter.end()) {
                iter = Counter.emplace(h, std::make_pair(std::make_unique<Specification>(spec), 0)).first;
                ProxyRequestor<proxy_entity_type, instance_type>::cachedEntity[h] = null_entity{};
            }

            return ProxyRequestor<proxy_entity_type, instance_type>(h, 0);
        }
    };

    /**
     * @brief ProxyRequestor performs the following tasks:
     *
     * 1. If the held `entt` is valid, it returns the entity as is.
     * 2. If the held `entt` is not valid, it retrieves the entity using the hash value of the specification.
     * 3. If the hash value of the specification is also unavailable, it reconstructs the entity using the specification.
     */
    template <typename ProxyEntityType, typename InstanceType>
    struct ProxyRequestor {
    public:
        using proxy_entity_type = typename ProxyEntityType; 
        using instance_type = typename InstanceType;
        using BaseSpecType = ProxyRequestSpecification<instance_type>;

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

            version = 0;

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

    


    template <class InstanceType>
    struct ProxyPrototypeCounter {
        static inline size_t version = 0;
    };




    // Change Specification to entity.
    class Proxy : public IContext{
    public:
        using SpecificationToEnttMap = std::unordered_map<size_t, __internal::ProxyEntityBase>;

        /**
         * @brief Ensures the following for the given requestor:
         *        - The associated cached Entt (requestor.getHash()) exists: Requestor::cachedEntt[requestor.getHash]
         *        - The Counter object exists: ProxyRequestSpecification::Counter[requestor.getHash]
         */
        template<typename ProxyEntityType, typename InstanceType>
        static void update_requestor(const ProxyRequestor<ProxyEntityType, InstanceType>& requestor, bool shouldCreate = true) {
            auto& M = ProxyRequestor<ProxyEntityType, InstanceType>::cachedEntity;
            auto& world = Universe::GetWorld<World<ProxyEntityType>>();

            if (world.contains<InstanceType>(requestor.entt));

            const size_t h = requestor.getHash();
            requestor.entt = M[h];

            // 3) from world
            if (shouldCreate && !world.contains<InstanceType>(requestor.entt)) {
                // creation instance
                requestor.entt = M[h] = world.Create();
            }
        }

        template<typename ProxyEntityType, typename InstanceType>
        static typename InstanceType& assure(const ProxyRequestor<ProxyEntityType, InstanceType>& requestor) {
            Proxy::update_requestor(requestor);
            auto& world = Universe::GetWorld<World<ProxyEntityType>>();
            if (!world.contains<InstanceType>(requestor.entt)) world.emplace<InstanceType>(requestor.entt, requestor.generateInstance());
            return world.query<InstanceType>(requestor.entt);
        }

        template<typename ProxyEntityType, typename InstanceType>
        static typename InstanceType* try_get(const ProxyRequestor<ProxyEntityType, InstanceType>& requestor) {
            Proxy::update_requestor(requestor, false);
            auto& world = Universe::GetWorld<World<ProxyEntityType>>();
            InstanceType* out = nullptr;
            if (world.contains<InstanceType>(requestor.entt)) out = &world.query<InstanceType>(requestor.entt);
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
        template<typename ProxyEntityType, typename InstanceType>
        static typename bool remove(const ProxyRequestor<ProxyEntityType, InstanceType>& requestor) {
            bool res = false;
            auto& M = ProxyRequestor<ProxyEntityType, InstanceType>::cachedEntity;
            auto& world = Universe::GetWorld<World<ProxyEntityType>>();

            // assure() returns an updated requestor
            Proxy::update_requestor(requestor);

            res |= world.remove<InstanceType>(requestor.entt);
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
        template<typename ProxyEntityType, typename InstanceType>
        static typename InstanceType& update(const ProxyRequestor<ProxyEntityType, InstanceType>& requestor) {
            return Proxy::assure(requestor) = requestor.generateInstance();
        }

        template<typename ProxyEntityType, typename InstanceType>
        static ProxyRequestor<ProxyEntityType, InstanceType> prototype(const ProxyRequestor<ProxyEntityType, InstanceType>& requestor) {
            auto newRequestor(requestor);
            newRequestor.setVersion(++ProxyPrototypeCounter<InstanceType>::version);

            // Copy constructor & to rvalue
            Proxy::assure(newRequestor) = std::move(InstanceType(Proxy::assure(requestor)));
            // Avoid unnecessary object creation at the point of generating the requestor, thus the object creation is commented out.
//            newRequestor.entt = proxy.emplace<ProxyEntityType, InstanceType>(this->assure<InstanceType>(), newRequestor.getHash(), instance);
            return newRequestor;
        }
 
        template<typename ProxyEntityType, typename InstanceType>
        static auto& GetWorld(const ProxyRequestor<ProxyEntityType, InstanceType>& requestor) {
            auto& world = Universe::GetWorld<World<ProxyEntityType>>();
            return world;
        }

        template <typename ProxyEntityType>
        static auto& GetWorld() {
            auto& world = Universe::GetWorld<World<ProxyEntityType>>();
            return world;
        }

    };
}
