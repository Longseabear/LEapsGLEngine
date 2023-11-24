#pragma once

#include "core/World.h"
#include "core/entity.h"
#include "core/Container.h"
#include "core/System.h"
#include "core/Type.h"


#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace LEapsGL {

    struct GLFWContext : LEapsGL::IContext {
        void init(uint16_t width, uint16_t height, std::string TitleName) {
            glfwInit();

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            this->window = glfwCreateWindow(width, height, TitleName.c_str(), NULL, NULL);
            if (window == NULL) {
                std::cout << "window create error" << std::endl;
                glfwTerminate();
                exit(-1);
            }

            glfwMakeContextCurrent(window);
        }
        inline GLFWwindow* getWindow() noexcept {
            return window;
        }
        void setWindow(GLFWwindow* window) {
            this->window = window;
        }
        inline uint16_t getWidth() noexcept{
            return width;
        }
        inline uint16_t getHeight() noexcept {
            return height;
        }
        void updateViewport() {
            glViewport(0, 0, width, height);
        }
        void updateViewport(uint16_t width, uint16_t height) {
            this->width = width;
            this->height = height;
            glViewport(0, 0, width, height);
        }
    private:
        GLFWwindow* window;
        uint16_t width, height;
    };

    namespace event {
        /**
         * @brief Represents an event containing mouse position.
         */
        struct MousePositionEvent {
            float xpos, ypos;
        };
        /**
         * @brief Represents an event containing the delta (change) in mouse position.
         */
        struct MousePositionDeltaEvent {
            float xoffset, yoffset;
        };
        /**
         * @brief Represents an event containing the delta (change) in mouse position.
         */
        struct MouseScrollEvent {
            float xoffset, yoffset;
        };


        /*
         * @brief view port change
         */
        struct FrameBufferSizeChangeEvent {
            int width, height;
        };
    }

    template <class GLEventSystem>
    struct GLEventSystemTrait {
        template <typename Fun>
        static void Activate(Fun callback) {
            if (!GLEventSystemTrait::activated) {
                auto& glfw = Context::getGlobalContext<GLFWContext>();
                auto& system = Context::getGlobalContext<GLEventSystem>();
                system.Configure();
                system.Start();
                callback(glfw.getWindow(), GLEventSystem::callback);

                GLEventSystemTrait::activated = true;
            }
        }
        template <typename Fun>
        static void Deactivate(Fun callback) {
            if (GLEventSystemTrait::activated) {
                auto& glfw = Context::getGlobalContext<GLFWContext>();
                auto& system = Context::getGlobalContext<GLEventSystem>();

                callback(glfw.getWindow(), NULL);
                system.Unconfigure();
                GLEventSystemTrait::activated = false;
            }
        }
        inline static bool activated = false; ///< Flag indicating whether the system is activated.
    };

    /**
     * @brief A template class representing a mouse event system.
     *
     * @tparam World The world type associated with the system.
     */
    struct MouseEventSystem : public BaseSystem, EventSubscriber<event::MousePositionEvent>, IContext {

        using Self = MouseEventSystem;
        using Event = event::MousePositionEvent;

        using MouseDeltaEvent = event::MousePositionDeltaEvent;

        /**
         * @brief Callback function for mouse position events.
         *
         * @param window The GLFW window.
         * @param xposIn The x-coordinate of the mouse position.
         * @param yposIn The y-coordinate of the mouse position.
         */
        static void callback(GLFWwindow* window, double xposIn, double yposIn) {
            float xpos = static_cast<float>(xposIn);
            float ypos = static_cast<float>(yposIn);

            Universe::emit<Event>(Event{ xpos, ypos });
        }

        /**
         * @brief Activates the mouse event system.
         *
         * @param window The GLFW window.
         */
        static void Activate() {
            GLEventSystemTrait<Self>::Activate(glfwSetCursorPosCallback);
            
        }

        /**
         * @brief Deactivates the mouse event system.
         *
         * @param window The GLFW window.
         */
        static void Deactivate() {
            GLEventSystemTrait<Self>::Deactivate(glfwSetCursorPosCallback);
        }

        // BaseSystem overrides

        /**
         * @brief Configures the mouse event system.
         */
        virtual void Configure() override {
            Universe::subscribe<Event>(this);
        }

        /**
         * @brief Unconfigures the mouse event system.
         */
        virtual void Unconfigure() override {
            Universe::unsubscribeAll(this);
        }


        /**
         * @brief Initializes the mouse event system.
         */
        virtual void Start() override {
            firstMouse = true;
            lastEvent.xpos = lastEvent.ypos = 0;
        };

        /**
         * @brief Updates the mouse event system.
         */
        virtual void Update() override {
        };


        /**
         * @brief Receives and processes mouse position events.
         *
         * @param event The received mouse position event.
         */
        virtual void receive(const Event& event) override
        {
            if (firstMouse)
            {
                lastEvent = event;
                firstMouse = false;
            }

            float xoffset = event.xpos - lastEvent.xpos;
            float yoffset = lastEvent.ypos - event.ypos; // reversed since y-coordinates go from bottom to top

            lastEvent = event;
            Universe::emit<MouseDeltaEvent>(MouseDeltaEvent{ xoffset, yoffset });
        }

    private:
        bool firstMouse; ///< Flag indicating whether it's the first mouse event.
        Event lastEvent; ///< The last recorded mouse position event.
    };

    struct ScrollEventSystem : public BaseSystem, IContext {
        using Self = ScrollEventSystem;
        using Event = event::MouseScrollEvent;

        static void callback(GLFWwindow* window, double xoffset_, double yoffset_)
        {
            float xoffset = static_cast<float>(xoffset_);
            float yoffset = static_cast<float>(yoffset_);

            Universe::emit<Event>(Event{ xoffset, yoffset });
        }


        static void Activate() {
            GLEventSystemTrait<Self>::Activate(glfwSetScrollCallback);
        }

        /**
         * @brief Deactivates the mouse event system.
         *
         * @param window The GLFW window.
         */
        static void Deactivate() {
            GLEventSystemTrait<Self>::Deactivate(glfwSetScrollCallback);
        }

        // BaseSystem overrides

        /**
         * @brief Configures the mouse event system.
         */
        virtual void Configure() override {
        }

        /**
         * @brief Unconfigures the mouse event system.
         */
        virtual void Unconfigure() override {
            Universe::unsubscribeAll(this);
        }


        /**
         * @brief Initializes the mouse event system.
         */
        virtual void Start() override {
        };

        /**
         * @brief Updates the mouse event system.
         */
        virtual void Update() override {
        };
    };

    struct FrameBufferSizeEventSystem : public BaseSystem, IContext {
        using Self = FrameBufferSizeEventSystem;
        using Event = event::FrameBufferSizeChangeEvent;

        static void callback(GLFWwindow* window, int width, int height)
        {
            Universe::emit<Event>(Event { width, height });
            Context::getGlobalContext<GLFWContext>().updateViewport(width, height);
        }


        static void Activate() {
            GLEventSystemTrait <Self> ::Activate(glfwSetFramebufferSizeCallback);
        }

        /**
         * @brief Deactivates the mouse event system.
         *
         * @param window The GLFW window.
         */
        static void Deactivate() {
            GLEventSystemTrait<Self>::Deactivate(glfwSetFramebufferSizeCallback);
        }

        // BaseSystem overrides

        /**
         * @brief Configures the mouse event system.
         */
        virtual void Configure() override {
        }

        /**
         * @brief Unconfigures the mouse event system.
         */
        virtual void Unconfigure() override {
            Universe::unsubscribeAll(this);
        }


        /**
         * @brief Initializes the mouse event system.
         */
        virtual void Start() override {
        };

        /**
         * @brief Updates the mouse event system.
         */
        virtual void Update() override {
        };
    };
}