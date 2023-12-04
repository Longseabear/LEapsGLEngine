#pragma once

#include <vector>

#include <core/Type.h>
#include <core/entity.h>

#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include <cassert>
#include <string>
#include <queue>
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


    namespace __internal {
        template <typename T, typename = void>
        struct CInstanceTypeSelector {
            using type = T;
        };

        template <typename T>
        struct CInstanceTypeSelector<T, std::void_t<typename T::instance_type>> {
            using type = typename T::instance_type;
        };
    }

    namespace traits {
        template<typename T>
        using to_instance_t = typename __internal::CInstanceTypeSelector<T>::type;
    }


    //template <typename T, typename = void>
    //struct ComponentTypeSelector {
    //    /**
    //     * @brief Default component type = component itself.
    //     */
    //    using type = typename T;
    //};
    //template <typename T>
    //struct ComponentTypeSelector<T, std::void_t<typename T::component_type>> {
    //    /**
    //    * @brief The selected type(component type)
    //    */
    //    using type = typename T::component_type;
    //};

    //template <typename T>
    //using ComponentTypeSelector_t = typename ComponentTypeSelector<T>::type;

    // simple sparse

    template <typename Entity>
    class ContainerBase {
    public:
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;

        virtual ~ContainerBase() {};

        virtual bool remove(const Entity& entt) = 0;
        virtual void emplace(const Entity& entt) = 0;
        virtual bool contains(const Entity& entt) const = 0;
    };

    /*
        ContextContainer
    */
//    template <typename GenerationKey, typename Entity, typename Object, typename Allocator = std::allocator<Entity>>
//    class ContextObjectPool {
//
//    private:
//        unordered_map<GenerationKey, size_t, typename GenerationKey::hash_fn> keyToIndex;
//
//        virtual bool remove(const Entity& entt) = 0;
//        virtual void emplace(const Entity& entt) = 0;
//        virtual bool contains(const Entity& entt) const = 0;
//
////        SparseVector<>;
//        vector<Entity> entity;
//        vector<Object> obj;
//    };


    template <typename Entity, typename Allocator = std::allocator<Entity>>
    class sparse_array : public ContainerBase <Entity>{
    protected:
        using alloc_traits = std::allocator_traits<Allocator>;
        using sparse_type = std::vector <typename alloc_traits::pointer, typename alloc_traits::template rebind_alloc<typename alloc_traits::pointer>>;
        using packed_type = std::vector<Entity, Allocator>;

        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;

    private:
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
        Entity* sparse_ptr(const Entity& entt) const {
            const auto entity_id = traits_type::to_entity(entt);

            const size_t bucketID = entity_id >> pageBits;
            const size_t idx = entity_id & pageMask;

            if (sparse.size() <= bucketID || !sparse[bucketID]) return nullptr;
            return &sparse[bucketID][idx];
        }
        inline Entity& sparse_get(const Entity& entt) const {
            const auto entity_id = traits_type::to_entity(entt);
            const size_t bucketID = entity_id >> pageBits;
            const size_t idx = entity_id & pageMask;
            return sparse[bucketID][idx];
        }

        auto& assure_sparse_get(const Entity& entt) {
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

        virtual void emplace(const Entity& entt) override {
            auto& packed_idx = assure_sparse_get(entt);
            packed_idx = traits_type::construct(packed.size(), 0);
            packed.emplace_back(entt);
        }
        virtual bool remove(const Entity& entt) {
            if (!contains(entt)) return false;
            CONTAINER_DEBUG_LOG("Remove Sparse Array : " << to_string(traits_type::to_entity(entt)));

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
        using instance_type = traits::to_instance_t<typename Type>;

    private:
        int curIdx;
        vector<Entity>* packed;
        vector<instance_type>* components;

    public:
        ComponentPoolIterator(vector<Entity>* _packed, vector<instance_type>* _components, int idx = 0) : packed{ _packed }, components{ _components }, curIdx(idx) {};
        ComponentPoolIterator& operator++() {
            curIdx++;
            return *this;
        }
        ComponentPoolIterator operator++(int) {
            ComponentPoolIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        tuple<const Entity, instance_type&> operator*() {
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
        using instance_type = typename traits::to_instance_t<Type>;

    private:
        int curIdx;
        vector<Entity>* packed;
        vector<instance_type>* components;

    public:
        ComponentPoolConstIterator(vector<Entity>* _packed, vector<instance_type>* _components, int idx = 0) : packed{ _packed }, components{ _components }, curIdx(idx) {};
        ComponentPoolConstIterator& operator++() {
            curIdx++;
            return *this;
        }
        ComponentPoolConstIterator operator++(int) {
            ComponentPoolConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        ComponentPoolConstIterator<const Entity, instance_type&> operator*() const {
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

    template <typename Type, typename Entity, typename Allocator = std::allocator<typename traits::to_instance_t<Type>>>
    class DefaultComponentPool : public sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
        using alloc_traits = std::allocator_traits<Allocator>;
    public:
        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using super = sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;
        using instance_type = typename traits::to_instance_t<Type>;
        
        std::tuple<instance_type&> get_as_tuple(const Entity& entt) {
            return std::forward_as_tuple(get(entt));
        }
        std::tuple<const instance_type&> get_as_tuple(const Entity& entt) const {
            return std::forward_as_tuple(get(entt));
        }
        instance_type& get(const Entity& entt) {
            return components[(size_t)traits_type::to_entity(super::sparse_get(entt))];
        }

        void emplace(const value_type& entt, instance_type&& arg) {
            super::emplace(entt);
            components.emplace_back(std::forward<instance_type>(arg));
        };

        bool remove(const Entity& entt) override {
            using std::swap;
            static_assert(std::is_swappable_v<instance_type>, "Error");

            if (!super::contains(entt)) return false;

            CONTAINER_DEBUG_LOG("Removed from component pool : " << std::to_string(traits_type::to_entity(entt)));

            const auto idx = (size_t)this->sparse_get(entt);
            auto out = super::remove(entt);
            if (!out) return false;

            swap(components[idx], components.back());

            components.pop_back();
            return true;
        }

        using iterator = ComponentPoolIterator<Entity, instance_type>;
        using const_iterator = ComponentPoolConstIterator<Entity, instance_type>;
        iterator begin() {
            return iterator(&this->packed, &components, 0);
        }
        const_iterator begin() const {
            return const_iterator(&this->packed, &components, 0);
        }
        iterator end() {
            return iterator(&this->packed, &components, this->packed.size());
        }
        const_iterator end() const {
            return const_iterator(&this->packed, &components, 0);
        }
    private:
        vector<instance_type> components;
    };

    template <typename Type, typename Entity, typename Allocator = std::allocator<typename traits::to_instance_t<Type>>>
    class MemoryOptimizedComponentPool : public sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
        using alloc_traits = std::allocator_traits<Allocator>;
    public:
        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using super = sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;

        using instance_type = typename traits::to_instance_t<Type>;

        std::tuple<instance_type&> get_as_tuple(const Entity& entt) {
            return std::forward_as_tuple(get(entt));
        }
        std::tuple<const instance_type&> get_as_tuple(const Entity& entt) const {
            return std::forward_as_tuple(get(entt));
        }
        // -------------------------------------------------
        // Container pool interface
        instance_type& get(const Entity& entt) {
            return components[(size_t)get_index(entt)];
        }
        bool contains(const Entity& entt) {
            for (const auto& x : this->packed) if (x == entt) return true;
            return false;
        }
        void emplace(const Entity& entt, instance_type&& arg) {
            this->packed.emplace_back(entt);
            components.emplace_back(std::forward<instance_type>(arg));
        };
        bool remove(const Entity& entt) override {
            if (!contains(entt)) return false;

            CONTAINER_DEBUG_LOG("Removed from component pool : " << to_string(entt));

            const auto idx = (size_t)get_index(entt);
            std::swap(this->packed[idx], this->packed.back());
            std::swap(components[idx], components.back());
            this->packed.pop_back();
            components.pop_back();
            return true;
        }
        // -------------------------------------------------
        using iterator = ComponentPoolIterator<Entity, instance_type>;
        using const_iterator = ComponentPoolConstIterator<Entity, instance_type>;
        iterator begin() {
            return iterator(&this->packed, &components, 0);
        }
        const_iterator begin() const {
            return const_iterator(&this->packed, &components, 0);
        }
        iterator end() {
            return iterator(&this->packed, &components, this->packed.size());
        }
        const_iterator end() const {
            return const_iterator(&this->packed, &components, 0);
        }
    private:
        inline size_t get_index(const Entity& entt) {
            for (size_t i = 0; i < this->packed.size(); i++) if (entt == this->packed[i]) return i;
            assert(false, "get_index should guarantee the presence of data.");
        }
        vector<instance_type> components;
    };

    template <typename Type, typename Entity, typename Allocator = std::allocator<typename traits::to_instance_t<Type>>>
    class FlagComponentPool : public sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
        using alloc_traits = std::allocator_traits<Allocator>;
    public:
        using traits_type = LEapsGL::entity_traits<Entity>;
        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using super = sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;
        using packed_type = typename super::packed_type;

        struct Iterator {
        private:
            int curIdx;
            vector<Entity>* packed;

        public:
            Iterator(vector<Entity>* _packed, int idx = 0) : packed{ _packed }, curIdx(idx) {};
            Iterator& operator++() {
                curIdx++;
                return *this;
            }
            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }
            tuple<const Entity> operator*() {
                return std::make_tuple((*packed)[curIdx]);
            }
            bool operator==(const Iterator& rhs) const noexcept {
                return rhs.curIdx == curIdx && rhs.packed == packed;
            }

            bool operator!=(const Iterator& rhs) const noexcept {
                return !operator==(rhs);
            }
        };

        struct ConstIterator {
        private:
            int curIdx;
            vector<Entity>* packed;

        public:
            ConstIterator(vector<Entity>* _packed, int idx = 0) : packed{ _packed }, curIdx(idx) {};
            ConstIterator& operator++() {
                curIdx++;
                return *this;
            }
            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }
            std::tuple<const Entity> operator*() const {
                return std::make_tuple((*packed)[curIdx]);
            }
            bool operator==(const ConstIterator& rhs) const noexcept {
                return rhs.curIdx == curIdx && rhs.packed == packed;
            }

            bool operator!=(const ConstIterator& rhs) const noexcept {
                return !operator==(rhs);
            }
        };

        using iterator = Iterator;
        using const_iterator = ConstIterator;

        iterator begin() {
            return iterator(&this->packed, 0);
        }
        const_iterator begin() const {
            return const_iterator(&this->packed, 0);
        }
        iterator end() {
            return iterator(&this->packed, this->packed.size());
        }
        const_iterator end() const {
            return const_iterator(&this->packed, 0);
        }
        // -------------------------------------------------
    };

    //
    //template <typename Entity, typename Value, typename GetHash, typename GetEqual>
    //class ContextContainer : public ContainerBase <Entity> {
    //    using traits_type = LEapsGL::entity_traits<Entity>;
    //    using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
    //    using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
    //    using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
    //    using super = sparse_array<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;

    //private:       
    //    std::unordered_map<Entity, Value, GetHash, GetEqual> context;
    //};

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


    template <typename ... ComponentPoolTypes>
    struct W_ComponentPool {};

    template <typename ... FilterPools>
    struct Filter {};

    template <typename...>
    class View;

    template <typename... ComponentPoolTypes, typename... Filters>
    class View<W_ComponentPool<ComponentPoolTypes...>, Filter<Filters...>>{
    private:
        using Entity = std::common_type_t<typename ComponentPoolTypes::value_type...>;
        using super_type = std::common_type_t<typename ComponentPoolTypes::super...>;

        using value_type = typename LEapsGL::entity_traits<Entity>::value_type;
        using entity_type = typename LEapsGL::entity_traits<Entity>::entity_type;
        using version_type = typename LEapsGL::entity_traits<Entity>::version_type;
        using traits_type = LEapsGL::entity_traits<Entity>;

        template<typename T>
        static constexpr std::size_t index_of = type_index<T, std::tuple <typename ComponentPoolTypes::instance_type...>>::value;

        // The set with the smallest element
        const super_type* view;
        void setSmallestComponentPoolPointer() noexcept {
            view = std::get<0>(containers_);
            std::apply([this](auto *...components) {((this->view = components->size() < this->view->size() ? components : this->view), ...); }, containers_);
            if constexpr (sizeof...(Filters) > 0) std::apply([this](auto *...filters) {((this->view = filters->size() < this->view->size() ? filters : this->view), ...); }, filters_); 
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
        View(ComponentPoolTypes*... val, Filters*... filter) : containers_(std::tie(val...)), filters_(std::tie(filter...)) {
            setSmallestComponentPoolPointer();
        }

        template <typename T, typename... Others>
        decltype(auto) get(const Entity& entt) {
            return get<index_of<T>, index_of<Others>...>(entt);
        }

        template <typename Fun>
        void each(Fun fn) {
            this->view ? this->pick_and_each(fn, std::index_sequence_for<ComponentPoolTypes...>{}, std::index_sequence_for<Filters...>{}) : void();
        }

        template <typename Fun, std::size_t... Index, std::size_t... FilterIndex>
        void pick_and_each(Fun& fn, std::index_sequence<Index...> sequence, std::index_sequence<FilterIndex...> filter_sequence) const {
            ((this->view == std::get<Index>(containers_) ? this->each<Index, true>(fn, sequence, filter_sequence) : void()), ...);
            ((this->view == std::get<FilterIndex>(filters_) ? this->each<FilterIndex, false>(fn, sequence, filter_sequence) : void()), ...);
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

        template <std::size_t BaseIndex, bool IS_COMPONENT_VIEW, typename Fun, std::size_t... Index, std::size_t... FilterIndex>
        void each(Fun fn, std::index_sequence<Index...>, std::index_sequence<FilterIndex...>) const {
            if constexpr (IS_COMPONENT_VIEW) {
                for (const auto items : *std::get<BaseIndex>(containers_)) {
                    if (const auto entt = std::get<0>(items); ((BaseIndex == Index || std::get<Index>(containers_)->contains(entt)) && ...) && (std::get<FilterIndex>(filters_)->contains(entt) && ...)) {
                        if constexpr (std::is_invocable<decltype(fn), std::add_lvalue_reference<ComponentPoolTypes::instance_type>::type...>::value) {
                            std::apply(fn, std::tuple_cat(this->dispatch_get<BaseIndex, Index>(items)...));
                        }
                        else {
                            std::apply(fn, std::tuple_cat(std::forward_as_tuple(entt), this->dispatch_get<BaseIndex, Index>(items)...));
                        }
                    }
                }
            }
            else {
                for (const auto items : *std::get<BaseIndex>(filters_)) {
                    if (const auto entt = std::get<0>(items); ((std::get<Index>(containers_)->contains(entt)) && ...) && ((BaseIndex == FilterIndex || std::get<FilterIndex>(filters_)->contains(entt)) && ...)) {
                        if constexpr (std::is_invocable<decltype(fn), std::add_lvalue_reference<ComponentPoolTypes::instance_type>::type...>::value) {
                            std::apply(fn, std::tuple_cat(this->dispatch_get<sizeof...(Index), Index>(items)...));
                        }
                        else {
                            std::apply(fn, std::tuple_cat(std::forward_as_tuple(entt), this->dispatch_get<sizeof...(Index), Index>(items)...));
                        }
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
        std::tuple<Filters*...> filters_;
    };

    enum class ComponentPoolType {
        Default, Dynamic, MemoryOptimized, Flag
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
        template <typename Type, typename Entity>
        struct ComponentPoolSelector<ComponentPoolType::Flag, Type, Entity> {
            using type = FlagComponentPool<Type, Entity>;
            static constexpr ComponentPoolType value = ComponentPoolType::Flag;
        };

        // Default Handler : 타입이 어떤 option을 가지는지 (entity X)
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
            static constexpr ComponentPoolType value = LEapsGL::__internal::template __component_pool_select_handler<Type>::value;
            using selector = typename __internal::ComponentPoolSelector<value, Type, Entity>;

            using BaseType = ContainerBase<Entity>;
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

    struct BaseDispatcher {
        virtual ~BaseDispatcher() {};
        virtual void send() = 0;
    };

    template <typename ... DispatchTag>
    struct EventQueue {
        using DispatchPtr = std::shared_ptr<BaseDispatcher>;

        template<typename E, typename T>
        using first_elem = E;

        template<typename T>
        static constexpr std::size_t index_of = type_index<T, std::tuple <DispatchTag...>>::value;

        using base_type = std::common_type<DispatchTag...>;
        
    public:
        template <typename Tag>
        void emplace(shared_ptr<BaseDispatcher> dispatcher) {
            std::get<index_of<Tag>>(queues).push(dispatcher);
        }

        template <typename Tag>
        void sendAll() {
            auto& q = std::get<index_of<Tag>>(queues);
            while (!q.empty()) {
                q.front()->send();
                q.pop();
            }
        }
    private:
        std::tuple<first_elem<std::queue<DispatchPtr>, DispatchTag>...> queues;
    };

    // Component...
    namespace ComponentType {
        using MemoryOptimized = LEapsGL::opt::IOptionComponentPoolType<LEapsGL::opt::ComponentPoolType::MemoryOptimized>;
        using Flag = LEapsGL::opt::IOptionComponentPoolType<LEapsGL::opt::ComponentPoolType::Flag>;
    }

}
