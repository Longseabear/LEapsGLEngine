#pragma once
#include <cstring>
#include <EngineConfigure.h>
namespace LEapsGL {
    /*
    * Type system Setting
    */

    template<typename Type>
    [[nodiscard]] constexpr auto stripped_type_name() noexcept {
        std::string_view pretty_function{ __MY_PRETTY_FUNCTION_SIGNITURE };
        auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(__MY_PRETTY_FUNCTION_START) + 1);
        auto value = pretty_function.substr(first, pretty_function.find_last_of(__MY_PRETTY_FUNCTION_END) - first);
        return value;
    }

    template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
    [[nodiscard]] constexpr std::string_view type_name(int) noexcept {
        constexpr auto value = stripped_type_name<Type>();
        return value;
    }

    template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
    [[nodiscard]] constexpr auto get_type_hash() {
        unsigned int h = 0;
        for (auto x : stripped_type_name<Type>()) h = 65599 * h + x;
        return h ^ (h >> 16);
    }

    inline void hash_combine(std::size_t& seed) { }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        hash_combine(seed, rest...);
    }

    // -----------------------
    template <int MaxSize>
    class FixedString {
    public:
        constexpr static size_t MAX_LEN = MaxSize - 1;
        FixedString() {
            _len = 0;
            _data[MaxSize - 1] = 0;
        }
        FixedString(const char* rhs) {
            _len = 0;
            while (_len < MAX_LEN && (_data[_len++] = *rhs++));
        }
        friend void swap(FixedString<MaxSize>& lhs, FixedString<MaxSize>& rhs) {
            using std::swap;
            for (int i = 0; i < MAX_LEN; i++) swap(lhs._data[i], rhs._data[i]);
            swap(lhs._len, rhs._len);
        }
        FixedString(const FixedString<MaxSize>& rhs) {
            _len = rhs._len;
            std::memcpy(_data, rhs._data, MAX_LEN);
        }
        FixedString& operator=(FixedString<MaxSize> rhs) {
            swap(*this, rhs);
            return *this;
        }
        const char* c_str() const {
            return _data;
        }
        size_t size() {
            return _len;
        }
        size_t length() {
            return _len;
        }
        char& operator[](size_t index) {
            if (index < _len) {
                return _data[index];
            }
            else {
                throw std::out_of_range("Index out of range");
            }
        }
        bool operator==(const FixedString<MaxSize>& other) const {
            return strcmp(_data, other._data) == 0;
        }

    private:
        char _data[MaxSize];
        size_t _len;
    };

    using PathString = FixedString<256>;

    /**
     * @brief A class that extends FixedString to store a fixed-size string and calculate its hash value.
     *
     * @tparam MaxSize The maximum size of the string (in bytes).
     */
    template <int MaxSize>
    class FixedSizeHashString : public FixedString<MaxSize> {
    public:
        struct HashedFixedStringHash {
            size_t operator()(const FixedSizeHashString<MaxSize>& str) const {
                return str.getHash();
            }
        };

        struct HashedFixedStringEqual {
            bool operator()(const FixedSizeHashString<MaxSize>& str1, const FixedSizeHashString<MaxSize>& str2) const {
                return str1.getHash() == str2.getHash();
            }
        };


        /**
         * @brief Default constructor. Initializes an empty string with a hash value of 0.
         */
        FixedSizeHashString() : FixedString<MaxSize>() {
            _hash = 0;
        }

        /**
         * @brief Constructor that initializes the string with the provided C-style string and calculates the hash.
         *
         * @param rhs The C-style string to initialize the object with.
         */
        FixedSizeHashString(const char* rhs) : FixedString<MaxSize>(rhs) {
            updateHash();
        }

        /**
         * @brief Copy constructor. Creates a copy of another HashedFixedString object.
         *
         * @param rhs The object to copy.
         */
        FixedSizeHashString(const FixedSizeHashString<MaxSize>& rhs) : FixedString<MaxSize>(rhs) {
            _hash = rhs._hash;
        }

        /**
         * @brief Assignment operator. Assigns the value of another HashedFixedString object.
         *
         * @param rhs The object to assign.
         * @return The assigned object.
         */
        FixedSizeHashString& operator=(FixedSizeHashString<MaxSize> rhs) {
            FixedString<MaxSize>::operator=(rhs);
            _hash = rhs._hash;
            return *this;
        }

        /**
         * @brief Get the hash value of the stored string.
         *
         * @return The hash value.
         */
        size_t getHash() const {
            return _hash;
        }

    private:
        size_t _hash; ///< The hash value of the stored string.

        /**
         * @brief Calculate and update the hash value of the stored string.
         */
        void updateHash() {
            std::hash<std::string> hasher;
            _hash = hasher(this->c_str());
        }
    };
}