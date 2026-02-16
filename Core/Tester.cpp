/*****************************************************************//**
 * \file   Tester.cpp
 * \brief  Object used for testing various features of the engine.
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#include "pch.hpp"

#include "EngineManager.hpp"

#include "Tester.hpp"

namespace Client
{
    // useful for testing creates an entity and automatically destroys it when it goes out of scope
    class ScopedEntity
    {
    public:
        ScopedEntity(Gep::EngineManager& manager, const std::string& name = "", const gtl::uuid& = gtl::uuid{})
            : mManager(manager)
            , mEntity(manager.CreateEntity(name))
        {
        }
        ~ScopedEntity()
        {
            mManager.DestroyEntity(mEntity);
        }
        Gep::Entity Get() const { return mEntity; }

    private:
        Gep::EngineManager& mManager;
        Gep::Entity mEntity;
    };

    bool EngineTester::TestEntity_Basic()
    {
        const std::string testUUIDStr = "TTTTTTTT-TTTTTTTT-TTTTTTTT";
        const gtl::uuid testUUID = gtl::to_uuid(testUUIDStr);
        const std::string testName = "TestEntity:" + testUUIDStr;
        ScopedEntity e(mEngineManager, testName, testUUID);

        // check if the entity was created successfully
        if (!mEngineManager.EntityExists(e.Get()))
        {
            return false;
        }

        // check if the name was set correctly
        if (mEngineManager.GetName(e.Get()) != testName)
        {
            return false;
        }

        // check if uuid was set correctly
        if (mEngineManager.GetUUID(e.Get()) != testUUID)
        {
            return false;
        }

        // the found entity should be the same as the created entity
        if (mEngineManager.FindEntity(testUUID) != e.Get())
        {
            return false;
        }

        // entity should have no components
        if (mEngineManager.GetSignature(e.Get()) != 0)
        {
            return false;
        }

        // entity should have no children
        if (mEngineManager.GetChildCount(e.Get()) != 0)
        {
            return false;
        }

        // entity should have no parent
        if (mEngineManager.HasParent(e.Get()))
        {
            return false;
        }

        // parent should have an invalid entity
        if (mEngineManager.GetParent(e.Get()) != Gep::INVALID_ENTITY)
        {
            return false;
        }

        // entity should have no ancestors
        if (mEngineManager.GetAncestors(e.Get()).size() != 0)
        {
            return false;
        }

        // entity is root
        if (mEngineManager.GetRoot(e.Get()) == e.Get())
        {
            return false;
        }
    }

    bool EngineTester::TestComponents_AddRemove()
    {
        ScopedEntity e(mEngineManager, "Test Entity");

        const auto& componentDatas = mEngineManager.GetComponentDatas();

        // add all components
        for (size_t i = 0; i < 1000; ++i)
        {
            for (const auto [i, cd] : componentDatas)
            {
                cd.add(e.Get());
            }

            // remove all components
            for (const auto [i, cd] : componentDatas)
            {
                cd.remove(e.Get());
            }
        }

        return componentDatas[0].count == 0;
    }

    EngineTester::EngineTester(Gep::EngineManager& em)
        : mEngineManager(em)
    {
    }
}
