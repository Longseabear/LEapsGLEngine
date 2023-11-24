#include "pch.h"
#include "core/Container.h"
#include "core/entity.h"

class TestEntity {
public:
    using entity_type = std::uint32_t;
    
    uint32_t id;

    constexpr TestEntity(const entity_type& d) :id(d) {};  
    TestEntity(const TestEntity& entt) :id(entt.id) {};

    operator entity_type() const {
        return static_cast<entity_type>(id);
    }
};
class TestEntity2 {
public:
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;

    uint32_t id;

    constexpr TestEntity2(const entity_type& d) :id(d) {};
    TestEntity2(const TestEntity& entt) :id(entt.id) {};

    operator entity_type() const {
        return static_cast<entity_type>(id);
    }
};

using namespace LEapsGL;

struct Position {
    int x, y;
};
struct transpose {
    int x, y, z;
};
struct playable {
    bool value;
};

struct user_transpose : transpose {};


using ViewTestEntity = uint64_t;


template <typename Entity, typename Type, typename ComponentType = DefaultComponentPool<Type, Entity>>
struct getComponentPoolType {
    using type = ComponentType;
};


template <typename... ComponentPools>
View<ComponentPools...> makeViewAnything(ComponentPools*... args) {
    return { args... };
}

template <typename... Types>
View<DefaultComponentPool<Types, ViewTestEntity>...> makeView(DefaultComponentPool<Types, ViewTestEntity>*... args) {
    return {args...};
}

TEST(ComponentPoolTest, MemoryOptimizedContainerPoolTest) {
    using Entity = ViewTestEntity;

    using transpose_pool = DefaultComponentPool<transpose, Entity>;
    using user_transpose_pool = DefaultComponentPool<user_transpose, Entity>;

    using position_pool = MemoryOptimizedComponentPool<Position, Entity>;
    using transpose_pool = DefaultComponentPool<transpose, Entity>;
    using playable_pool = MemoryOptimizedComponentPool<playable, Entity>;

    std::map<int, shared_ptr<ContainerBase<Entity>>> M;
    M[0] = std::allocate_shared<position_pool>(std::allocator<position_pool>(), position_pool{});
    M[1] = std::allocate_shared<transpose_pool>(std::allocator<transpose_pool>(), transpose_pool{});
    M[2] = std::allocate_shared<playable_pool>(std::allocator<playable_pool>(), playable_pool{});

    auto p1 = static_cast<position_pool*>(M[0].get());
    auto p2 = static_cast<transpose_pool*>(M[1].get());
    auto p3 = static_cast<playable_pool*>(M[2].get());

    for (int i = 0; i < 5; i++) {
        Entity e = i;
        p1->emplace(e, { i, i + 1 });
        p2->emplace(e, { i + 2, i + 3, i + 4 });
        p3->emplace(e, { (bool)(i & 1) });
    }

    auto view = makeViewAnything(p1, p2, p3);
    view.each([](Position& pos, auto& trans, playable& isPlay) {
        cout << pos.x << " " << pos.y << "\n";
    cout << trans.x << trans.y << trans.z << "\n";
    cout << isPlay.value << "\n";
        });
}
TEST(ComponentPoolTest, ViewExample) {
    DefaultComponentPool<transpose, uint64_t> V;
    using Entity = ViewTestEntity;

    using position_pool = DefaultComponentPool<Position, Entity>;
    using transpose_pool = DefaultComponentPool<transpose, Entity>;
    using playable_pool = DefaultComponentPool<playable, Entity>;

    std::map<int, shared_ptr<ContainerBase<Entity>>> M;
    M[0] = std::allocate_shared<position_pool>(std::allocator<position_pool>(), position_pool{});
    M[1] = std::allocate_shared<transpose_pool>(std::allocator<transpose_pool>(), transpose_pool{});
    M[2] = std::allocate_shared<playable_pool>(std::allocator<playable_pool>(), playable_pool{});

    auto p1 = static_cast<position_pool*>(M[0].get());
    auto p2 = static_cast<transpose_pool*>(M[1].get());
    auto p3 = static_cast<playable_pool*>(M[2].get());

    for (int i = 0; i < 5; i++) {
        Entity e = i;
        p1->emplace(e, { i, i+1 });
        p2->emplace(e, { i + 2, i + 3, i + 4 });
        p3->emplace(e, { (bool)(i & 1) });
    }

    for (int i = 5; i < 10; i++) p1->emplace(Entity(i), { 100, 101 });
    for (int i = 5; i < 15; i++) p3->emplace(Entity(i), { true});
    for (int i = 5; i < 8; i+=2) p2->emplace(Entity(i), {1,1,1});
    p2->emplace(Entity(15), { 1,1,1 });
    auto view = makeView(p1, p2, p3);
    
    for (int i = 0; i < 5; i++) {
        auto& p = view.get<Position>(i);
        cout << p.x << p.y << "\n";
        p.x *= 2;
        p.y *= 2;
    }
    for (int i = 0; i < 5; i++) {
        auto& p = view.get<Position>(i);
        cout << p.x << p.y << "\n";
        p.x *= 2;
        p.y *= 2;
    }


    auto iter = view.get<Position, playable>(0);
    view.each([](Position& pos, auto& trans, playable& isPlay) {
        cout << pos.x << " " << pos.y << "\n";
        cout << trans.x << trans.y << trans.z << "\n";
        cout << isPlay.value << "\n";
    });


    view.each([](Entity entt, Position& pos, auto& trans, playable& isPlay) {
        cout << "Entity Index: " << entt << "\n";
        cout << pos.x << " " << pos.y << "\n";
        cout << trans.x << trans.y << trans.z << "\n";
        cout << isPlay.value << "\n";
    });
    auto tt = view.begin();

    for (auto entt : view) {
        cout << "[" << entt << "]\n";
        auto& t = view.get<transpose>(entt);
        t.z = 100;
    }

    for (auto entt : view) cout << view.get<transpose>(entt).z;


    

    //view.each([]() {
    //    });
}
TEST(ComponentPoolTest, SparseArrayCustomEntityTest) {

    /*
       Registery.emplace<Type>();
        for(auto entity: view) {
        // gets only the components that are going to be used ...

        auto &vel = view.get<velocity>(entity);

        vel.dx = 0.;
        vel.dy = 0.;

        // ...

        void update(std::uint64_t dt, entt::registry &registry) {
        registry.view<position, velocity>().each([dt](auto &pos, auto &vel) {
            // gets all the components of the view at once ...

            pos.x += vel.dx * dt;
            pos.y += vel.dy * dt;

            // ...
        });
    }
    }
    */
    using Entity = TestEntity;

//    auto T = std::make_shared<ComponentPool<Position, Entity>>();
//    ComponentPool<Position, Entity> T;
    
    DefaultComponentPool<transpose, Entity> V;

    using position_pool = DefaultComponentPool<Position, Entity>;

    std::map<int, shared_ptr<ContainerBase<Entity>>> M;

    auto& iter = M[0];
    iter = std::allocate_shared<position_pool>(std::allocator<position_pool>(), position_pool{});

    static_cast<position_pool*>(iter.get())->emplace(5, {1,2});
    static_cast<position_pool*>(iter.get())->emplace(6, { 1,3 });
    static_cast<position_pool*>(iter.get())->emplace(7, { 1, 4 });

    auto& pool = *static_cast<position_pool*>(iter.get());
    
    cout << "auto& x : pool\n";
    for (auto& x : pool) {
        auto entt = std::get<0>(x);
        cout << entt << ">>";
        auto& y = std::get<1>(x);
        cout << y.x << " " << y.y << "\n";
        y.x = 13;
    }

    cout << "Normal iteration\n";
    for (auto iter = pool.begin(); iter != pool.end(); iter++) {
        cout << std::get<0>(*iter) << "\n";
        cout << std::get<1>(*iter).x << " " << std::get<1>(*iter).y << "\n";
        EXPECT_TRUE(std::get<1>(*iter).x == 13);
    }
    cout << "const auto& x : pool\n";
    for (const auto& x : pool) {
        auto& entt = std::get<0>(x);
        cout << entt << ">>";
        auto& y = std::get<1>(x);
        cout << y.x << " " << y.y << "\n";
    }
    for (const auto& [entity, component] : pool){
        cout << component.x << "\n";
        cout << component.y << "\n";
        cout << entity << " " << to_string(component.x) <<" " << to_string(component.y) << "\n";
    }
    cout << "Normal iteration\n";

    vector<Entity> gt_entity;
    vector<int> gt_x;
    vector<int> gt_y;
    for (auto iter = pool.begin(); iter != pool.end(); iter++) {
        gt_entity.push_back(std::get<0>(*iter));
        gt_x.push_back(std::get<1>(*iter).x);
        gt_y.push_back(std::get<1>(*iter).y);
    }


    for (int i = 0; i < gt_entity.size(); i++) {
        auto& pos = pool.get(gt_entity[i]);
        EXPECT_EQ(pos.x, gt_x[i]);
        EXPECT_EQ(pos.y, gt_y[i]);
        pos.x = i;
        pos.y = i + 10;
    }

    for (int i = 0; i < gt_entity.size(); i++) {
        auto& pos = pool.get(gt_entity[i]);
        EXPECT_EQ(pos.x, i);
        EXPECT_EQ(pos.y, i+10);
    }

    static_cast<position_pool*>(iter.get())->remove(5);
    static_cast<position_pool*>(iter.get())->remove(7);

    // if removed,
    EXPECT_FALSE(pool.contains(5));
    EXPECT_FALSE(pool.contains(7));


    //T.emplace(5, {1, 2});
    //T.emplace(6, { 1, 3 });
    //T.emplace(7, { 1, 4 });

    //for (auto t : T) {
    //    cout << t.id;
    //}
    //static_cast<ComponentPool<Position, Entity>*>(q[0])->remove(5);
    //static_cast<ComponentPool<Position, Entity>*>(q[0])->remove(6);
    //M[0]->remove(5);
    //M[0]->remove(6);

    //q[0]->remove(5);
    //q[0]->remove(6);
};
TEST(SparseArrayTest, SparseArrayCustomEntityTest) {
    using Entity = TestEntity;
    using Entity2 = TestEntity2;

    sparse_array<Entity> v;
    sparse_array<Entity2> v2;

    v.emplace((Entity)4097);
    v.emplace((Entity)9001);
    v.emplace((Entity)55);

    v2.emplace((Entity2)5);
    v2.emplace((Entity2)213885);
    v2.emplace((Entity2)8585);

    EXPECT_TRUE(v.contains((Entity)4097));
    EXPECT_TRUE(v.contains((Entity)9001));
    EXPECT_TRUE(v.contains((Entity)55));

    EXPECT_FALSE(v.contains(5));
    EXPECT_FALSE(v.contains(213885));
    EXPECT_FALSE(v.contains(8585));


    EXPECT_FALSE(v2.contains((Entity)4097));
    EXPECT_FALSE(v2.contains((Entity)9001));
    EXPECT_FALSE(v2.contains((Entity)55));

    EXPECT_TRUE(v2.contains(5));
    EXPECT_TRUE(v2.contains(213885));
    EXPECT_TRUE(v2.contains(8585));


    for (auto x : v) {
        cout << x.id << "\n";
    }
}
TEST(SparseArrayTest, SimpleSparseArrayTest) {
    using Entity = std::uint64_t;

    sparse_array<Entity> v;

    
    v.emplace((Entity)4097 );
    v.emplace((Entity)9001);
    v.emplace((Entity)55);

    EXPECT_TRUE(v.contains((Entity)4097));
    EXPECT_TRUE(v.contains((Entity)9001));
    EXPECT_TRUE(v.contains((Entity)55));
    
    EXPECT_FALSE(v.contains(5));
    EXPECT_FALSE(v.contains(213885));
    EXPECT_FALSE(v.contains(8585));

    for (auto x : v) {
        cout << x << "\n";
    }

    cout << "------------\n";
    v.remove(4097);
    for (auto x : v) {
        cout << x << "\n";
    }

    EXPECT_FALSE(v.contains(4097));
    v.emplace((Entity)4097);
    EXPECT_TRUE(v.contains(4097));
}