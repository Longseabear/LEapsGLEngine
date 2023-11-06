#pragma once
// @ref entt framework: entity.hpp
// for modern c++ study;
// 

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "EngineConfigure.h"
#include <tuple>

namespace LEapsGL {
    namespace __internal {
        template<typename Type>
        constexpr int popcount(Type value) noexcept {
            return value ? (int(value & 1) + popcount(value >> 1)) : 0;
        }

        template<typename, typename = void>
        struct __entity_traits;
        
        template<typename Type>
        struct __entity_traits <Type, std::enable_if_t<std::is_enum_v<Type>>>
            :__entity_traits<std::underlying_type_t<Type>> {
            using value_type = Type;
        };

        template<typename Type>
        struct __entity_traits<Type, std::enable_if_t<std::is_class_v<Type>>>
            : __entity_traits<typename Type::entity_type> {
            using value_type = Type;
        };

        template <>
        struct __entity_traits<std::uint32_t> {
            using value_type = std::uint32_t;
            using entity_type = std::uint32_t;
            using version_type = std::uint16_t;

            static constexpr entity_type entity_mask = 0xFFFFF;
            static constexpr entity_type version_mask = 0xFFF;
            static constexpr entity_type invalid = 0xFFFFF;
        };

        template <>
        struct __entity_traits<std::uint64_t> {
            using value_type = std::uint64_t;
            using entity_type = std::uint64_t;
            using version_type = std::uint32_t;
            
            static constexpr entity_type entity_mask = 0xFFFFFFFF;
            static constexpr entity_type version_mask = 0xFFFFFFFF;
            static constexpr entity_type invalid = 0xFFFFFFFF;
        };
    }


    template <typename Traits>
    class basic_entity_traits {
        static constexpr auto length = __internal::popcount(Traits::entity_mask);

        static_assert(Traits::entity_mask && ((typename Traits::entity_type{ 1 } << length) == (Traits::entity_mask + 1)), "Invalid entity mask");
        static_assert((typename Traits::entity_type{ 1 } << __internal::popcount(Traits::version_mask)) == (Traits::version_mask + 1), "Invalid version mask");

    public:
        using value_type = typename Traits::value_type;
        using entity_type = typename Traits::entity_type;
        using version_type = typename Traits::version_type;

        /*! @brief Entity mask size. */
        static constexpr entity_type entity_mask = Traits::entity_mask;
        /*! @brief Version mask size */
        static constexpr entity_type version_mask = Traits::version_mask;
        static constexpr entity_type invalid = Traits::invalid;

        // ---------------------------------------- interface
        [[nodiscard]] static constexpr entity_type to_integral(const value_type value) noexcept {
            return static_cast<entity_type>(value);
        }
        [[nodiscard]] static constexpr entity_type to_entity(const value_type value) noexcept {
            return (to_integral(value) & entity_mask);
        }
        [[nodiscard]] static constexpr version_type to_version(const value_type value) noexcept {
            return static_cast<version_type>(to_integral(value) >> length);
        }
        [[nodiscard]] static constexpr value_type construct(const entity_type entity, const version_type version) noexcept {
            return value_type{ (entity & entity_mask) | (static_cast<entity_type>(version) << length) };
        }
        [[nodiscard]] static constexpr value_type next_version(const value_type value) noexcept {
            const auto vers = to_version(value) + 1;
            return construct(to_entity(value), static_cast<version_type>(vers + (vers == version_mask)));
        }
        [[nodiscard]] static constexpr value_type set_entity(const value_type value, const entity_type entity = invalid) noexcept {
            return construct(to_entity(entity), static_cast<version_type>(to_version(value)));
        }
        [[nodiscard]] static constexpr value_type set_version(const value_type value, const version_type version = invalid) noexcept {
            return construct(to_entity(value), static_cast<version_type>(to_version(version)));
        }
        [[nodiscard]] static constexpr bool is_valid(const value_type value) noexcept {
            return to_entity(value) != invalid;
        }
        [[nodiscard]] static constexpr value_type reset(const value_type value) noexcept {
            return construct(to_entity(invalid), static_cast<version_type>(to_version(next_version(value))));
        }
        [[nodiscard]] static constexpr bool is_same(const value_type lhs, const value_type rhs) noexcept {
            return is_valid(lhs) && is_valid(rhs) && lhs == rhs;
        }
    };


    template<typename Type>
    struct entity_traits : basic_entity_traits<__internal::__entity_traits<Type>> {
        using base_type = basic_entity_traits<__internal::__entity_traits<Type>>;
        static constexpr std::size_t page_size = PAGE_SIZE;
    };

    template<typename Entity>
    [[nodiscard]] constexpr typename entity_traits<Entity>::value_type to_integral(const Entity value) noexcept {
        return entity_traits<Entity>::to_integral(value);
    }
    template<typename Entity>
    [[nodiscard]] constexpr typename entity_traits<Entity>::entity_type to_entity(const Entity value) noexcept {
        return entity_traits<Entity>::to_entity(value);
    }
    template<typename Entity>
    [[nodiscard]] constexpr typename entity_traits<Entity>::version_type to_version(const Entity value) noexcept {
        return entity_traits<Entity>::to_version(value);
    }


    struct null_entity {
        template<typename Entity>
        [[nodiscard]] constexpr operator Entity() const noexcept {
            using traits_type = entity_traits<Entity>;
            constexpr auto value = traits_type::construct(traits_type::invalid, traits_type::invalid);
            return value;
        }
        [[nodiscard]] constexpr bool operator==([[maybe_unused]] const null_entity other) const noexcept {
            return true;
        }
        [[nodiscard]] constexpr bool operator!=([[maybe_unused]] const null_entity other) const noexcept {
            return false;
        }
        template<typename Entity>
        [[nodiscard]] constexpr bool operator==(const Entity entity) const noexcept {
            using traits_type = entity_traits<Entity>;
            return traits_type::to_entity(entity) == traits_type::to_entity(*this);
        }
        template<typename Entity>
        [[nodiscard]] constexpr bool operator!=(const Entity entity) const noexcept {
            return !(entity == *this);
        }
    };

    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity, const null_entity other) noexcept {
        return other.operator==(entity);
    }
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity, const null_entity other) noexcept {
        return !(other == entity);
    }

    inline constexpr null_entity null{};

    enum class entity : uint64_t;
}
