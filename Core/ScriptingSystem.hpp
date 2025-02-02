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

#include <sol/sol.hpp>

namespace Client
{
    class ScriptingSystem : public Gep::ISystem
    {
    public:
        ScriptingSystem(Gep::EngineManager& em);

        template <typename... ComponentTypes>
        void OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes);

        void Initialize();
        void Update(float dt);

    private:
        // given a componentIndex, sets up the lua fields for that compoennt
        std::vector<std::function<void(Gep::Entity, sol::table&)>> mSetComponentMemberReferences;
        sol::state mLua;
        sol::environment mEnvironment;
    };

    template<typename ...ComponentTypes>
    inline void ScriptingSystem::OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes)
    {
        componentTypes.for_each([&]<typename ComponentType>()
        {
            std::string typeName = Gep::GetTypeInfo<ComponentType>().PrettyName();
            ComponentType component{};
            ComponentType* componentPtr = &component;
            auto tempView = rfl::to_view(component);

            auto luaComponentType = mLua.new_usertype<ComponentType>(typeName.c_str(), sol::no_constructor);

            tempView.apply([&](const auto& field)
            {
                using FieldType = std::remove_reference_t<decltype(*field.value())>;

                static const size_t pointerDiff = reinterpret_cast<size_t>(field.value()) - reinterpret_cast<size_t>(componentPtr);
                const std::string_view fieldName = field.name();

                luaComponentType[fieldName] = sol::property(
                [&](const ComponentType& c) -> FieldType
                {
                    FieldType* fieldPtr = reinterpret_cast<FieldType*>(reinterpret_cast<size_t>(&c) + pointerDiff);

                    return *fieldPtr;
                },
                [&](ComponentType& c, FieldType value)
                { 
                    FieldType* fieldPtr = reinterpret_cast<FieldType*>(reinterpret_cast<size_t>(&c) + pointerDiff);

                    *fieldPtr = value; 
                });
            });

            mSetComponentMemberReferences.push_back([&](Gep::Entity entity, sol::table& entityTable)
            {
                ComponentType& component = mManager.GetComponent<ComponentType>(entity);
                std::string typeName = Gep::GetTypeInfo<ComponentType>().PrettyName();

                entityTable[typeName] = &component;
            });
        });
    }
}


