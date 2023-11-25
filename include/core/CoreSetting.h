#pragma once

namespace LEapsGL{

    /*
    * Type system Setting
    */
    // Maximum length Path String Size = default 256
    constexpr size_t TYPE_MaxPathStringSize = 256;

#if defined(__GNUC__) || defined(__clang__) // GCC 컴파일러에서 정의되는 매크로
#define __MY_PRETTY_FUNCTION_SIGNITURE __PRETTY_FUNCTION__
#define __MY_PRETTY_FUNCTION_START '='
#define __MY_PRETTY_FUNCTION_END ']'
#elif defined(_MSC_VER) // Microsoft Visual C++에서 정의되는 매크로
#define __MY_PRETTY_FUNCTION_SIGNITURE __FUNCSIG__
#define __MY_PRETTY_FUNCTION_START '<'
#define __MY_PRETTY_FUNCTION_END '>'
#endif

    /*
        Entity SEtting
    */
    constexpr size_t PAGE_SIZE = 4096;
    using BaseEntityType = uint64_t; // Base entity type
    
    /*
        Proxy
    */
    constexpr size_t PROXY_SEED = 18446744073709551557;
    constexpr size_t HASH_RANDOM_SEED = 18446744073709551609;

    /*
        Type
    */
    constexpr size_t STR_IDENTIFIER_SIZE = 32;
}
