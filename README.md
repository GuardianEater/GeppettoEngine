# Geppetto Engine
# Overview

This is a personal project of mine that follow an Entity-Component-System (ECS) architecture that makes writing high performance code easy. Its 'one line' modular, so a new system or new component can be registered and start running with a single line of code. Many features get generated through reflection, such as UI and serialization.

---
![Lighting](https://guardianeater.github.io/travis-c-gronvold/resources/lighting.pn)
# API
---
## Getting Started
- Systems are where executable code occurs.
- Components are where per entity data is stored.
- Resources are where shared data is stored.
### Adding a System
- Per system memory is discouraged, use a resource instead.
- The system must be default constructable.

`MySystem.hpp`
```cpp
class MySystem : public Gep::ISystem
{
  void Initialize() override;
  void Update(float dt) override;
};
```
`main.cpp`
```cpp
int main()
{
  // ...

  gtl::type_list
  <
    // add all systems here
    MySystem,

    // ...
  > 
  systemTypes;

  // ...
}
```
### Adding a Component
- Components should be relatively *simple structs*.
- No custom constructors/destructors.
- No private data.

`MyComponent.hpp`
```cpp
struct MyComponent
{
  float myFloat = 0.0f;
  int myInt = 1;
  std::string myString = "HelloWorld"
};
```
`main.cpp`
```cpp
int main()
{
  // ...

  gtl::type_list
  <
    // add all components here
    MyComponent,

    // ...
  > 
  componentTypes;

  // ...
}
```
### Adding a Resource
- Resources are an *"anything goes"* structure, however running code is discouraged.

`MyComponent.hpp`
```cpp
class MyResource
{
  std::unordered_map<std::string, std::vector<char>> mLoadedData;
};
```
`main.cpp`
```cpp
int main()
{
  // ...
  gtl::type_list
  <
    // add all resources here
    MyResource,

    // ...
  > 
  resourceTypes;
  // ...
}
```
---
## Basic workflow
#### Getting a Resource:
```cpp
void MySystem::Initialize()
{
  MyResource& mr = mManager.GetResource<MyResource>();

  auto& data = mr.mLoadedData;

  // load all of the data into the resource
}
```
#### Getting Components:
The lamda passed to ForEachArchetype automatically queries for requested components.
```cpp
void MySystem::Update(float dt)
{
  mManager.ForEachArchetype([&](Gep::Entity e, MyComponent& mc)
  {
    mc.myFloat += 2.0f * dt;
  });
}
```
```cpp
void MySystem::Update(float dt)
{
  mManager.ForEachArchetype([&](Gep::Entity e, const MyComponent& mc, Transform& t)
  {
    t.position.x += mc.myFloat;
  });
}
```
```cpp
void MySystem::Update(float dt)
{
  mManager.ForEachArchetype([&](Gep::Entity e, MyComponent& mc, Transform& t)
  {
    t.position.x += mc.myFloat;
    mc.myFloat += 2.0f * dt;

    if (mManager.HasComponent<RigidBody>(e))
    {
      Rigidbody& rb = mManager.GetComponent<RigidBody>(e);
      rb.velocity.x = 1.0f;
    }
  });
}
```
