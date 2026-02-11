/*****************************************************************//**
 * \file   ScriptingSystem.hpp
 * \brief  allows adding scripts to entities
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <ISystem.hpp>
#include <Script.hpp>
#include "EngineManager.hpp"
#include "ScriptingResource.hpp"

namespace Client
{
    //class ScriptingSystem : public Gep::ISystem
    //{
    //public:
    //    ScriptingSystem(Gep::EngineManager& em);
    //    ~ScriptingSystem() {};

    //    template <typename... ComponentTypes>
    //    void OnComponentsRegistered(gtl::type_list<ComponentTypes...> componentTypes);

    //    void Initialize();
    //    void Update(float dt);

    //private:
    //    void OnScriptAdded(const Gep::Event::ComponentAdded<Script>& event);
    //    void OnScriptRemoved(const Gep::Event::ComponentRemoved<Script>& event);

    //    void OnScriptEditorRender(const Gep::Event::ComponentEditorRender<Script>& event);
    //    void OnEntityCreated(const Gep::Event::EntityCreated& event);
    //    void OnEngineStateChanged(const Gep::Event::EngineStateChanged& event);

    //    ScriptingResource& mScriptingResource;
    //};

    //template<typename ...ComponentTypes>
    //inline void ScriptingSystem::OnComponentsRegistered(gtl::type_list<ComponentTypes...> componentTypes)
    //{
    //    ScriptingResource& sr = mManager.GetResource<ScriptingResource>();

    //    componentTypes.for_each([&]<typename ComponentType>()
    //    {
    //        std::string typeName = Gep::GetTypeInfo<ComponentType>().PrettyName();
    //        ComponentType component{};

    //        //sol::usertype<ComponentType> luaComponentType = lua.new_usertype<ComponentType>(typeName);

    //        //if constexpr (HasOnScriptAccess<ComponentType>)
    //        //    component.OnScriptAccess(luaComponentType);

    //        //tempView.apply([&](const auto& field)
    //        //{
    //        //    using FieldType = std::remove_reference_t<decltype(*field.value())>;

    //        //    static const size_t pointerDiff = reinterpret_cast<size_t>(field.value()) - reinterpret_cast<size_t>(componentPtr);
    //        //    const std::string_view fieldName = field.name();

    //        //    luaComponentType[fieldName] = sol::property(
    //        //    [&](const ComponentType& c) -> FieldType
    //        //    {
    //        //        FieldType* fieldPtr = reinterpret_cast<FieldType*>(reinterpret_cast<size_t>(&c) + pointerDiff);

    //        //        return *fieldPtr;
    //        //    },
    //        //    [&](ComponentType& c, FieldType value)
    //        //    { 
    //        //        FieldType* fieldPtr = reinterpret_cast<FieldType*>(reinterpret_cast<size_t>(&c) + pointerDiff);

    //        //        *fieldPtr = value; 
    //        //    });
    //        //});

    //        //mSetComponentMemberReferences.push_back([&](Gep::Entity entity, sol::table& self)
    //        //{
    //        //    if constexpr (HasOnScriptAccess<ComponentType>)
    //        //    {
    //        //        ComponentType& component = mManager.GetComponent<ComponentType>(entity);
    //        //        std::string typeName = Gep::GetTypeInfo<ComponentType>().PrettyName();

    //        //        self[typeName] = &component;
    //        //    }
    //        //});
    //    });
    //}
}


