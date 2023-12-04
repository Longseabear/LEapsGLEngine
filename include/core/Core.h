#pragma once

#include <string>
#include <map>
#include <core/CoreSetting.h>
#include <core/entity.h>
#include <core/Container.h>

using namespace std;

// Workspace: To
namespace LEapsGL {
    /*
    LEapsEngine: Responsible for the overall structure and functionality of the engine. This class serves as the core component of the engine and initializes and manages various features.
        WorkspaceManager: Manages and creates Workspaces. The WorkspaceManager creates and oversees multiple Workspaces.
            Workspace: Represents a single animation frame. This class manages the visual content and components of an animation. It plays a role similar to Unity's Scene.
                Frame: Represents the content displayed for a specific duration within a workspace. Each Frame can contain multiple AnimationFrames.
                ImageProcessor: Handles image processing-related functions. This class is responsible for tasks such as image creation, editing, and filtering.
                AnimationController: Manages the flow and control of animations. It oversees the animations within each Frame and provides control over them.
                Timeline: Represents the animation frames over time and assists in combining multiple Frames and AnimationFrames to create scenarios.
    */



    class LEapsObject {};

    struct LEapsTransferData{
    };

    // all System Serializable object
    class Serializable : public LEapsObject {
        // 가상 소멸자 (virtual destructor)
        virtual ~Serializable() {}

        // 객체를 직렬화하는 가상 함수
        virtual void serialize(LEapsTransferData& data) const = 0;

        // 직렬화된 문자열을 이용해 객체를 역직렬화하는 가상 함수
        virtual void deserialize(const LEapsTransferData& data) = 0;
    };


    //   ####     ####    ##  ##   ######   ######   ##  ##   ######
    //  ##  ##   ##  ##   ### ##     ##     ##       ##  ##     ##
    //  ##       ##  ##   ######     ##     ##        ####      ##
    //  ##       ##  ##   ######     ##     ####       ##       ##
    //  ##       ##  ##   ## ###     ##     ##        ####      ##
    //  ##  ##   ##  ##   ##  ##     ##     ##       ##  ##     ##
    //   ####     ####    ##  ##     ##     ######   ##  ##     ##
    /**
     * @brief Class for handling global information in a Context
     *
     * This Context class satisfies the following characteristics:
     * 1. Operates independently of scenes.
     * 2. Maintains only a single instance.
     *
     * It is used for storing global data and providing functionality that needs to be accessed universally.
     *
     * Example usage:
     * \code
     * Context::getGlobalContext<CTX>()...
     * \endcode
     */

    // [Primary key, pointer] 
    // SubContext must always have a primary key and temperal entity.
    // It must be possible to create a temperal entity using the primary key.
    // You should be able to obtain the desired pointer component using the temperal entity.
    class IContext {};

    /*
    For global context (e.g., ShaderManager, SoundManager...)
    
    */
    class Context : public Singleton<Context> {
    public:
        template <typename CTX>
        static CTX& getGlobalContext() {
            constexpr size_t id = get_type_hash<CTX>();
            auto& ctx = Context::get_instance();
            auto iter = ctx.M.find(id);
            if (iter == ctx.M.end()) iter = ctx.M.emplace(id, new CTX{}).first;
            return *static_cast<CTX*>(iter->second);
        }
        std::unordered_map<size_t, IContext*> M;
    protected:
        Context() {};
    };
};

