/*****************************************************************//**
 * \file   ActionHistoryResource.hpp
 * \brief  Resource for managing action history in the editor; supports undo/redo functionality.
 * 
 * \author 2018t
 * \date   December 2025
 *********************************************************************/

#pragma once

#include <vector>
#include <nlohmann/json.hpp>

#include "UUID.hpp"

namespace Gep
{
    // about to change transform:
        // store the current state
        // perform the change
        // 

    //enum class ActionType
    //{
    //    ComponentAdd,
    //    ComponentRemove,
    //    EntityCreate,
    //    EntityDelete
    //};

    //struct Action
    //{
    //    ActionType type;
    //    nlohmann::json data; // serialized data before the action
    //};

    //class ActionHistoryResource
    //{
    //public:
    //    void StartAction();
    //    void Undo();
    //    void Redo();

    //private:
    //    std::vector<Action> mActionStack;
    //    size_t mCurrentActionIndex = 0;

    //    Action mCurrentAction;
    //};
}
