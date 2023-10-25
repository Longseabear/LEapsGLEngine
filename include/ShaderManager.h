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
#include <functional>
#include "entity.h"
using namespace std;

#define PRINT_NAME(name) (#name)

#define SHADER_PROGRAM_DEBUG_LOG_ON true

#ifdef SHADER_PROGRAM_DEBUG_LOG_ON
#define SHADER_PROGRAM_DEBUG_LOG(message) \
    std::cout << message << "\n";
#else
#define SHADER_PROGRAM_DEBUG_LOG(message)
#endif


char infoLog[1024];
namespace LEapsGL {
    class ShaderManager;
    enum ShaderObjectSource {
        FROM_FILE, FROM_BINARY, FROM_ENTITY
    };

    using ShaderProgramIdentifier = FixedString<SHADER_ShaderProgramNameMaxLen>;

    //enum class ShaderProgramEntity : std::uint64_t {};
    using ShaderProgramEntity = std::uint64_t;
    constexpr size_t ShaderProgramInvalid = 0;

    struct ShaderProgramEntityHandler {
        using tempo_type = temporary<ShaderProgramEntity, ShaderManager>;

        using entity_type = entity_traits<ShaderProgramEntity>::entity_type;
        using version_type = entity_traits<ShaderProgramEntity>::version_type;
        ShaderProgramEntityHandler(ShaderProgramIdentifier _name) :name(_name) {
            temporarl_entity = tempo_type{ (ShaderProgramEntity)null_entity {} };
        };
        ShaderProgramEntityHandler() {
            temporarl_entity = tempo_type{ (ShaderProgramEntity)null_entity {} };
        }

        ShaderProgramIdentifier name;
    private:
        friend ShaderManager;
        tempo_type temporarl_entity;
    };

    /**
     * @struct ShaderObjectDescription
     * @brief Describes an OpenGL shader object, including its type and source.
     *
     * The `ShaderObjectDescription` structure provides information about an OpenGL shader object,
     * such as its type, source, and an identifier (usually a file path or entity name).
     */
    struct ShaderObjectDescription {
        /**
         * @struct ShaderObjectDescriptionHash
         * @brief Hash functor for `ShaderObjectDescription`.
         *
         * This functor calculates a hash value for a `ShaderObjectDescription` object.
         */
        struct ShaderObjectDescriptionHash {
            size_t operator()(const ShaderObjectDescription& desc) const noexcept {
                return desc.getHash();
            }
        };

        /**
         * @brief Constructor for `ShaderObjectDescription`.
         *
         * Creates a `ShaderObjectDescription` with the specified type, identifier, and source type.
         *
         * @param _type The type of the shader (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
         * @param _name The identifier for the shader object (e.g., file path or entity name).
         * @param _sourceType The source type of the shader object (default is FROM_FILE).
         */
        ShaderObjectDescription(GLuint _type, const string & _name, ShaderObjectSource _sourceType = FROM_FILE) : type(_type), identifier(_name.c_str()), sourceType(_sourceType){
        };

        /**
         * @brief Default constructor for `ShaderObjectDescription` (deleted).
         */
        ShaderObjectDescription() = delete;

        /**
         * @brief Calculate the hash value for the `ShaderObjectDescription`.
         * @return The hash value of the shader object description.
         */
        size_t getHash() const noexcept{
            size_t h;
            LEapsGL::hash_combine(h, type, string(identifier.c_str()), sourceType);
            return h;
        }

        /**
         * @brief Check if two `ShaderObjectDescription` objects are equal.
         * @param other The `ShaderObjectDescription` to compare with.
         * @return `true` if the objects are equal, `false` otherwise.
         */
        bool operator==(const ShaderObjectDescription& other) const noexcept{
            return (type == other.type) &&
                (sourceType == other.sourceType) &&
                (identifier == other.identifier);
        }

        GLuint type = 0;
        PathString identifier;
        ShaderObjectSource sourceType = FROM_FILE;
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
     * @brief Default code generator for ShaderObjectDescription.
     *
     * This function generates the source code for a shader object based on its description.
     *
     * @param shaderObjectDescription The description of the shader object.
     * @return The source code as a string.
     *
     * This function is used as the default code generator for shader objects. It checks the source type
     * specified in the shader object description and generates the source code accordingly. If the source
     * type is FROM_FILE, it reads the source code from a file. If the source type is FROM_BINARY, it throws
     * an exception indicating that this source type is not implemented. If the source type is FROM_ENTITY,
     * it throws an exception indicating that the Entity type must implement a to_string function and accept
     * a code_generator as an argument.
     */
    string DefaultCodeGenerator(const ShaderObjectDescription& shaderObjectDescription) {
        switch (shaderObjectDescription.sourceType)
        {
        case LEapsGL::FROM_FILE:
            return ReadFile(shaderObjectDescription.identifier.c_str());
            break;
        case LEapsGL::FROM_BINARY:
            throw std::runtime_error("[Todo] Unimplemented enum value: FROM_BINARY");
            break;
        case LEapsGL::FROM_ENTITY:
            throw std::runtime_error("The Entity type must implement a to_string function and accept a code_generator as an argument");
            break;
        }
    }

    /*
    * Don use __internal namespace
    */
    namespace __internal {
        /**
        * @class ShaderObject
        * @brief Represents an OpenGL shader object.
        *
        * This class encapsulates the functionality for an OpenGL shader object.
        * It provides constructors, copy/move semantics, and methods for shader management.
        *
        * ShaderObject instances are not meant to be exposed to external clients.
        * Copying of ShaderObject instances is not allowed.
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
            /*    : shaderID(0), type(other.type), compiled(false), keepMemory(other.compiled) {
            }*/

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
            ShaderObject& operator=(const ShaderObject& rhs) = delete;
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
            void DeleteShader() noexcept {
                if (shaderID != 0) {
                    glDeleteShader(shaderID);
                }
                shaderID = 0;
                compiled = false;
            }

            friend void swap(ShaderObject& lhs, ShaderObject& rhs) noexcept{
                using std::swap;
                swap(lhs.shaderID, rhs.shaderID);
                swap(lhs.type, rhs.type);
                swap(lhs.source, rhs.source);
                swap(lhs.compiled, rhs.compiled);
                swap(lhs.keepMemory, rhs.keepMemory);
            }

            void SetSourceCode(const string & _source) noexcept{ source = _source; };
            bool IsCompiled() const {
                return compiled;
            }
            GLuint GetID() const {
                return shaderID;
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
        class ShaderProgram {
        public:
            friend void swap(ShaderProgram& lhs, ShaderProgram& rhs) noexcept{
                using std::swap;
                swap(lhs.programID, rhs.programID);
                swap(lhs.name, rhs.name);
                swap(lhs.descriptors, rhs.descriptors);
                swap(lhs.linked, rhs.linked);
            }

            /**
             * @brief Attaches a shader to the program.
             *
             * @param shaderID The ID of the shader to attach.
             */
            inline void attach(GLuint shaderID) const{
                glAttachShader(programID, shaderID);
            }

            /**
             * @brief Default constructor.
             *
             * Creates an uninitialized shader program.
             */
            ShaderProgram() : programID(0), linked(false) {}
            /**
             * @brief Constructor with a ShaderProgramObject.
             *
             * @param c The ShaderProgramObject to use as the program's name.
             */
            ShaderProgram(ShaderProgramIdentifier identifier) : programID(0), linked(false), name(identifier) {}


            // Copy Constructor
            ShaderProgram(const ShaderProgram& other) = delete;

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
                assert(programID == 0);
                deleteProgram();
            }


            // Client[ShaderManager] Side Method

            inline void use() {
                glUseProgram(programID);
            }
            /**
             * @brief Deletes the shader program and releases associated resources.
             */
            void deleteProgram() {
                if (programID != 0) glDeleteProgram(programID);
                programID = 0;
                linked = false;
            }

            /**
             * @brief Initializes a shader program by creating a new program ID.
             *
             * @return The ID of the created shader program.
             */
            int InitShaderProgram() {
                deleteProgram();
                return programID = glCreateProgram();
            }
            /**
             * @brief Links the shader program.
             *
             * @return True if linking is successful, false otherwise.
             */
            bool link() {
                int state;
                assert(!linked);

                linked = true;
                glLinkProgram(programID);
                glGetProgramiv(programID, GL_LINK_STATUS, &state);

                if (!state) {
                    glGetProgramInfoLog(programID, sizeof(infoLog), NULL, infoLog);

                    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
                    linked = false;
                    deleteProgram();
                    return false;
                }

                SHADER_PROGRAM_DEBUG_LOG(string("Linked program id:") << programID);

                return linked;
            }


            void resetLinked() { linked = false; };
            bool isLinked() { return linked; };
            GLuint getProgramID() {
                return programID;
            }
            vector<ShaderObjectDescription>& getDescriptor() {
                return descriptors;
            }

            const ShaderProgramIdentifier& getName() const {
                return name;
            }

        private:
            ShaderProgramIdentifier name;
            GLuint programID;
            vector<ShaderObjectDescription> descriptors;
            bool linked;
        };
    }

    /**
     * @brief The ShaderManager class handles the management and interaction with Shader Program Objects.
     *
     * The ShaderManager class is responsible for managing Shader Program Objects (ShaderProgramObject).
     * Clients interact with this class to create, delete, and assign descriptions to shader program objects.
     * The ShaderManager maintains internal state based on these interactions. 
     */
    class ShaderManager {
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
            while (!shaderProgramMap.empty()) {
                auto it = shaderProgramMap.begin();
                DeleteShaderProgram(it->second);
            }
        }

        
        /**
         * @brief Get or create a ShaderProgramObject with the given name.
         *
         * If a ShaderProgramObject with the specified name exists, it is returned. Otherwise, a new one is created.
         *
         * @param name The name of the ShaderProgramObject to get or create.
         * @return The ShaderProgramObject.
         */
        ShaderProgramEntityHandler getOrCreateShaderProgram(const string& name) {
            ShaderProgramEntityHandler shaderProgramEntityHandler(name.c_str());
            auto iter = shaderProgramMap.find(shaderProgramEntityHandler.name);
            if (iter != shaderProgramMap.end()) return packedEntity[iter->second];
            return CreateShaderProgram(name);
        }

        /**
         * @brief Create a new ShaderProgramObject with the given name.
         *
         * If a ShaderProgramObject with the specified name already exists, an exception is thrown.
         *
         * @param name The name of the ShaderProgramObject to create.
         * @return The newly created ShaderProgramObject.
         */
        ShaderProgramEntityHandler CreateShaderProgram(const string& name) {
            ShaderProgramEntityHandler shaderProgramEntityHandler(name.c_str());
            auto iter = shaderProgramMap.find(shaderProgramEntityHandler.name);
            if (iter != shaderProgramMap.end()) throw std::runtime_error("Already exist shader program: " + name);
            __internal::ShaderProgram program(shaderProgramEntityHandler.name);
            program.InitShaderProgram();
            const size_t newIndex = program.getProgramID();

            iter = shaderProgramMap.emplace(shaderProgramEntityHandler.name, program.getProgramID()).first;
            shaderProgramEntityHandler.temporarl_entity.data = traits_type::construct(
                newIndex, traits_type::to_version(packedEntity[newIndex].temporarl_entity.data)
            );
            packedEntity[newIndex] = shaderProgramEntityHandler;
            packedShaderProgram[newIndex] = std::move(program);

            SHADER_PROGRAM_DEBUG_LOG("[Info] Created Shader Program: " << string(shaderProgramEntityHandler.name.c_str()) << " Entity: " << traits_type::to_entity(shaderProgramEntityHandler.temporarl_entity.data) << " Version: " << traits_type::to_version(shaderProgramEntityHandler.temporarl_entity.data));

            return shaderProgramEntityHandler;
        }

        /**
         * @brief Delete a ShaderProgramObject with the given name.
         *
         * Removes the ShaderProgramObject associated with the provided name, if it exists.
         *
         * @param name The name of the ShaderProgramObject to delete.
         */
        void DeleteShaderProgram(const string& name) {
            ShaderProgramEntityHandler shaderProgramEntityHandler(name.c_str());
            auto iter = shaderProgramMap.find(shaderProgramEntityHandler.name);
            if (iter == shaderProgramMap.end()) return;
            DeleteShaderProgram(iter->second);
        }
        /**
         * @brief Use a ShaderProgramObject with the specified name.
         *
         * Activates and uses the ShaderProgramObject associated with the provided name. If the program is not linked, it will be linked and attached to its shader objects.
         *
         * @param name The name of the ShaderProgramObject to use.
         */
        void UseShaderProgram(ShaderProgramEntityHandler& shaderProgramEntityHandler) noexcept{
            size_t shaderProgramIndex = getShaderProgramIndex(shaderProgramEntityHandler);
            if (shaderProgramIndex == ShaderProgramInvalid) throw std::runtime_error("shader program not founded. Name: " + string(shaderProgramEntityHandler.name.c_str()));

            if (!packedShaderProgram[shaderProgramIndex].isLinked()) {
                packedShaderProgram[shaderProgramIndex].InitShaderProgram();

                auto oldIter = shaderProgramMap.find(shaderProgramEntityHandler.name);
                if (oldIter != shaderProgramMap.end()) {
                    const size_t oldIndex = oldIter->second;
                    if (oldIndex != 0) {
                        packedEntity[oldIndex].temporarl_entity.data = traits_type::reset(packedEntity[oldIndex].temporarl_entity.data);
                    }
                }
                __internal::ShaderProgram tmpInstance = std::move(packedShaderProgram[shaderProgramIndex]);

                const size_t newIndex = tmpInstance.getProgramID();
                shaderProgramEntityHandler.temporarl_entity.data = traits_type::set_entity(packedEntity[newIndex].temporarl_entity.data, newIndex);
                
                oldIter->second = newIndex;

                packedEntity[newIndex] = shaderProgramEntityHandler;
                packedShaderProgram[newIndex] = std::move(tmpInstance);
                shaderProgramIndex = newIndex;

                for (const auto& desc : packedShaderProgram[shaderProgramIndex].getDescriptor()) {
                    packedShaderProgram[shaderProgramIndex].attach(getShaderObjectID(desc));
                }
                packedShaderProgram[shaderProgramIndex].link();

                SHADER_PROGRAM_DEBUG_LOG("[Info] Modified Shader Program: " << string(shaderProgramEntityHandler.name.c_str()) << " Entity: " << traits_type::to_entity(shaderProgramEntityHandler.temporarl_entity.data) << " Version: " << traits_type::to_version(shaderProgramEntityHandler.temporarl_entity.data));
            }

            usedID = packedShaderProgram[shaderProgramIndex].isLinked() ? packedShaderProgram[shaderProgramIndex].getProgramID() : 0;
            packedShaderProgram[shaderProgramIndex].use();
        }


        /**
         * @brief Clear the memory of shader objects that are marked to keep memory.
         *
         * Iterates through shader objects and deletes the ones that are marked to keep memory.
         */
        void memoryClear() noexcept{
            for (auto& x : shaderObjects) if (!x.getKeepMemory()) x.DeleteShader();
        }

        /**
         * @brief Mark a ShaderObjectDescription to keep its memory.
         *
         * Marks the specified ShaderObjectDescription to keep its memory, preventing it from being deleted in memoryClear.
         *
         * @param desc The ShaderObjectDescription to mark.
         */
        void activateKeepMemory(const ShaderObjectDescription & desc) {
            auto& handler = getOrCreateHandler(desc);
            shaderObjects[handler.index].setKeepMemory(true);
        }
        /**
         * @brief Mark a ShaderProgramObject's descriptors to keep their memory.
         *
         * Marks all descriptors of the specified ShaderProgramObject to keep their memory, preventing them from being deleted in memoryClear.
         *
         * @param spo The ShaderProgramObject to mark.
         */
        void activateKeepMemory(ShaderProgramEntityHandler& shaderProgramEntityHandler) {
            activateKeepMemory(packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)].getDescriptor());
        }

        /**
         * @brief Mark a list of ShaderObjectDescriptions to keep their memory.
         *
         * Marks all ShaderObjectDescriptions in the provided container to keep their memory, preventing them from being deleted in memoryClear.
         *
         * @param descList The container of ShaderObjectDescriptions to mark.
         * @tparam Container The type of the container, defaults to vector<ShaderObjectDescription>.
         */
        template <typename Container = vector<ShaderObjectDescription>>
        void activateKeepMemory(const Container & descList) {
            for (const auto& x : descList) activateKeepMemory(x);
        }
        
        /**
         * @brief Unmark a ShaderObjectDescription to release its memory.
         *
         * Unmarks the specified ShaderObjectDescription to release its memory, allowing it to be deleted in memoryClear.
         *
         * @param desc The ShaderObjectDescription to unmark.
         */
        void deactivateKeepMemory(const ShaderObjectDescription & desc) {
            auto& handler = getOrCreateHandler(desc);
            shaderObjects[handler.index].setKeepMemory(false);
            checkAndRemoveShaderObject(desc);
        }
        /**
         * @brief Unmark a ShaderProgramObject's descriptors to release their memory.
         *
         * Unmarks all descriptors of the specified ShaderProgramObject to release their memory, allowing them to be deleted in memoryClear.
         *
         * @param spo The ShaderProgramObject to unmark.
         */
        void deactivateKeepMemory(ShaderProgramEntityHandler& shaderProgramEntityHandler) {
            deactivateKeepMemory(packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)].getDescriptor());
        }

        /**
         * @brief Unmark a list of ShaderObjectDescriptions to release their memory.
         *
         * Unmarks all ShaderObjectDescriptions in the provided container to release their memory, allowing them to be deleted in memoryClear.
         *
         * @param descList The container of ShaderObjectDescriptions to unmark.
         * @tparam Container The type of the container, defaults to vector<ShaderObjectDescription>.
         */
        template <typename Container = vector<ShaderObjectDescription>>
        void deactivateKeepMemory(Container descList) {
            for (auto& x : descList) deactivateKeepMemory(x);
        }



        /**
        * @brief Assign a shader object description to a shader program using different parameter types.
        * @param shaderProgram The shader program or name to which the shader object will be assigned.
        * @param desc The description of the shader object to assign.
        * @param name The name or descriptor of the shader program.
        *
        * The `Assign` function assigns a shader object to a shader program. You can use different
        * parameter types such as a `ShaderProgram`, a program name as a string, or a program
        * descriptor to specify the shader program.
        *
        * Note: This function is not frequently called.
        *
        * @code
        * ShaderProgramObject program = ShaderManager.CreateShaderProgram("ExampleProgram");
        * ShaderObjectDescription description("example_shader.glsl");
        * AssignDescription(program, description);
        * AssignDescription("ExampleProgram", description);
        * AssignDescription(ShaderProgramName::Example, description);
        * @endcode
        */
        void AssignDescription(__internal::ShaderProgram& shaderProgram, const ShaderObjectDescription desc)
        {
            auto& shaderProgramDescriptors = shaderProgram.getDescriptor();
            if (std::find(shaderProgramDescriptors.begin(), shaderProgramDescriptors.end(), desc)
                != shaderProgramDescriptors.end()) return;

            shaderProgramDescriptors.push_back(desc);
            auto& handler = getOrCreateHandler(desc);
            handler.usedProgram.push_back(shaderProgram.getName());
        }
        void AssignDescription(ShaderProgramEntityHandler& shaderProgramEntityHandler, const ShaderObjectDescription desc) {
            auto& shaderProgram = packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)];
            AssignDescription(shaderProgram, desc);
        }
        template <typename ShaderObjectDescriptionContainer = vector<ShaderObjectDescription>>
        void AssignDescription(ShaderProgramEntityHandler& shaderProgramEntityHandler, const ShaderObjectDescriptionContainer& items) {
            for(const auto& item : items) AssignDescription(packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)], item);
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

        /**
         * @brief Set a uniform value for a shader program identified by a ShaderProgramObject.
         *
         * Sets a uniform value for a shader program identified by a ShaderProgramObject using the uniform name and value.
         *
         * @param shaderProgramIdentifier The ShaderProgramObject that identifies the shader program.
         * @param name The name of the uniform.
         * @param value The value to set for the uniform.
         * @tparam T The data type of the uniform value.
         */
        template <class T>
        void SetUniform(ShaderProgramEntityHandler& shaderProgramEntityHandler, const string& name, const T& value) {
            auto id = packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)].getProgramID();
            GLint location = glGetUniformLocation(id, name.c_str());
            SetUniform(id, location, name, value);
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
            auto id = usedID;
            GLint location = glGetUniformLocation(id, name.c_str());
            if constexpr (SHADER_PROGRAM_DEBUG_LOG_ON) if(location == -1) SHADER_PROGRAM_DEBUG_LOG("[WANING] location = -1 detected: " << name << "\n");
            SetUniform(id, location, name, value);
        }

        /**
         * @brief Assign source code to shader objects associated with a ShaderProgramObject.
         *
         * Assigns source code to all shader objects associated with the provided ShaderProgramObject, using the specified code generator function.
         *
         * @param spo The ShaderProgramObject to which source code will be assigned.
         * @param code_generator The code generator function that generates the source code for the shader objects.
         */
        void AssignShaderObjectSourceCode(ShaderProgramEntityHandler& shaderProgramEntityHandler,
            function<string(const ShaderObjectDescription&)> code_generator = DefaultCodeGenerator) {
            AssignShaderObjectSourceCode(packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)].getDescriptor());
        }
        /**
         * @brief Assign source code to shader objects with the specified ShaderObjectDescription.
         *
         * Assigns source code to all shader objects with the specified ShaderObjectDescription, using the specified code generator function.
         *
         * @param desc The ShaderObjectDescription for which source code will be assigned.
         * @param code_generator The code generator function that generates the source code for the shader objects.
         */
        void AssignShaderObjectSourceCode(const ShaderObjectDescription& desc, 
            function<string(const ShaderObjectDescription&)> code_generator = DefaultCodeGenerator) {

            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) return;
            auto& handler = iter->second;

            shaderObjects[handler.index] = std::move(GenerateShaderObject(desc, code_generator));
        }

        /**
         * @brief Assign source code to multiple shader objects.
         *
         * Assigns source code to multiple shader objects with the provided ShaderObjectDescriptions, using the specified code generator function.
         *
         * @param descList The list of ShaderObjectDescriptions for which source code will be assigned.
         * @param code_generator The code generator function that generates the source code for the shader objects.
         * @tparam Container The type of the container, defaults to vector<ShaderObjectDescription>.
         */
        template <typename Container = vector<ShaderObjectDescription>>
        void AssignShaderObjectSourceCode(const Container& descList, function<string(const ShaderObjectDescription&)> code_generator = DefaultCodeGenerator) {
            for (const auto& x : descList) AssignShaderObjectSourceCode(x, code_generator);
        };

        /**
         * @brief Destroy shader objects associated with a ShaderProgramObject.
         *
         * Destroys shader objects associated with the provided ShaderProgramObject, removing them from memory.
         *
         * @param spo The ShaderProgramObject for which shader objects will be destroyed.
         */
        void DestroyShaderObject(ShaderProgramEntityHandler& shaderProgramEntityHandler) {
            DestroyShaderObject(packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)].getDescriptor());
        };
        /**
         * @brief Destroy shader objects with the specified ShaderObjectDescription.
         *
         * Destroys shader objects associated with the specified ShaderObjectDescription, removing them from memory.
         *
         * @param desc The ShaderObjectDescription for which shader objects will be destroyed.
         */
        void DestroyShaderObject(const ShaderObjectDescription& desc) {
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) return;
            auto& handler = iter->second;
            shaderObjects[handler.index].DeleteShader();
        };
        /**
         * @brief Destroy multiple shader objects.
         *
         * Destroys multiple shader objects associated with the provided ShaderObjectDescriptions, removing them from memory.
         *
         * @param descList The list of ShaderObjectDescriptions for which shader objects will be destroyed.
         * @tparam Container The type of the container, defaults to vector<ShaderObjectDescription>.
         */
        template <typename Container = vector<ShaderObjectDescription>>
        void DestroyShaderObject(const Container& descList) {
            for (const auto& x : descList) DestroyShaderObject(x);
        };
        /**
         * @brief Relink shader programs associated with a ShaderObjectDescription.
         *
         * Relinks shader programs associated with the provided ShaderObjectDescription. Used when changes are made to shader objects.
         *
         * @param desc The ShaderObjectDescription for which shader programs will be relinked.
         */
        void RelinkToShaderProgram(const ShaderObjectDescription& desc) {
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) return;
            auto& handler = iter->second;
            for (const auto & prog_name : handler.usedProgram) packedShaderProgram[getShaderProgramIndex(prog_name)].resetLinked();
        }

        /**
         * @brief Relink a shader program associated with a ShaderProgramObject.
         *
         * Relinks a shader program associated with the provided ShaderProgramObject. Used when changes are made to shader objects.
         *
         * @param spo The ShaderProgramObject for which the shader program will be relinked.
         */
        void RelinkToShaderProgram(ShaderProgramEntityHandler& shaderProgramEntityHandler) {
            packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)].resetLinked();
        }

        const vector<ShaderObjectDescription>& getAllDescirption(const ShaderProgramIdentifier& name) {
            return packedShaderProgram[getShaderProgramIndex(name)].getDescriptor();
        }
        const vector<ShaderObjectDescription>& getAllDescirption(ShaderProgramEntityHandler & shaderProgramEntityHandler) {
            return packedShaderProgram[getShaderProgramIndex(shaderProgramEntityHandler)].getDescriptor();
        }


    private:
        /**
         * @brief Get the index of a ShaderProgram using a ShaderProgramEntityHandler.
         *
         * This function searches for the index of a ShaderProgram using a provided ShaderProgramEntityHandler.
         * It first attempts to extract an entity from the ShaderProgramEntityHandler, compares it with stored entities,
         * and if a match is found, returns the index. If no match is found, it looks up the ShaderProgram by its name.
         * If the name is found in the map, the ShaderProgramEntityHandler's temporarl_entity is set to the found entity's temporarl_entity.
         *
         * @param shaderProgramEntityHandler The ShaderProgramEntityHandler to search for.
         * @return The index of the ShaderProgram, or 0 if not found.
         */
        size_t getShaderProgramIndex(ShaderProgramEntityHandler & shaderProgramEntityHandler) {
            entity_type idx = traits_type::to_entity(shaderProgramEntityHandler.temporarl_entity.getValue());

            if (idx != ShaderProgramInvalid) {
                auto& entity = packedEntity[idx];
                if (traits_type::is_same(entity.temporarl_entity.data, shaderProgramEntityHandler.temporarl_entity.data)) {
                    return idx;
                }
            }

            auto iter = shaderProgramMap.find(shaderProgramEntityHandler.name);
            if (iter == shaderProgramMap.end()) {
                shaderProgramEntityHandler.temporarl_entity.data = null_entity();
                return ShaderProgramInvalid;
            }
            shaderProgramEntityHandler.temporarl_entity = packedEntity[iter->second].temporarl_entity;
            return iter->second;
        }
        /**
         * @brief Get the index of a ShaderProgram using a ShaderProgramIdentifier.
         *
         * This function searches for the index of a ShaderProgram using a provided ShaderProgramIdentifier (name).
         * It looks up the ShaderProgram by its name in the map.
         * If the name is found, it returns the index; otherwise, it returns 0.
         *
         * @param name The name of the ShaderProgram to search for.
         * @return The index of the ShaderProgram, or 0 if not found.
         */
        inline size_t getShaderProgramIndex(const ShaderProgramIdentifier & name) {
            auto iter = shaderProgramMap.find(name);
            if (iter != shaderProgramMap.end()) return 0;
            return iter->second;
        }

        void DeleteShaderProgram(size_t idx) {
            auto& shaderProgram = packedShaderProgram[idx];
            for (const auto& x : packedShaderProgram[idx].getDescriptor()) {
                auto& handler = getOrCreateHandler(x);
                auto it = std::find(handler.usedProgram.begin(), handler.usedProgram.end(), shaderProgram.getName());
                if (it != handler.usedProgram.end()) {
                    handler.usedProgram.erase(it);
                    checkAndRemoveShaderObject(handler);
                }
            }
            shaderProgramMap.erase(shaderProgram.getName());
            packedShaderProgram[idx].deleteProgram();
            packedEntity[idx].temporarl_entity.data = traits_type::reset(packedEntity[idx].temporarl_entity.data);
        }
        struct Handler {
            int index;
            vector<ShaderProgramIdentifier> usedProgram;
        };

        void checkAndRemoveShaderObject(Handler& handler) {
            ShaderObjectDescription desc = shaderDescriptions[handler.index];
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
        void checkAndRemoveShaderObject(const ShaderObjectDescription & desc) {
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) return;
            auto& handler = iter->second;
            checkAndRemoveShaderObject(handler);
        }


        __internal::ShaderObject GenerateShaderObject(const ShaderObjectDescription& shaderObjectDescription,
                                          function<string(const ShaderObjectDescription&)> code_generator = DefaultCodeGenerator ) {
            __internal::ShaderObject shaderObject(shaderObjectDescription);
            shaderObject.SetSourceCode(code_generator(shaderObjectDescription));
            return shaderObject;
        }
        Handler& getOrCreateHandler(const ShaderObjectDescription desc){
            auto iter = shaderDescriptionMap.find(desc);
            if (iter == shaderDescriptionMap.end()) {
                SHADER_PROGRAM_DEBUG_LOG("[Info] Added Shader Object: " << desc.getHash() << " " << desc.identifier.c_str() << " " << desc.type << " " << desc.sourceType);
                iter = shaderDescriptionMap.emplace(desc, Handler()).first;
                shaderDescriptions.push_back(desc);
                shaderObjects.push_back(GenerateShaderObject(desc));
                iter->second.index = shaderObjects.size() - 1;
            }
            return iter->second;
        }
        GLuint getShaderObjectID(const ShaderObjectDescription& desc) {
            auto& handler = getOrCreateHandler(desc);
            __internal::ShaderObject& shaderObject = shaderObjects[handler.index];
            int returnID = shaderObject.GetID();
            if (!shaderObject.IsCompiled()) shaderObject.Compile() ? returnID = shaderObject.GetID() : returnID = 0;
            return returnID;
        }


    private:
        // Current State
        GLuint usedID = 0;

        // ShaderManager Component Access Functions
        unordered_map<ShaderProgramIdentifier, size_t, ShaderProgramIdentifier::FixedStringHashFn> shaderProgramMap;  ///> Shader program mapper
        SparseVector<__internal::ShaderProgram> packedShaderProgram;
        SparseVector<ShaderProgramEntityHandler> packedEntity;

//        entity_traits<std::uint64_t>::

        // unordred <shaderObjectDescription, option>
        unordered_map<ShaderObjectDescription, Handler, ShaderObjectDescription::ShaderObjectDescriptionHash> shaderDescriptionMap;  ///> Sparse map (Shader description to shader map)
        vector<ShaderObjectDescription> shaderDescriptions;                    ///> Packed description vector
        vector<__internal::ShaderObject> shaderObjects;                                    ///> Packed shader object vector

    };
}