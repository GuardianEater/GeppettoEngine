/*****************************************************************//**
 * \file   Tester.hpp
 * \brief  object used for testing various features of the engine.
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#pragma once

namespace Gep
{
    class EngineManager;
}

namespace Client
{
    class EngineTester
    {
    public:
        EngineTester(Gep::EngineManager& em);

        // tests a handful of basic properties of an entity such as its name, uuid, parent child relationships, and signature. Also tests that destroying an entity works correctly
        bool TestEntity_Basic();

        bool TestComponents_AddRemove();
    private:
        std::vector<std::function<void()>> mTests;
        Gep::EngineManager& mEngineManager;

        std::unordered_map<std::string, std::pair<bool, std::string>> mTestResults; // test name -> (whether the test passed, message about the test)
    };
}
