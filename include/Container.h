#pragma once

#include <vector>

#include "Type.h"
#include "entity.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
using namespace std;

#define CONTAINER_DEBUG_LOG_ON true

#ifdef CONTAINER_DEBUG_LOG_ON
#define CONTAINER_DEBUG_LOG(message) \
    std::cout << message << "\n";
#else
#define CONTAINER_DEBUG_LOG(message)
#endif


// constexpr size_t pageSize = 4096;
// template <typename T, typename Allocator = std::allocator<T>>
// struct PageContainer {
// 	constexpr static size_t page_size = pageSize;

// 	using alloc_traits = std::allocator_traits<Allocator>;
// //	using page_type = std::vector<alloc_traits::pointer, Allocator>;
// };

namespace LEapsGL{

    constexpr size_t pageBits = 12;
    constexpr size_t pageMask = (1 << 12) - 1;

    // simple sparse

    template <typename Entity, typename Allocator = std::allocator<Entity>>
    class sparse_array {
        using alloc_traits = std::allocator_traits<Allocator>;
        using sparse_type = std::vector <typename alloc_traits::pointer, typename alloc_traits::template rebind_alloc<typename alloc_traits::pointer>>;
        using packed_type = std::vector<Entity, Allocator>;

        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;

        void release() {
            CONTAINER_DEBUG_LOG("Sparse array released..");

            auto page_allocator{ packed.get_allocator() };

            for (auto&& page : sparse) {
                if (page != nullptr) {
                    std::destroy(page, page + (1 << pageBits));
                    alloc_traits::deallocate(page_allocator, page, (1 << pageBits));
                    page = nullptr;
                }
            }
        }
    public:

        using iterator = typename packed_type::iterator;
        using const_iterator = typename packed_type::const_iterator;

        virtual ~sparse_array() {
            release();
        }
        Entity* sparse_ptr(const entity_type& entt) const {
            const auto entity_id = traits_type::to_entity(entt);

            const size_t bucketID = entity_id >> pageBits;
            const size_t idx = entity_id & pageMask;

            if (sparse.size() <= bucketID || !sparse[bucketID]) return nullptr;
            return &sparse[bucketID][idx];
        }
        inline Entity& sparse_get(const entity_type& entt) const {
            const auto entity_id = traits_type::to_entity(entt);
            const size_t bucketID = entity_id >> pageBits;
            const size_t idx = entity_id & pageMask;
            return sparse[bucketID][idx];
        }

        auto& assure_sparse_get(const entity_type& entt) {
            assert(entt == traits_type::to_entity(entt), "The function must receive the entity id value as input.");
            const auto entity_id = traits_type::to_entity(entt);

            const size_t bucketID = entity_id >> pageBits;
            const size_t idx = entity_id & pageMask;

            if (sparse.size() <= bucketID)
                sparse.resize(bucketID + 1, nullptr);

            if (!sparse[bucketID]) {
                auto alloc{ packed.get_allocator() };
                sparse[bucketID] = alloc_traits::allocate(alloc, 1 << pageBits);
                std::uninitialized_fill(sparse[bucketID], sparse[bucketID] + (1 << pageBits), LEapsGL::null);
            }
            return sparse[bucketID][idx];
        }

        bool contains(const Entity& entt) const noexcept {
            auto* packed_idx = sparse_ptr(entt);
            if (packed_idx == nullptr) return false;

            const size_t idx = traits_type::to_entity(*packed_idx);
            return (*packed_idx) != LEapsGL::null &&
                idx < packed.size() &&
                entt == packed[idx];
        }

        std::tuple<value_type&> get_as_tuple(const Entity& entt) {
            return std::forward_as_tuple(get(entt));
        }
        std::tuple<const value_type&> get_as_tuple(const Entity& entt) const {
            return std::forward_as_tuple(get(entt));
        }
        friend void swap(sparse_array& lhs, sparse_array& rhs) noexcept {
            using std::swap;
            swap(lhs.packed, rhs.packed);
            swap(lhs.sparse, rhs.sparse);
        }

        virtual void emplace(const entity_type& entt){
            auto& packed_idx = assure_sparse_get(entt);
            packed_idx = traits_type::construct(packed.size(), 0);
            packed.emplace_back(entt);
        }
        virtual bool remove(const Entity& entt) {
            if (!contains(entt)) return false;
            CONTAINER_DEBUG_LOG("Remove Sparse Array : " << to_string(entt));

            assert(contains(entt), "The remove function was called on an element that was not included.");

            Entity& lastSparseItem = sparse_get(packed.back());
            Entity& targetSparseItem = sparse_get(entt);

            std::swap(packed[traits_type::to_entity(targetSparseItem)], packed[traits_type::to_entity(lastSparseItem)]);
            std::swap(targetSparseItem, lastSparseItem);
            packed.pop_back();

            return true;
        }
        size_t size() const {
            return packed.size();
        }
        iterator begin() {
            return packed.begin();
        }
        const_iterator begin() const { return packed.begin(); };

        iterator end() {
            return packed.end();
        }
        const_iterator end() const { return packed.end(); };

        sparse_type sparse;
        packed_type packed;
    };


    template<typename Entity, typename Type>
    struct ComponentPoolIterator {
    private:
        int curIdx;
        vector<Entity>* packed;
        vector<Type>* components;

    public:
        ComponentPoolIterator(vector<Entity>* _packed, vector<Type>* _components, int idx = 0) : packed{ _packed }, components{ _components }, curIdx(idx) {};
        ComponentPoolIterator& operator++() {
            curIdx++;
            return *this;
        }
        ComponentPoolIterator operator++(int) {
            ComponentPoolIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        tuple<const Entity, Type&> operator*() {
            return std::tuple_cat(
                std::make_tuple((*packed)[curIdx]),
                std::forward_as_tuple((*components)[curIdx])
            );
        }
        bool operator==(const ComponentPoolIterator& rhs) const noexcept {
            return rhs.curIdx == curIdx && rhs.packed == packed && rhs.components == components;
        }

        bool operator!=(const ComponentPoolIterator& rhs) const noexcept {
            return !operator==(rhs);
        }
    };

    template<typename Entity, typename Type>
    struct ComponentPoolConstIterator {
    private:
        int curIdx;
        vector<Entity>* packed;
        vector<Type>* components;

    public:
        ComponentPoolConstIterator(vector<Entity>* _packed, vector<Type>* _components, int idx = 0) : packed{ _packed }, components{ _components }, curIdx(idx) {};
        ComponentPoolConstIterator& operator++() {
            curIdx++;
            return *this;
        }
        ComponentPoolConstIterator operator++(int) {
            ComponentPoolConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        ComponentPoolConstIterator<const Entity, Type&> operator*() const {
            return std::tuple_cat(
                std::make_tuple((*packed)[curIdx]),
                std::forward_as_tuple((*components)[curIdx])
            );
        }
        bool operator==(const ComponentPoolConstIterator& rhs) const noexcept {
            return rhs.curIdx == curIdx && rhs.packed == packed && rhs.components == components;
        }

        bool operator!=(const ComponentPoolConstIterator& rhs) const noexcept {
            return !operator==(rhs);
        }
    };

    template <typename Type, typename Entity, typename Allocator = std::allocator<Type>>
    class DefaultComponentPool : public sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
        using alloc_traits = std::allocator_traits<Allocator>;
    public:
        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using super = sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;

        using component_type = Type;

        std::tuple<component_type&> get_as_tuple(const Entity& entt) {
            return std::forward_as_tuple(get(entt));
        }
        std::tuple<const component_type&> get_as_tuple(const Entity& entt) const {
            return std::forward_as_tuple(get(entt));
        }
        component_type& get(const Entity& entt) {
            return components[(size_t)traits_type::to_entity(super::sparse_get(entt))];
        }

        void emplace(const value_type& entt, const Type& arg) {
            super::emplace(entt);
            components.emplace_back(arg);
        };

        bool remove(const Entity& entt) override {
            if (!super::contains(entt)) return false;

            CONTAINER_DEBUG_LOG("Removed from component pool : " << to_string(entt));

            const auto idx = (size_t)sparse_get(entt);
            auto out = super::remove(entt);
            if (!out) return false;
            std::swap(components[idx], components.back());
            components.pop_back();
            return true;
        }

        using iterator = ComponentPoolIterator<Entity, Type>;
        using const_iterator = ComponentPoolConstIterator<Entity, Type>;
        iterator begin() {
            return iterator(&packed, &components, 0);
        }
        const_iterator begin() const {
            return const_iterator(&packed, &components, 0);
        }
        iterator end() {
            return iterator(&packed, &components, packed.size());
        }
        const_iterator end() const {
            return const_iterator(&packed, &components, 0);
        }
    private:
        vector<Type> components;
    };

    template <typename Type, typename Entity, typename Allocator = std::allocator<Type>>
    class MemoryOptimizedComponentPool : public sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
        using alloc_traits = std::allocator_traits<Allocator>;
    public:
        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using super = sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;

        using component_type = Type;

        std::tuple<component_type&> get_as_tuple(const Entity& entt) {
            return std::forward_as_tuple(get(entt));
        }
        std::tuple<const component_type&> get_as_tuple(const Entity& entt) const {
            return std::forward_as_tuple(get(entt));
        }
        // -------------------------------------------------
        // Container pool interface
        component_type& get(const Entity& entt) {
            return components[(size_t)traits_type::to_entity(get_index(entt))];
        }
        bool contains(const Entity& entt) {
            for (const auto& x : packed) if (x == entt) return true;
            return false;
        }
        void emplace(const Entity& entt, const Type& arg) {
            packed.emplace_back(entt);
            components.emplace_back(arg);
        };
        bool remove(const Entity& entt) override {
            if (!contains(entt)) return false;

            CONTAINER_DEBUG_LOG("Removed from component pool : " << to_string(entt));

            const auto idx = (size_t)get_index(entt);
            std::swap(packed[idx], packed.back());
            std::swap(components[idx], components.back());
            packed.pop_back();
            components.pop_back();
            return true;
        }
        // -------------------------------------------------
        using iterator = ComponentPoolIterator<Entity, Type>;
        using const_iterator = ComponentPoolConstIterator<Entity, Type>;
        iterator begin() {
            return iterator(&packed, &components, 0);
        }
        const_iterator begin() const {
            return const_iterator(&packed, &components, 0);
        }
        iterator end() {
            return iterator(&packed, &components, packed.size());
        }
        const_iterator end() const {
            return const_iterator(&packed, &components, 0);
        }
    private:
        inline size_t get_index(const Entity& entt) {
            for (size_t i = 0; i < packed.size(); i++) if (entt == packed[i]) return i;
            assert(false, "get_index should guarantee the presence of data.");
        }
        vector<Type> components;
    };

    template <typename Type, typename Entity, typename Allocator = std::allocator<Type>>
    class MemoryOptimizedComponentPool : public sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
        using alloc_traits = std::allocator_traits<Allocator>;
    public:
        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using super = sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;

        using component_type = Type;

        std::tuple<component_type&> get_as_tuple(const Entity& entt) {
            return std::forward_as_tuple(get(entt));
        }
        std::tuple<const component_type&> get_as_tuple(const Entity& entt) const {
            return std::forward_as_tuple(get(entt));
        }
        // -------------------------------------------------
        // Container pool interface
        component_type& get(const Entity& entt) {
            return components[(size_t)traits_type::to_entity(get_index(entt))];
        }
        bool contains(const Entity& entt) {
            for (const auto& x : packed) if (x == entt) return true;
            return false;
        }
        void emplace(const Entity& entt, const Type& arg) {
            packed.emplace_back(entt);
            components.emplace_back(arg);
        };
        bool remove(const Entity& entt) override {
            if (!contains(entt)) return false;

            CONTAINER_DEBUG_LOG("Removed from component pool : " << to_string(entt));

            const auto idx = (size_t)get_index(entt);
            std::swap(packed[idx], packed.back());
            std::swap(components[idx], components.back());
            packed.pop_back();
            components.pop_back();
            return true;
        }
        // -------------------------------------------------
        using iterator = ComponentPoolIterator<Entity, Type>;
        using const_iterator = ComponentPoolConstIterator<Entity, Type>;
        iterator begin() {
            return iterator(&packed, &components, 0);
        }
        const_iterator begin() const {
            return const_iterator(&packed, &components, 0);
        }
        iterator end() {
            return iterator(&packed, &components, packed.size());
        }
        const_iterator end() const {
            return const_iterator(&packed, &components, 0);
        }
    private:
        inline size_t get_index(const Entity& entt) {
            for (size_t i = 0; i < packed.size(); i++) if (entt == packed[i]) return i;
            assert(false, "get_index should guarantee the presence of data.");
        }
        vector<Type> components;
    };

    //template<typename Type, typename Entity = LEapsGL::entity, typename Allocator = std::allocator<std::remove_const_t<Type>>>
    //struct component_pool_for {
    //    /*! @brief Type-to-storage conversion result. */
    //    using type = constness_as_t<storage_type_t<std::remove_const_t<Type>, Entity, Allocator>, Type>;
    //};
    //
    //template<typename Type>
    //using component_pool_for_type = typename component_pool_for<Type, Entity, typename alloc_traits::template rebind_alloc<std::remove_const_t<Type>>>::type;


    template<typename T, typename Tuple>
    struct type_index;


    template <typename T, typename... Others>
    struct type_index<T, std::tuple<T, Others...>> {
        static constexpr size_t value = 0;
    };

    template <typename T, typename U, typename... Others>
    struct type_index<T, std::tuple<U, Others...>> {
        static constexpr size_t value = 1 + type_index<T, std::tuple<Others...>>::value;
    };


    template <typename... ComponentPoolTypes>
    class View {
    private:

        using Entity = std::common_type_t<typename ComponentPoolTypes::value_type...>;
        using super_type = std::common_type_t<typename ComponentPoolTypes::super...>;

        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using traits_type = LEapsGL::entity_traits<Entity>;

        template<typename T>
        static constexpr std::size_t index_of = type_index<T, std::tuple <typename ComponentPoolTypes::component_type...>>::value;

        // The set with the smallest element
        const super_type* view;
        void setSmallestComponentPoolPointer() noexcept {
            view = std::get<0>(containers_);
            std::apply([this](auto*, auto *...components) {((this->view = components->size() < this->view->size() ? components : this->view), ...); }, containers_);
        }

        template<std::size_t ... Index>
        decltype(auto) get(const Entity entt) const {
            if constexpr (sizeof...(Index) == 0) {
                return std::apply([entt](auto * ... cur) { return std::tuple_cat(cur->get_as_tuple(entt)...); }, containers_);
            }
            else if constexpr (sizeof...(Index) == 1) {
                return (std::get<Index>(containers_)->get(entt), ...);
            }
            else {
                return std::tuple_cat(std::get<Index>(containers_)->get_as_tuple(entt)...);
            }
        }

    public:
        View(ComponentPoolTypes*... val) : containers_(std::tie(val...)) {
            setSmallestComponentPoolPointer();
        }

        template <typename T, typename... Others>
        decltype(auto) get(const Entity& entt) {
            return get<index_of<T>, index_of<Others>...>(entt);
        }

        template <typename Fun>
        void each(Fun& fn) {
            view ? pick_and_each(fn, std::index_sequence_for<ComponentPoolTypes...>{}) : void();
        }

        template <typename Fun, std::size_t... Index>
        void pick_and_each(Fun& fn, std::index_sequence<Index...> sequence) const {
            ((view == std::get<Index>(containers_) ? each<Index>(fn, sequence) : void()), ...);
        }


        template<std::size_t BaseIndex, std::size_t Others, typename... Args>
        auto dispatch_get(const std::tuple<const entity_type, Args...>& items) const {
            if constexpr (Others == BaseIndex) {
                return std::forward_as_tuple(std::get<Args>(items)...);
            }
            else {
                return std::get<Others>(containers_)->get_as_tuple(std::get<0>(items));
            }
        }

        template <std::size_t BaseIndex, typename Fun, std::size_t... Index>
        void each(Fun& fn, std::index_sequence<Index...>) const {
            for (const auto items : *std::get<BaseIndex>(containers_)) {
                if (const auto entt = std::get<0>(items); ((BaseIndex == Index || std::get<Index>(containers_)->contains(entt)) && ...)) {
                    if constexpr (std::is_invocable<decltype(fn), ComponentPoolTypes::component_type...>::value) {
                        std::apply(fn, std::tuple_cat(dispatch_get<BaseIndex, Index>(items)...));
                    }
                    else {
                        std::apply(fn, std::tuple_cat(std::make_tuple(entt), dispatch_get<BaseIndex, Index>(items)...));
                    }
                }
            }
        }

        //    using const_iterator = ViewConstIterator;
        class ViewConstIterator {
        public:
            using const_entity_iterator = typename super_type::const_iterator;

            template <std::size_t... Index>
            bool checkAlltypeContains(const Entity& entt, std::index_sequence<Index...>) const {
                return (std::get<Index>(container->containers_)->contains(entt) && ...);
            }
            ViewConstIterator() :container{}, iter{} {};
            ViewConstIterator(View* _view, const_entity_iterator _iter) : container(_view), iter(_iter) {
                while (iter != container->view->end() && (!checkAlltypeContains(*iter, std::index_sequence_for<ComponentPoolTypes...>{}))) iter++;
            };
            const Entity& operator*() {
                return *iter;
            }
            const ViewConstIterator& operator++() {
                iter++;
                while (iter != container->view->end() && (!checkAlltypeContains(*iter, std::index_sequence_for<ComponentPoolTypes...>{}))) iter++;
                return *this;
            }

            bool operator==(const ViewConstIterator& other) const noexcept {
                return iter == other.iter && container == other.container;
            }
            bool operator!=(const ViewConstIterator& other) const noexcept {
                return !operator==(other);
            }
        private:
            View* container;
            const_entity_iterator iter;
        };
        using const_iterator = ViewConstIterator;

        auto begin() {
            return const_iterator{ static_cast<View*>(this), view->begin() };
        }

        auto end() {
            return const_iterator{ static_cast<View*>(this), view->end() };
        }

    private:
        std::tuple<ComponentPoolTypes*...> containers_;
    };

    enum class ComponentPoolType {
        Default, Dynamic, MemoryOptimized,
    };
    namespace __internal {
        // [Todo] make general selector
        template <ComponentPoolType T, typename Type, typename Entity>
        struct ComponentPoolSelector;

        template <typename Type, typename Entity>
        struct ComponentPoolSelector<ComponentPoolType::Default, Type, Entity> {
            using type = DefaultComponentPool<Type, Entity>;
            static constexpr ComponentPoolType value = ComponentPoolType::Default;
        };
        template <typename Type, typename Entity>
        struct ComponentPoolSelector<ComponentPoolType::Dynamic, Type, Entity> {
            using type = DefaultComponentPool<Type, Entity>;
            static constexpr ComponentPoolType value = ComponentPoolType::Dynamic;
        };
        template <typename Type, typename Entity>
        struct ComponentPoolSelector<ComponentPoolType::MemoryOptimized, Type, Entity> {
            using type = MemoryOptimizedComponentPool<Type, Entity>;
            static constexpr ComponentPoolType value = ComponentPoolType::MemoryOptimized;
        };

        // Default Handler : Ÿ���� � option�� �������� (entity X)
        template<typename Type, typename = void>
        struct __component_pool_select_handler {
            static constexpr ComponentPoolType value = ComponentPoolType::Default;
        };
        template<typename Type>
        struct __component_pool_select_handler<Type, std::void_t<decltype(Type::COMPONENT_POOL_TYPE)>> {
            static constexpr ComponentPoolType value = Type::COMPONENT_POOL_TYPE;
        };
    };
    namespace opt {
        using ComponentPoolType = ComponentPoolType;

        template<ComponentPoolType opt>
        struct IOptionComponentPoolType{
            static constexpr ComponentPoolType COMPONENT_POOL_TYPE = opt;
        };

        template<typename Type, typename Entity>
        struct OptionSelector<ComponentPoolType, Type, Entity> {
            static constexpr ComponentPoolType value = typename LEapsGL::__internal::__component_pool_select_handler<Type>::value;
            using selector = typename __internal::ComponentPoolSelector<value, Type, Entity>;

            using BaseType = sparse_array<Entity>;
            using DerivedType = typename selector::type;

            // ---------------------------------------- interface
            [[nodiscard]] static constexpr std::shared_ptr<BaseType> CreateShared() noexcept {
                return std::make_shared<DerivedType>();
            }
            [[nodiscard]] static constexpr decltype(auto) ToComponentPool(const std::shared_ptr<BaseType>& ptr) noexcept {
                return static_cast<DerivedType*>(ptr.get());
            }
        };
    }

    
}