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
            mSetComponentMemberReferences.push_back([&](Gep::Entity entity, sol::table& entityTable)
            {
                ComponentType& component = mManager.GetComponent<ComponentType>(entity);
                std::string typeName = Gep::GetTypeInfo<ComponentType>().PrettyName();

                auto view = rfl::to_view(component);

                sol::table componentTable = mLua.create_table();
                view.apply([&](const auto& field)
                {
                    using FieldType = std::remove_reference_t<decltype(*field.value())>;
                    const std::string fieldName(field.name());
                    auto& fieldValue = *field.value();

                    componentTable[fieldName] = field.value();
                });

                entityTable[typeName] = componentTable;
            });
        });
    }
}


