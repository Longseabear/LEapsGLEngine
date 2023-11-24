#include "pch.h"
#include "core/World.h"
#include "core/Container.h"
#include "core/entity.h"
#include "core/Type.h"
#include "core/System.h"

struct Position {
};
struct PositionMemoryOptimized : public LEapsGL::opt::IOptionComponentPoolType<LEapsGL::opt::ComponentPoolType::MemoryOptimized>{
    int x, y;
    PositionMemoryOptimized() = default;
    PositionMemoryOptimized(int _x, int _y) :x(_x), y(_y) {};
};

using BaseSystem = LEapsGL::BaseSystem;

class ModelTransformSystem : BaseSystem {
public:
    void Start() {

    }
    void update() {
        //auto view = world->view<PositionMemoryOptimized>().each([](auto& pos) {

        //    });

    }
    LEapsGL::BaseWorld* world;
};

template<class T>
class CustomEntity {
public:
    using entity_type = std::uint32_t;

    uint32_t id;

    constexpr CustomEntity(const entity_type& d) :id(d) {};
    CustomEntity(const CustomEntity& entt) :id(entt.id) {};

    operator entity_type() const {
        return static_cast<entity_type>(id);
    }
};
struct A {};
struct B {};

enum class AC : uint64_t {} ;
enum class BC : uint64_t {};



struct ComponentA {
};
struct ComponentB {
};
struct ComponentC {
    using entity_type = uint32_t;
    using component_type = int;
};
struct ComponentD {
    using entity_type = uint32_t;
    using component_type = int;
};
TEST(WorldTest, UniverseTest) {
    auto& A = LEapsGL::Universe::GetRelativeWorld<ComponentA, ComponentB>();
    auto& B = LEapsGL::Universe::GetRelativeWorld<ComponentC, ComponentD>();
    // compile error therow
    // auto C = LEapsGL::Universe::GetRelativeWorld<ComponentA, ComponentD>();

    auto entt = B.Create();
    B.emplace<ComponentC>(entt, int(1));
}
TEST(WorldTest, TypeDefTest) {
    auto AWorld = LEapsGL::Universe::GetWorld<LEapsGL::World<AC>>();
}
TEST(WorldTest, CustomWorldTestWithEnum) {
    auto AWorld = LEapsGL::Universe::GetWorld<LEapsGL::World<AC>>();
    auto BWorld = LEapsGL::Universe::GetWorld<LEapsGL::World<BC>>();

    for (int i = 0; i < 10; i++) {
        auto entt = AWorld.Create();
        AWorld.emplace<Position>(entt, Position{});

        cout << AWorld.getEntityID(entt) << " ";
        ASSERT_TRUE(AWorld.getEntityID(entt) == i);
    }
    cout << "\n";

    for (int i = 0; i < 10; i++) {
        auto entt = BWorld.Create();
        BWorld.emplace<Position>(entt, Position{});
        cout << BWorld.getEntityID(entt) << " ";
        ASSERT_TRUE(BWorld.getEntityID(entt) == i);
    }
    cout << "\n";

    for (int i = 0; i < 10; i++) {
        AWorld.Destroy((AC)i);
    }
    for (int i = 0; i < 10; i++) {
        auto entt = AWorld.Create();
        AWorld.emplace<Position>(entt, Position{});
        cout << AWorld.getEntityID(entt) << " " << AWorld.getEntityVersion(entt) << " / ";
    }
    cout << "\n";
    for (int i = 0; i < 10; i++) {
        auto entt = BWorld.Create();
        BWorld.emplace<Position>(entt, Position{});
        cout << BWorld.getEntityID(entt) << " " << BWorld.getEntityVersion(entt) << " / ";
    }
    cout << "\n";
}

TEST(WorldTest, CustomWorldTestWithClass) {
    auto AWorld = LEapsGL::Universe::GetWorld<LEapsGL::World<CustomEntity<A>>>();
    auto BWorld = LEapsGL::Universe::GetWorld<LEapsGL::World<CustomEntity<B>>>();

    for (int i = 0; i < 10; i++) {
        auto entt = AWorld.Create();
        AWorld.emplace<Position>(entt, Position{});
        cout << entt.id << " ";
        ASSERT_TRUE(entt.id == i);
    }
    cout << "\n";

    for (int i = 0; i < 10; i++) {
        auto entt = BWorld.Create();
        BWorld.emplace<Position>(entt, Position{});
        cout << entt.id << " ";
        ASSERT_TRUE(entt.id == i);
    }
    cout << "\n";

    for (int i = 0; i < 10; i++) {
        AWorld.Destroy(i);
    }
    for (int i = 0; i < 10; i++) {
        auto entt = AWorld.Create();
        AWorld.emplace<Position>(entt, Position{});
        cout << AWorld.getEntityID(entt)  << " " << AWorld.getEntityVersion(entt) << " / ";
    }
    cout << "\n";
    for (int i = 0; i < 10; i++) {
        auto entt = BWorld.Create();
        BWorld.emplace<Position>(entt, Position{});
        cout << BWorld.getEntityID(entt) << " " << BWorld.getEntityVersion(entt) << " / ";
    }
    cout << "\n";
}

TEST(WorldTest, entity_create_destory) {
    auto world = LEapsGL::World<uint64_t>();
    using traits_type = LEapsGL::entity_traits<uint64_t>;

    auto printEntity = [&world](uint64_t f) {
        std::cout << "ID: " << world.getEntityID(f) << " Version: " << world.getEntityVersion(f)<<"\n";
    };

    vector<uint64_t> v;
    for (int i = 0; i < 10; i++) v.push_back(world.Create()), printEntity(v.back());
    EXPECT_TRUE(world.size()==10);
    cout << "\n";

    cout << "\n";
    for (int i = 0; i < 10; i++) v.push_back(world.Create()), printEntity(v.back());

    auto t = world.assureComponent<Position>();
    auto t2 = world.assureComponent<PositionMemoryOptimized>();
    const auto t3 = LEapsGL::OptionSelector<LEapsGL::opt::ComponentPoolType, PositionMemoryOptimized, uint64_t>::selector::value;
    auto t4 = world.get<Position>();

    EXPECT_TRUE(t3
        == LEapsGL::opt::ComponentPoolType::MemoryOptimized);
    auto t5 = world.get<PositionMemoryOptimized>();

    for (int i = 0; i < 10; i++) world.emplace<PositionMemoryOptimized>(v[i], PositionMemoryOptimized{1,2});
    for (int i = 0; i < 10; i += 2) {
        world.Destroy(v[i]);
        cout << "Destoryed " << v[i] << "\n";
    }

    auto view = world.view<PositionMemoryOptimized>();
    
    view.each([](const auto& entt, auto& pos) {
        EXPECT_TRUE(pos.x == 1);
        EXPECT_TRUE(pos.y == 2);
        pos.x = 1;
    });
    view.each([](auto& pos) {
        EXPECT_TRUE(pos.x == 1);
    EXPECT_TRUE(pos.y == 2);
    pos.x = 1;
        });

    // LEapsGL::option_traits<Type, ComponentPoolHandler>();
    // LEapsGL::option_traits<LEapsGL::__internal::__component_pool_select_handler
    //auto t = LEapsGL::option_traits<LEapsGL::__internal::__component_pool_select_handler<PositionMemoryOptimized>, uint64_t>::Selector<
    //    LEapsGL::opt::ComponentPoolType::MemoryOptimized, Position, uint64_t>;
//    LEapsGL::opt::OptionTraits<PositionMemoryOptimized>::;

    //EXPECT_TRUE(::pool_type == 
    //LEapsGL::ComponentPoolOption::ComponentPoolType::MemoryOptimized);
    //world.SetComponentPoolType<Position>(LEapsGL::ComponentPoolType::MemoryOptimized);
}