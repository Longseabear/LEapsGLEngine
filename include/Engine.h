#pragma once

#include <glad/glad.h>
#include <string>
#include <map>

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

    //class ComponentExample {
    //    using entity_type = uint64_t; 
    //    using container_type = ...; /*Defualt: Dynamic Component Pool*/
    //        using instance_type = ...;
    //        using dependency_type = ...;
    // };
    
};

