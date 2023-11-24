#include "pch.h"
#include "core/Container.h"
#include "core/entity.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <core/World.h>
#include <core/Proxy.h>
#include <core/Type.h>

struct MyObject {
    int data[5];
    int* dataPointer;

    friend void swap(MyObject& lhs, MyObject& rhs) {
        swap(lhs.data, rhs.data);
        swap(lhs.dataPointer, rhs.dataPointer);
    }
    MyObject() : dataPointer(nullptr) {};
    ~MyObject() {
        if (dataPointer) 
            delete dataPointer;
    };
    MyObject(const MyObject& obj) : dataPointer(nullptr) {
        for (int i = 0; i < 5; i++) data[i] = obj.data[i];
        if(obj.dataPointer) dataPointer = new int(*obj.dataPointer);
    }


    MyObject(MyObject&& obj) : MyObject(){
        swap(*this, obj);
    }
    MyObject& operator=(MyObject obj) {
        swap(*this, obj);
        return *this;
    }

    void print() {
        for (int i = 0; i < 5; i++) cout << data[i] << " -> ";
        cout << "\n";
    }
};
// MyObject& obj = proxy::request(MySpecification);
// MyObject& obj = proxy::clone<MyObject>(MySpecification);
struct MyObjectSpecification : public LEapsGL::ProxyRequestSpecification<MyObject>{
public:
    // Required::
    // ---------------------------------------------
    using instance_type = MyObject;
//    using entity_type = 
    // ---------------------------------------------
    // Client resource
    int k = 0;

    // spec
    virtual instance_type generateInstance() const{
        instance_type myObject;
        for (int i = 0; i < 5; i++) myObject.data[i] = k + i;
        myObject.dataPointer = new int(5);
        return std::move(myObject);
    }
    virtual size_t hash() override
    {
        return k;
    }
};

struct MyObjectGroup {
};
struct MyObjectSpecification3 : public LEapsGL::ProxyRequestSpecification<MyObject> {
public:
    // Required::
    // ---------------------------------------------
    using proxy_group = LEapsGL::ProxyEntity<MyObjectGroup>;
    using instance_type = MyObject;
    // ---------------------------------------------
    // Client resource
    int k = 0;

    // spec
    virtual instance_type generateInstance() const {
        instance_type myObject;
        for (int i = 0; i < 5; i++) myObject.data[i] = k + i;
        myObject.dataPointer = new int(5);
        return std::move(myObject);
    }
    virtual size_t hash() override
    {
        return k;
    }
};
struct MyObjectSpecification2 : public LEapsGL::ProxyRequestSpecification<MyObject> {
public:
    // Required::
    // ---------------------------------------------
    using proxy_group = LEapsGL::ProxyEntity<MyObjectGroup>;
    using instance_type = MyObject;

    // ---------------------------------------------
    // Client resource
    int k = 0;

    // spec
    virtual instance_type generateInstance() const {
        instance_type myObject;
        for (int i = 0; i < 5; i++) myObject.data[i] = k + i;
        myObject.dataPointer = new int(5);
        return std::move(myObject);
    }
    virtual size_t hash() override
    {
        return k;
    }
};
TEST(ModelAssimpTest, ProxyTest) {

    {
    MyObjectSpecification spec;
    spec.k = 5;
    auto requestor = LEapsGL::ProxyTraits::Get(spec);
        auto sameSpecification = spec;
        auto requestor_2 = LEapsGL::ProxyTraits::Get(sameSpecification);
        auto sameSpecification2 = LEapsGL::ProxyTraits::Get(spec);

        auto& proxy = LEapsGL::Context::getGlobalContext<LEapsGL::Proxy>();
        MyObject& x = proxy.get(requestor);
        x.print();

        MyObject& y = proxy.get(requestor_2);
        y.print();

        ASSERT_TRUE(&x == &y);

        MyObjectSpecification2 spec2;
        spec2.k = 6;
        MyObjectSpecification3 spec3;
        spec3.k = 7;


        auto requestor2 = LEapsGL::ProxyTraits::Get(spec2);
        auto requestor3 = LEapsGL::ProxyTraits::Get(spec3);


        proxy.get(requestor2);
        proxy.get(requestor3);

        proxy.update(requestor2);
        proxy.remove(requestor2);


        // Prototype test
        auto proto1 = proxy.prototype(requestor3);
        auto proto2 = proxy.prototype(requestor3);
        auto proto3 = proxy.prototype(requestor3);
        auto proto4 = proxy.prototype(proto3);

        proxy.get(proto1);
        proxy.get(proto2);
        proxy.get(proto3);
        proxy.get(proto4);
        proxy.get(proto4);
    }

    EXPECT_TRUE(LEapsGL::ProxyRequestSpecification<MyObject>::Counter.find(5) == LEapsGL::ProxyRequestSpecification<MyObject>::Counter.end());
    EXPECT_TRUE(LEapsGL::ProxyRequestSpecification<MyObject>::Counter.size() == 0);
    
    //for (int i = 0; i < 5; i++) cout << x.data[i] << " ";
}
TEST(ModelAssimpTest, SimpleLoadTest) {

}