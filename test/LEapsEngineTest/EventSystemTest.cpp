#include "pch.h"
#include "core/World.h"
#include "core/Container.h"
#include "core/entity.h"
#include "core/System.h"


// Milestone
// Component / System / SystemGroup
using Entity = uint64_t;

struct Position {
    float x, y, z;
};
// emit => world에게 emit 요청, 즉각 동작
// emit_after => 특정 작업이 끝나고 처리.


struct WorldEventQueueType {
    enum class A;
    enum class B;
    enum class C;
};
struct MyDispatcher : public LEapsGL::BaseDispatcher {
    MyDispatcher(int k) :a(k) {
    }
    virtual void send() override
    {
        std::cout << "Event Occur: " << a << "\n";
    }
    int a;
};

class MyEvent {
public:
    string value;
    void onEvent() const {
        cout << "Event occur: " << value << "\n";
    }
};

class DummySystem : public LEapsGL::BaseSystem {
    virtual void Configure() override
    {
    }
    virtual void Unconfigure() override {

    }
    virtual void Start() override
    {
    }
    virtual void Update() override
    {
        cout << "Dummy System Update\n";
    }
};
class EmitTestSystem : public LEapsGL::BaseSystem,public LEapsGL::EventSubscriber<MyEvent>{
public:
    void Configure() {
        // register all event....
        LEapsGL::Universe::subscribe<MyEvent>(this);
    }
    void Unconfigure() {
        LEapsGL::Universe::unsubscribe<MyEvent>(this);
    }
    void Start() {
    }
    void Update() {
        static int T = 0;
        cout << "Update time: " << to_string(T) << "\n";
        T++;

        for (int i = 0; i < 5; i++) {
            cout << "Add Event (After_System): " + to_string(i) + "\n";
            LEapsGL::Universe::emit<MyEvent, LEapsGL::EventPolish::AFTER_SYSTEM>(MyEvent{ to_string(T) + " Index: " + to_string(i) });
        }
        for (int i = 0; i < 1; i++) {
            cout << "Add Event (After_Update): " + to_string(i) + "\n";
            LEapsGL::Universe::emit<MyEvent, LEapsGL::EventPolish::AFTER_UPDATE>(MyEvent{ to_string(T) + " Index: " + to_string(i) });
        }
    }

    virtual void receive(const MyEvent& event) override
    {
        cout << "Recive Event!\n";
        event.onEvent();
    }

    LEapsGL::BaseWorld& worlds = LEapsGL::Universe::GetWorld<LEapsGL::BaseWorld>();
};



TEST(EventSystemTest, EventSystemTest) {
    auto queue = LEapsGL::EventQueue<WorldEventQueueType::A, WorldEventQueueType::B, WorldEventQueueType::C>();

    queue.emplace<WorldEventQueueType::A>(make_shared<MyDispatcher>(1));
    queue.emplace<WorldEventQueueType::A>(make_shared<MyDispatcher>(2));
    queue.emplace<WorldEventQueueType::A>(make_shared<MyDispatcher>(3));

    queue.emplace<WorldEventQueueType::B>(make_shared<MyDispatcher>(4));
    queue.emplace<WorldEventQueueType::B>(make_shared<MyDispatcher>(5));
    queue.emplace<WorldEventQueueType::B>(make_shared<MyDispatcher>(6));
    queue.emplace<WorldEventQueueType::B>(make_shared<MyDispatcher>(7));

    queue.emplace<WorldEventQueueType::C>(make_shared<MyDispatcher>(10));

    queue.sendAll<WorldEventQueueType::A>();
    queue.sendAll<WorldEventQueueType::C>();
    queue.sendAll<WorldEventQueueType::B>();
    queue.sendAll<WorldEventQueueType::B>();
    queue.sendAll<WorldEventQueueType::B>();

}
TEST(EventSystemTest, SystemEmitTest) {
    auto& world = LEapsGL::Universe::GetBaseWorld();

    const Entity entt = world.Create();
    world.emplace<Position>(entt, Position{ 0.1, 0.1, 0.1 });
    
    LEapsGL::Universe::registerSystem(new EmitTestSystem());
    LEapsGL::Universe::registerSystem(new DummySystem());

    for (int i = 0; i < 10; i++) {
        LEapsGL::Universe::Update();
        if (i == 5)LEapsGL::Universe::emit<MyEvent>(MyEvent{ "Direct" });
    }
}