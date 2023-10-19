#pragma once
#include <cstring>
#include <glad/glad.h>

namespace LEapsGL {
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
        ShaderFramework Setting
    */
    // Maximum length ShaderProgramName = default 128
    constexpr size_t SHADER_ShaderProgramNameMaxLen = 128;
    constexpr GLuint SHADER_INVALID = 0;
}
