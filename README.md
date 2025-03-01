## TLDR
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

  Gep::type_list
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

  Gep::type_list
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

  em.RegisterResource<MyResource>();

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
```cpp
void MySystem::Update(float dt)
{
  // gets all entities that have the component 'MyComponent'
  auto& entities = mManager.GetEntities<MyComponent>();

  for (auto entity : entities)
  {
    MyComponent& mc = mManager.GetComponent(entity);

    mc.myFloat += 2.0f * dt;
  }
}
```
```cpp
void MySystem::Update(float dt)
{
  // gets all entities that have the components 'MyComponent' AND 'Transform'
  auto& transformEntities = mManager.GetEntities<MyComponent, Transform>();

  for (auto entity : transformEntities)
  {
    MyComponent& mc = mManager.GetComponent(entity);
    Transform& transform = mManger.GetComponent(entity);

    transform.position.x += mc.myFloat;
  }
}
```
```cpp
void MySystem::Update(float dt)
{
  // gets all entities that have the components 'MyComponent' AND 'Transform'
  auto& transformEntities = mManager.GetEntities<MyComponent, Transform>();

  for (auto entity : transformEntities)
  {
    MyComponent& mc = mManager.GetComponent(entity);
    Transform& transform = mManger.GetComponent(entity);

    transform.position.x += mc.myFloat;

    // if one of the entities happens to have a 'Rigidbody'
    if (HasComponent<Rigidbody>(entity))
    {
      Rigidbody& rb = mManager.GetComponent(entity);

      rb.velocity.x = 1.0f;
    }
  }
}
```