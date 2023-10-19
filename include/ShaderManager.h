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
#include "Object.h"
#include "Color.h"
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Type.h>
#include <EngineConfigure.h>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <Timer.h>

using namespace std;

#define PRINT_NAME(name) (#name)

#define SHADER_PROGRAM_DEBUG_LOG_ON

#ifdef SHADER_PROGRAM_DEBUG_LOG_ON
#define SHADER_PROGRAM_DEBUG_LOG(message) \
    std::cout << message << "\n";
#else
#define SHADER_PROGRAM_DEBUG_LOG(message)
#endif


static char infoLog[1024];
namespace LEapsGL {
    class ShaderManager;
    enum ShaderObjectSource {
        FROM_FILE, FROM_BINARY, FROM_ENTITY
    };
    struct ShaderObjectDescription {
        struct ShaderObjectDescriptionHash {
            size_t operator()(const ShaderObjectDescription& desc) const noexcept {
                return desc.getHash();
            }
        };

        ShaderObjectDescription(GLuint _type, const char* _name, ShaderObjectSource _sourceType = FROM_FILE) : type(_type), identifier(_name), sourceType(_sourceType) {
        };
        ShaderObjectDescription() = default;
        GLuint type = 0;
        PathString identifier;
        ShaderObjectSource sourceType = FROM_FILE;

        size_t getHash() const noexcept {
            size_t h = 0;
            LEapsGL::hash_combine(h, type, identifier.c_str(), sourceType);
            return h;
        }
        bool operator==(const ShaderObjectDescription& other) const {
            return (type == other.type) &&
                (sourceType == other.sourceType) &&
                (identifier == other.identifier);
        }
    };

    const char* GetShaderTypeName(GLenum type) {
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

    std::string ReadFile(const char* filePath)
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


    /**
    * @class ShaderObject
    * @brief Represents an OpenGL shader object.
    *
    * This class encapsulates the functionality for an OpenGL shader object.
    * It provides constructors, copy/move semantics, and methods for shader management.
    */
    class ShaderObject {
    public:
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
         */
        ShaderObject(ShaderObjectDescription desc)
            : shaderID(0), type(desc.type), compiled(false), keepMemory(false) {
        }

        /**
         * @brief Copy constructor.
         *
         * Creates a new shader object by copying the source and type from another shader object.
         *
         * @param other The shader object to copy.
         */
        ShaderObject(const ShaderObject& other) = delete;
        //    : type(other.type), source(other.source), keepMemory(false), compiled(false) {
        //    this->shaderID = glCreateShader(type);
        //}

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
        ShaderObject& operator=(ShaderObject rhs) = delete;
        /*{
            swap(*this, rhs);
            return *this;
        }*/

        /**
         * @brief Move assignment operator.
         *
         * Moves the contents of another shader object.
         *
         * @param rhs The shader object to move.
         * @return A reference to the current shader object.
         */
        ShaderObject& operator=(ShaderObject&& rhs) noexcept {
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
        void DeleteShader() {
            if (shaderID != 0) {
                glDeleteShader(shaderID);
            }
            shaderID = 0;
            compiled = false;
        }


        int AssignShaderID() {
            DeleteShader();
            this->shaderID = glCreateShader(type);
            this->compiled = false;
            return this->shaderID;
        }

        friend void swap(ShaderObject& lhs, ShaderObject& rhs) {
            using std::swap;
            swap(lhs.shaderID, rhs.shaderID);
            swap(lhs.type, rhs.type);
            swap(lhs.source, rhs.source);
            swap(lhs.compiled, rhs.compiled);
            swap(lhs.keepMemory, rhs.keepMemory);
        }

        void setSourceCode(string _source) { source = _source; };

        bool IsCompiled() const {
            return compiled;
        }
        GLuint GetID() const {
            return shaderID;
        }


        void setKeepMemory(bool val) {
            keepMemory = val;
        }
        bool getKeepMemory() const{
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
                SHADER_PROGRAM_DEBUG_LOG(string("Compiled: ") << shaderID);
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

    /*
 This class, ShaderProgram, manages multiple Shader Objects and handles Shader Program activation.
 The Use method activates the Shader Program, and if it's not linked yet, it performs Linking before activation.
 The Linking status is tracked by the linked variable. The Link method is responsible for Linking the Shader Progra

 If any subscribed shader object undergoes changes, it updates 'linked' to false.

 Note that the shaderProgram operates independently from the ECS system.
 */

    using ShaderProgramObject = FixedSizeHashString<SHADER_ShaderProgramNameMaxLen>;
    class ShaderProgram {
    public:
        friend void swap(ShaderProgram& lhs, ShaderProgram& rhs) {
            using std::swap;
            swap(lhs.id, rhs.id);
            swap(lhs.name, rhs.name);
            swap(lhs.descriptors, rhs.descriptors);
            swap(lhs.linked, rhs.linked);
        }

        void deleteProgram() {
            if (id != 0) {
                glDeleteProgram(id);
            }
            linked = false;
        }
        int CreateShaderProgram() {
            deleteProgram();
            return id = glCreateProgram();
        }

        inline void attach(GLuint shaderID) {
            glAttachShader(id, shaderID);
        }

        bool link() {
            int state;
            linked = true;
            glLinkProgram(id);
            glGetProgramiv(id, GL_LINK_STATUS, &state);

            if (!state) {
                glGetProgramInfoLog(id, sizeof(infoLog), NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
                linked = false;
                deleteProgram();
            }

            return linked;
        }

        ShaderProgram() : id(0), linked(false) {
        }
        ShaderProgram(ShaderProgramObject c) : id(0), linked(false), name(c) {
        }
        // Copy Constructor
        ShaderProgram(const ShaderProgram& other) = delete;
        // Move constructor
        ShaderProgram(ShaderProgram&& other) noexcept : ShaderProgram() {
            swap(*this, other);
        }
        ShaderProgram& operator=(ShaderProgram other) = delete;
        ShaderProgram& operator=(ShaderProgram&& other) noexcept {
            swap(*this, other);
            return *this;
        }

        virtual ~ShaderProgram() {
            deleteProgram();
        }

        inline void use() {
            glUseProgram(id);
        }

    private:
        friend ShaderManager;
        ShaderProgramObject name;
        GLuint id = 0; // Program ID
        vector<ShaderObjectDescription> descriptors;
        bool linked;
    };

    class ShaderManager {
    public:
        // Client side request function
        ShaderManager() = default;

        ShaderProgramObject getOrCreateShaderProgram(const string& name) {
            ShaderProgramObject shaderProgramObject(name.c_str());
            const auto iter = shaderProgramMap.find(shaderProgramObject.getHash());
            if (iter == shaderProgramMap.end()) shaderProgramMap.emplace(shaderProgramObject.getHash(), ShaderProgram(shaderProgramObject));
            return shaderProgramObject;
        }
        ShaderProgramObject CreateShaderProgram(const string& name) {
            ShaderProgramObject shaderProgramObject(name.c_str());
            if (shaderProgramMap.find(shaderProgramObject.getHash()) != shaderProgramMap.end()) throw std::runtime_error(name + " is not found in shaderProgramMap");
            shaderProgramMap.insert({ shaderProgramObject.getHash(), ShaderProgram(shaderProgramObject) });
            return shaderProgramObject;
        }
        void UseShaderProgram(const ShaderProgramObject& name) {
            auto& shaderProgram = shaderProgramMap[name.getHash()];
            if (!shaderProgram.linked) {
                shaderProgram.CreateShaderProgram();
                for (const auto& desc : shaderProgram.descriptors) {
                    shaderProgram.attach(getShaderObjectID(desc));
                }
                shaderProgram.link();
            }

            usedID = shaderProgram.linked ? shaderProgram.id : 0;
            shaderProgram.use();
        }

        void memoryClear() {
            for (auto& x : shaderObjects) if (!x.getKeepMemory()) x.DeleteShader();
        }
        void activateKeepMemory(ShaderObjectDescription desc) {
            auto& handler = getOrCreateHandler(desc);
            shaderObjects[handler.index].setKeepMemory(true);
        }
        template <typename Container = vector<ShaderObjectDescription>>
        void activateKeepMemory(Container descList) {
            for (auto& x : descList) activateKeepMemory(x);
        }

        void deactiveateKeepMemory(ShaderObjectDescription desc) {
            auto& handler = getOrCreateHandler(desc);
            shaderObjects[handler.index].setKeepMemory(false);
            checkAndRemoveShaderObject(desc);
        }
        template <typename Container = vector<ShaderObjectDescription>>
        void deactiveateKeepMemory(Container descList) {
            for (auto& x : descList) deactiveateKeepMemory(x);
        }


//
//                /**
//                 * @brief Assign a shader object description to a shader program using different parameter types.
//                 * @param shaderProgram The shader program or name to which the shader object will be assigned.
//                 * @param desc The description of the shader object to assign.
//                 * @param name The name or descriptor of the shader program.
//                 *
//                 * The `Assign` function assigns a shader object to a shader program. You can use different
//                 * parameter types such as a `ShaderProgram`, a program name as a string, or a program
//                 * descriptor to specify the shader program.
//                 *
//                 * Note: This function is not frequently called.
//                 *
//                 * @code
//                 * ShaderProgram program("ExampleProgram");
//                 * ShaderObjectDescription description("example_shader.glsl");
//                 * Assign(program, description);
//                 * Assign("ExampleProgram", description);
//                 * Assign(ShaderProgramName::Example, description);
//                 * @endcode
//                 */
        void AssignDescription(ShaderProgram& shaderProgram, const ShaderObjectDescription desc) 
        {
            auto& shaderProgramDescriptors = shaderProgram.descriptors;
            if (std::find(shaderProgramDescriptors.begin(), shaderProgramDescriptors.end(), desc)
                != shaderProgramDescriptors.end()) return;

            shaderProgramDescriptors.push_back(desc);
            auto& handler = getOrCreateHandler(desc);
            handler.usedProgram.push_back(shaderProgram.name);
        }
        void AssignDescription(const ShaderProgramObject& name, const ShaderObjectDescription desc) {
            auto iter = shaderProgramMap.find(name.getHash());
            if (iter == shaderProgramMap.end()) throw runtime_error("There is no shader program: " + string(name.c_str()));
            AssignDescription(iter->second, desc);
        }

        //template <typename ShaderObjectDescriptionContainer = vector<ShaderObjectDescription>>
        //void AssignDescription(const ShaderProgram& shaderProgram, const ShaderObjectDescriptionContainer& items) {
        //    for (auto iter = items.begin(); iter != items.end(); ++iter) AssignDescription(shaderProgram, *iter);
        //}
        template <typename ShaderObjectDescriptionContainer = vector<ShaderObjectDescription>>
        void AssignDescription(const ShaderProgramObject& desc, const ShaderObjectDescriptionContainer& items) {
            auto& shaderProgram = shaderProgramMap[desc.getHash()];
            for(const auto& item : items) AssignDescription(shaderProgram, item);
        }

        template <class T>
        void SetUniform(const int id, const GLint location, const string& name, const T& value) {
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
                glUniform4fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
            }
            else if constexpr (std::is_same<T, double>::value) {
                glUniform1f(location, static_cast<GLfloat>(value));
            }
            else {
                cout << stripped_type_name<T>() << " type is not defined.";
//                static_assert(false, "Unsupported type for setUniform");   
            }
        }

        template <class T>
        void SetUniform(const ShaderProgramObject& shaderProgramIdentifier, string& name, const T& value) {
            const ShaderProgram& program = shaderProgramMap[shaderProgramIdentifier.getHash()];
            auto id = program.id;
            GLint location = glGetUniformLocation(id, name.c_str());
            SetUniform(id, location, name, value);
        }

        template <class T>
        void SetUniform(const string& name, const T& value) {
            auto id = usedID;
            GLint location = glGetUniformLocation(id, name.c_str());
            SetUniform(id, location, name, value);
        }

        
        void DestroyShaderObject(ShaderObjectDescription desc) {
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) return;
            auto& handler = iter->second;
            shaderObjects[handler.index].DeleteShader();
        };
        void RelinkToShaderProgram(ShaderObjectDescription desc) {
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) return;
            auto& handler = iter->second;
            for (const auto & prog_name : handler.usedProgram) shaderProgramMap[prog_name.getHash()].linked = false;
        }

    private:

        struct Handler {
            int index;
            vector<ShaderProgramObject> usedProgram;
        };

        void checkAndRemoveShaderObject(const ShaderObjectDescription & desc) {
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) return;
            auto& handler = iter->second;
            if (!shaderObjects[handler.index].getKeepMemory() && handler.usedProgram.size() == 0) {
                auto x = shaderDescriptions.back();
                shaderDescriptionMap[x].index = handler.index;
                swap(shaderDescriptions[handler.index], shaderDescriptions.back());
                swap(shaderObjects[handler.index], shaderObjects.back());
                shaderDescriptions.pop_back();
                shaderObjects.pop_back();
                shaderDescriptionMap.erase(desc);
            }
        }

        ShaderObject GenerateShaderObject(const ShaderObjectDescription& shaderObjectDescription) {
            ShaderObject shaderObject(shaderObjectDescription);
            shaderObject.AssignShaderID();
            // todo
            switch (shaderObjectDescription.sourceType)
            {
            case LEapsGL::FROM_FILE:
                shaderObject.setSourceCode(ReadFile(shaderObjectDescription.identifier.c_str()));
                break;
            case LEapsGL::FROM_BINARY:
                throw std::runtime_error("Unimplemented enum value: FROM_BINARY");
                break;
            case LEapsGL::FROM_ENTITY:
                // entity => engine..
                throw std::runtime_error("Unimplemented enum value: FROM_ENTITY");
                break;
            }
            return shaderObject;
        }
        Handler& getOrCreateHandler(const ShaderObjectDescription desc){
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) {
                iter = shaderDescriptionMap.emplace(desc, Handler()).first;
                shaderDescriptions.push_back(desc);
                shaderObjects.push_back(GenerateShaderObject(desc));
                iter->second.index = shaderObjects.size() - 1;
            }
            return iter->second;
        }
        GLuint getShaderObjectID(const ShaderObjectDescription& desc) {
            auto& handler = getOrCreateHandler(desc);
            ShaderObject& shaderObject = shaderObjects[handler.index];
            int returnID = shaderObject.GetID();
            if (!shaderObject.IsCompiled()) shaderObject.Compile() ? returnID = shaderObject.GetID() : returnID = 0;
            return returnID;
        }

    private:
        // Current State
        GLuint usedID = 0;

        // ShaderManager Component Access Functions
        unordered_map<size_t, ShaderProgram> shaderProgramMap;  ///> Shader program mapper

        // unordred <shaderObjectDescription, option>
        unordered_map<ShaderObjectDescription, Handler, ShaderObjectDescription::ShaderObjectDescriptionHash> shaderDescriptionMap;  ///> Sparse map (Shader description to shader map)
        vector<ShaderObjectDescription> shaderDescriptions;                    ///> Packed description vector
        vector<ShaderObject> shaderObjects;                                    ///> Packed shader object vector
    };
}