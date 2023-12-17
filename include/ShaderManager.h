#pragma once

/**
 * @brief Shader management and compilation.
 *
 * This file contains declarations for the ShaderManager class, which is responsible for managing and compiling shader programs in an OpenGL application.
 *
 * @class ShaderManager
 * @brief Manages shader programs and shader objects.
 *
 * The ShaderManager class is responsible for managing shader programs and shader objects
 * in an OpenGL application. It provides functions for shader compilation, linking, and
 * program management.
 *
 * - ShaderManager
 *   - Controls the lifecycle of shader objects.
 *   - Manages shader program linking, updating, and removal.
 *   - Supervises shader program management with unique names.
 *   - Manages Shader Object supervision using a sparse set.
 *   - Handles removal of ShaderObjects when they lose their links.
 * - Shader Program
 *   - Encapsulates shader programs and provides a simple interface for clients.
 *   - Supports dynamic loading and recompilation of shaders.
 * - Shader Object
 *   - Represents individual shader objects.
 * - Struct ShaderObjectDescriptor
 *   - Defines shader object descriptors with maintenance times and file-based descriptors.
 *
 * Key Operations:
 * - Dynamic load is supported when ShaderObjectDescriptor has check_update = true.
 * - ShaderManager updates shader objects and performs recompilation when needed.
 * - Shader Program linking is performed when used, and recompilation is triggered for Shader Objects with compiled set to false.
 * - Shader Objects are removed when they are no longer needed, often due to Shader Program changes.
 * - The goal is to simplify ShaderManager by focusing on Shader Program loading.
 */


#include <glad/glad.h>
#include <iostream>
#include "Object.h"
#include "Color.h"
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <core/Type.h>
#include <EngineConfigure.h>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <Timer.h>
#include <functional>
#include "core/World.h"

#include <core/Container.h>
#include <core/entity.h>
#include <core/Proxy.h>


using namespace std;

#define PRINT_NAME(name) (#name)

#define SHADER_PROGRAM_DEBUG_LOG_ON true

#ifdef SHADER_PROGRAM_DEBUG_LOG_ON
#define SHADER_PROGRAM_DEBUG_LOG(message) \
    std::cout << message << "\n";
#else
#define SHADER_PROGRAM_DEBUG_LOG(message)
#endif

inline char infoLog[1024];
namespace LEapsGL {
    class ShaderManager;

    //enum class ShaderProgramEntity : std::uint64_t {};
    using ShaderProgramEntity = std::uint64_t;
    constexpr size_t ShaderProgramInvalid = 0;

    inline const char* GetShaderTypeName(GLenum type) {
        switch (type)
        {
        case GL_VERTEX_SHADER:
            return PRINT_NAME(GL_VERTEX_SHADER);
        case GL_FRAGMENT_SHADER:
            return PRINT_NAME(GL_FRAGMENT_SHADER);
        default:
            break;
        }
        return "Unknown Shader Type";
    }

    inline std::string ReadFile(const char* filePath)
    {
        std::string content;
        std::ifstream fileStream(filePath, std::ios::in);

        if (!fileStream.is_open()) {
            throw std::runtime_error("Could not read file: " + string(filePath) + ". File does not exist.");
        }

        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        fileStream.close();

        return buffer.str();
    }

    /* Shader Object */
    namespace __internal {
        class ShaderObject {
        public:
            struct ShaderObjectGroup{};
            using entity_type = LEapsGL::ProxyEntity<ShaderObjectGroup>;
            using RequestorType = LEapsGL::ProxyRequestor<ShaderObject>;

            /**
             * @brief Default constructor.
             *
             * Creates an uninitialized shader object.
             */
            ShaderObject() : shaderID(0), type(0), source(""), compiled(false), keepMemory(false) {};

            /**
             * @brief Constructor for creating a shader of the specified type.
             *
             * @param shaderType The type of shader (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
             */
            ShaderObject(GLenum shaderType)
                : shaderID(0), type(shaderType), compiled(false), keepMemory(false) {
            }

            /**
             * @brief Constructor for creating a shader of the specified type.
             *
             * @param shaderType The type of shader (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
             * @param _source source_code(string).
             */
            ShaderObject(GLenum shaderType, std::string _source)
                : shaderID(0), type(shaderType), source(_source), compiled(false), keepMemory(false) {
            }

            /**
             * @brief Copy constructor.
             *
             * Creates a new shader object by copying the source and type from another shader object.
             *
             * @param other The shader object to copy.
             */
            ShaderObject(const ShaderObject& other) : shaderID(0), type(other.type), compiled(false), keepMemory(other.compiled) {
            }

            /**
             * @brief Move constructor.
             *
             * Creates a new shader object by moving the contents of another shader object.
             *
             * @param rhs The shader object to move.
             */
            ShaderObject(ShaderObject&& rhs) noexcept : ShaderObject() {
                swap(*this, rhs);
            }

            /**
             * @brief Copy assignment operator.
             *
             * Copies the source and type from another shader object.
             *
             * @param rhs The shader object to copy.
             * @return A reference to the current shader object.
             */
            ShaderObject& operator=(ShaderObject rhs)
            {
                swap(*this, rhs);
                return *this;
            }


            /**
             * @brief Destructor.
             *
             * Destroys the shader object and releases associated resources.
             */
            virtual ~ShaderObject() {
                DeleteShader();
            }

            /**
             * @brief Deletes the shader object.
             *
             * Releases resources associated with the shader object.
             */
            void DeleteShader() noexcept {
                if (shaderID != 0) {
                    glDeleteShader(shaderID);
                }
                shaderID = 0;
                compiled = false;
            }

            friend void swap(ShaderObject& lhs, ShaderObject& rhs) noexcept {
                using std::swap;
                swap(lhs.shaderID, rhs.shaderID);
                swap(lhs.type, rhs.type);
                swap(lhs.source, rhs.source);
                swap(lhs.compiled, rhs.compiled);
                swap(lhs.keepMemory, rhs.keepMemory);
            }

            void SetSourceCode(const string& _source) noexcept { source = _source; };
            bool IsCompiled() const {
                return compiled;
            }
            GLuint GetID() const {
                return shaderID;
            }

            GLuint assureID() {
                if (compiled && shaderID != 0) return shaderID;
                if (Compile()) return shaderID;
                return 0;
            }

            void setKeepMemory(bool val) {
                keepMemory = val;
            }
            bool getKeepMemory() const {
                return keepMemory;
            }

            bool Compile() {
                DeleteShader();
                shaderID = glCreateShader(type);

                const char* sourcePtr = source.c_str();
                glShaderSource(shaderID, 1, &sourcePtr, nullptr);
                glCompileShader(shaderID);

                GLint compileStatus;
                glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);

                if (compileStatus == GL_TRUE) {
                    compiled = true;
                    SHADER_PROGRAM_DEBUG_LOG(string("Compiled shaderID:") << shaderID);
                }
                else {
                    GLint logLength;
                    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
                    std::vector<char> logMessage(logLength);
                    glGetShaderInfoLog(shaderID, logLength, nullptr, logMessage.data());
                    std::cerr << "Compile fail!! COMPILATION_FAILED " << GetShaderTypeName(type) << " \n" << logMessage.data() << std::endl;
                    compiled = false;
                }
                return compiled;
            }

            /*Specification*/
            struct ShaderObjectSpecification : public LEapsGL::ProxyRequestSpecification<ShaderObject> {
                GLuint type;
            };
            struct ShaderObjectFromFileSpecification : public ShaderObjectSpecification {
            public:
                // Required::
                // ---------------------------------------------
                using component_type = ShaderObject;
                // ---------------------------------------------
                using instance_type = traits::to_instance_t<component_type>; // Type of object to create

                PathString path;

                virtual instance_type generateInstance() const {
                    return ShaderObject(type, ReadFile(path.c_str()));
                }
                virtual size_t hash() override
                {
                    size_t h = LEapsGL::HASH_RANDOM_SEED;
                    LEapsGL::hash_combine(h, type, path);

                    // Implement an object creation method for a given specification
                    return h;
                }
            };
            struct ShaderObjectFromSourceSpecification : public ShaderObjectSpecification {
            public:
                // Required::
                // ---------------------------------------------
                using component_type = ShaderObject;
                // ---------------------------------------------
                using instance_type = traits::to_instance_t<component_type>; // Type of object to create

                std::string source_code;

                virtual instance_type generateInstance() const {
                    return ShaderObject(type, source_code);
                }
                virtual size_t hash() override
                {
                    size_t h = LEapsGL::HASH_RANDOM_SEED;
                    LEapsGL::hash_combine(h, type, source_code);

                    // Implement an object creation method for a given specification
                    return h;
                }
            };

            struct Factory {
                static auto from_file(const PathString& path, GLuint type) {
                    ShaderObjectFromFileSpecification spec;
                    spec.path = path;
                    spec.type = type;
                    return LEapsGL::ProxyTraits::Get<ShaderObjectFromFileSpecification>(spec);
                }
                static auto from_source_code(const string& source_code, GLuint type) {
                    ShaderObjectFromSourceSpecification spec;
                    spec.source_code = source_code;
                    spec.type = type;
                    return LEapsGL::ProxyTraits::Get<ShaderObjectFromSourceSpecification>(spec);
                }

            };
            

        private:
            //        friend ShaderManager;
            /**
                * @brief Get the OpenGL ID of the shader object.
                *
                * @return The OpenGL ID of the shader object.
                */
            GLuint shaderID; ///< The unique identifier of the shader used in OpenGL.
            GLenum type; ///< The type of the shader (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
            std::string source; ///< The string containing the source code of the shader.
            bool compiled;   ///< A flag indicating whether the shader has been compiled (true if compiled, false otherwise).
            bool keepMemory;  ///< A flag indicating whether memory should be retained (true if memory should be retained, false otherwise).
        };
    }

    using ShaderObjectFactory = __internal::ShaderObject::Factory;

    /* Shader Program */

    class ShaderProgram {
    public:
        struct ShaderProgramGroup {};
        using RequestorType = LEapsGL::ProxyRequestor<ShaderProgram>;

        friend void swap(ShaderProgram& lhs, ShaderProgram& rhs) noexcept {
            using std::swap;
            swap(lhs.programID, rhs.programID);
            swap(lhs.shaderObjects, rhs.shaderObjects);
        }

        /**
         * @brief Attaches a shader to the program.
         *
         * @param shaderID The ID of the shader to attach.
         */
        inline void attach(GLuint shaderID) const {
            glAttachShader(programID, shaderID);
        }

        /**
         * @brief Default constructor.
         *
         * Creates an uninitialized shader program.
         */
        ShaderProgram() : programID(0) {}
        /**
         * @brief Constructor with a ShaderProgramObject.
         *
         * @param c The ShaderProgramObject to use as the program's name.
         */


        // Copy Constructor
        ShaderProgram(const ShaderProgram& rhs) noexcept :ShaderProgram() {
        }

        // Move constructor
        ShaderProgram(ShaderProgram&& other) noexcept : ShaderProgram() {
            swap(*this, other);
        }
        ShaderProgram& operator=(const ShaderProgram& other) = delete;

        /**
         * @brief Move assignment operator.
         *
         * Moves the contents of another shader program.
         *
         * @param other The shader program to move.
         * @return A reference to the current shader program.
         */
        ShaderProgram& operator=(ShaderProgram&& other) noexcept {
            swap(*this, other);
            return *this;
        }

        /**
         * @brief Destructor.
         *
         * Destroys the shader program and releases associated resources.
         */
        virtual ~ShaderProgram() {
            deleteProgram();
        }


        /**
         * @brief Deletes the shader program and releases associated resources.
         */
        void deleteProgram() {
            if (programID != 0) glDeleteProgram(programID);
            programID = 0;
        }

        /**
         * @brief Links the shader program.
         *
         * @return True if linking is successful, false otherwise.
         */
        GLuint link() {
            int state;
            if (programID != 0) deleteProgram();

            programID = glCreateProgram();

            for (const auto& req : shaderObjects) {
                auto& obj = Proxy::assure(req);
                attach(obj.assureID());
            }

            glLinkProgram(programID);
            glGetProgramiv(programID, GL_LINK_STATUS, &state);

            if (!state) {
                glGetProgramInfoLog(programID, sizeof(infoLog), NULL, infoLog);

                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
                deleteProgram();
                return false;
            }

            SHADER_PROGRAM_DEBUG_LOG(string("Linked program id:") << programID);

            return programID;
        }

        void resetLinked() {
            deleteProgram();
        };
        bool isLinked() { return programID != 0; };
        GLuint getProgramID() {
            return programID;
        }
        vector<__internal::ShaderObject::RequestorType>& getShaderObjects() {
            return shaderObjects;
        }
        void setShaderObjects(const vector<__internal::ShaderObject::RequestorType>& reqs) {
            shaderObjects = reqs;
        }
        // User Interface
        void use() {
            if (programID == 0) link();
            glUseProgram(programID);
        }

        /**
         * @brief Set a uniform value for the specified shader program.
         *
         * Sets a uniform value for the specified shader program using its ID, the uniform location, and the uniform name.
         *
         * @param id The ID of the shader program.
         * @param location The uniform location.
         * @param name The name of the uniform.
         * @param value The value to set for the uniform.
         * @tparam T The data type of the uniform value.
         */
        template <class T>
        void SetUniform(const GLint location, const T& value) {
            if constexpr (std::is_same<T, int>::value) {
                glUniform1i(location, static_cast<GLint>(value));
            }
            else if constexpr (std::is_same<T, float>::value) {
                glUniform1f(location, static_cast<GLfloat>(value));
            }
            else if constexpr (std::is_same<T, bool>::value) {
                glUniform1i(location, (int)value);
            }
            else if constexpr (std::is_same<T, glm::mat4>::value) {
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
            }
            else if constexpr (std::is_same<T, glm::vec3>::value) {
                glUniform3fv(location, 1, glm::value_ptr(value));
            }
            else if constexpr (std::is_same<T, glm::vec4>::value) {
                glUniform3fv(location, 1, glm::value_ptr(value));
            }
            else if constexpr (std::is_same<T, double>::value) {
                glUniform1f(location, static_cast<GLfloat>(value));
            }
            else if constexpr (std::is_same<T, unsigned int>::value) {
                glUniform1f(location, static_cast<GLuint>(value));
            }
            else {
                cout << stripped_type_name<T>() << " type is not defined. (SetUniform Fail)";
                //                static_assert(false, "Unsupported type for setUniform");   
            }
        }

        /**
         * @brief Set a uniform value for the currently used shader program.
         *
         * Sets a uniform value for the currently used shader program using the uniform name and value.
         *
         * @param name The name of the uniform.
         * @param value The value to set for the uniform.
         * @tparam T The data type of the uniform value.
         */
        template <class T>
        void SetUniform(const string& name, const T& value) {
            GLint location = glGetUniformLocation(programID, name.c_str());
            if constexpr (SHADER_PROGRAM_DEBUG_LOG_ON) if (location == -1) SHADER_PROGRAM_DEBUG_LOG("[WANING] location = -1 detected: " << name << "\n");
            this->SetUniform(location, value);
        }


        /*Specification*/
        struct ShaderProgramSpecification : public LEapsGL::ProxyRequestSpecification<ShaderProgram> {
        public:
            // Required::
            using component_type = ShaderProgram;
            // ---------------------------------------------
            using instance_type = traits::to_instance_t<component_type>; // Type of object to create
                            // ---------------------------------------------

            LEapsGL::ObjectNameType name;

            virtual instance_type generateInstance() const {
                return ShaderProgram();
            }
            virtual size_t hash() override
            {
                size_t h = LEapsGL::HASH_RANDOM_SEED;
                LEapsGL::hash_combine(h, name);

                // Implement an object creation method for a given specification
                return h;
            }
        };

        struct Factory {
            static RequestorType from_name(const LEapsGL::ObjectNameType name) {
                ShaderProgramSpecification spec;
                spec.name = name;
                return LEapsGL::ProxyTraits::Get<ShaderProgramSpecification>(spec);
            }
        };

    private:
        GLuint programID;
        vector<__internal::ShaderObject::RequestorType> shaderObjects;
    };

    /**
     * @brief The ShaderManager class handles the management and interaction with Shader Program Objects.
     *
     * The ShaderManager class is responsible for managing Shader Program Objects (ShaderProgramObject).
     * Clients interact with this class to create, delete, and assign descriptions to shader program objects.
     * The ShaderManager maintains internal state based on these interactions. 
     */
    class ShaderManager : public IContext{
    public:
        using traits_type = entity_traits<ShaderProgramEntity>;
        using entity_type = entity_traits<ShaderProgramEntity>::entity_type;
        using version_type = entity_traits<ShaderProgramEntity>::version_type;

        /**
         * @brief Default constructor for the ShaderManager.
         */
        ShaderManager() = default;

        /**
         * @brief Destructor for the ShaderManager.
         *
         * Cleans up all shader program objects associated with the ShaderManager.
         */
        ~ShaderManager() {
        }

        /**
         * @brief Clear the memory of shader objects that are marked to keep memory.
         *
         * Iterates through shader objects and deletes the ones that are marked to keep memory.
         */
        void memoryClear() noexcept{
            /*auto view = Proxy::GetWorld<__internal::ShaderObject::entity_type>().view<__internal::ShaderObject>();
            view.each([](__internal::ShaderObject& shaderObject) {
                if (!shaderObject.getKeepMemory()) shaderObject.DeleteShader();
                });*/
        }

        /**
         * @brief Mark a ShaderObjectDescription to keep its memory.
         *
         * Marks the specified ShaderObjectDescription to keep its memory, preventing it from being deleted in memoryClear.
         *
         * @param desc The ShaderObjectDescription to mark.
         */
        void activateKeepMemory(const __internal::ShaderObject::RequestorType& object_ref) {
            Proxy::assure(object_ref).setKeepMemory(true);
        }
        
        /**
         * @brief Unmark a ShaderObjectDescription to release its memory.
         *
         * Unmarks the specified ShaderObjectDescription to release its memory, allowing it to be deleted in memoryClear.
         *
         * @param desc The ShaderObjectDescription to unmark.
         */
        void deactivateKeepMemory(const __internal::ShaderObject::RequestorType& object_ref) {
            Proxy::assure(object_ref).setKeepMemory(false);
        }


        void activateKeepMemory(const ShaderProgram::RequestorType& program_ref) {
            auto& program = Proxy::assure(program_ref);
            for (auto& x : program.getShaderObjects()) activateKeepMemory(x);
        }

        void deactivateKeepMemory(const ShaderProgram::RequestorType& program_ref) {
            auto& program = Proxy::assure(program_ref);
            for (auto& x : program.getShaderObjects()) deactivateKeepMemory(x);
        }

        ShaderProgram::RequestorType setShaderProgram(ObjectNameType name, const vector<__internal::ShaderObject::RequestorType>& objects) {
            auto ref = ShaderProgram::Factory::from_name(name);
            return setShaderProgram(ref, objects);
        }
        ShaderProgram::RequestorType setShaderProgram(const ShaderProgram::RequestorType& ref, const vector<__internal::ShaderObject::RequestorType>& objects) {
            auto& program = Proxy::assure(ref);
            program.deleteProgram();
            program.setShaderObjects(objects);
            return ref;
        }

        /**
         * @brief Destroy shader objects with the specified ShaderObjectDescription.
         *
         * Destroys shader objects associated with the specified ShaderObjectDescription, removing them from memory.
         *
         * @param desc The ShaderObjectDescription for which shader objects will be destroyed.
         */
        void DestroyShaderObject(const __internal::ShaderObject::RequestorType& desc) {
            auto& object = Proxy::assure(desc);
            object.DeleteShader();
        };

        ShaderProgram::RequestorType GetGlobalProgramRequestor(const ObjectNameType& name) {
            auto iter = programs.find(name);
            if (iter == programs.end()) iter = programs.emplace(name, ShaderProgram::Factory::from_name(name)).first;
            return iter->second;
        }

        void deleteShaderProgram(const ObjectNameType& name) {
            auto iter = programs.find(name);
            if (iter != programs.end()) programs.erase(iter);
        }

        ShaderProgram::RequestorType GetProgramRequestor(const ObjectNameType& name) {
            return ShaderProgram::Factory::from_name(name);
        }

    private:
        std::map<ObjectNameType, ShaderProgram::RequestorType> programs;
    };
}