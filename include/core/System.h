#pragma once

#include "entity.h"
#include "Container.h"
#include "Type.h"
#include <unordered_map>

namespace LEapsGL {
    class BaseSystem {
    public:
        virtual void Configure() = 0; /* Register event system */
        virtual void Unconfigure() = 0; /* Register event system */
        virtual void Start() = 0; 
        virtual void Update() = 0;
    };

    class DefaultSystem : public BaseSystem{
    public:
        virtual void Configure() {
        }
        virtual void Unconfigure() {
        }
        virtual void Start() {
        }
        virtual void Update() {
        }
    };

    namespace __internal {
        class BaseEventSubscriber {
        public:
            virtual ~BaseEventSubscriber() {};
        };
    }
    template<typename T>
    class EventSubscriber : public LEapsGL::__internal::BaseEventSubscriber
    {
    public:
        virtual ~EventSubscriber() {}

        /**
        * Called when an event is emitted by the world.
        */
        virtual void receive(const T& event) = 0;
    };
}