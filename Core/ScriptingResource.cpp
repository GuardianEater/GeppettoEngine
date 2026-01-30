/*****************************************************************//**
 * \file   ScriptingResource.cpp
 * \brief  implementation for the ScriptingResource
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "ScriptingResource.hpp"

#include "Script.hpp"

namespace Client
{
    //ScriptingResource::ScriptingResource()
    //{
    //    // if the user has python installed access their packages
    //    py::module sys = py::module::import("sys");
    //    auto path = sys.attr("path").cast<py::list>();
    //    path.append("C:/Users/2018t/AppData/Local/Programs/Python/Python313/Lib/site-packages");

    //    // vec3 class
    //    py::class_<glm::vec3>(mMain, "Vec3")
    //        .def(py::init<float, float, float>(), py::arg("x") = 0, py::arg("y") = 0, py::arg("z") = 0)
    //        .def_readwrite("x", &glm::vec3::x)
    //        .def_readwrite("y", &glm::vec3::y)
    //        .def_readwrite("z", &glm::vec3::z);

    //    // override print so it outputs to the logger
    //    py::module builtins = py::module::import("builtins");
    //    builtins.attr("print") = py::cpp_function([](py::args args, py::kwargs kwargs) 
    //    {
    //        std::string msg;
    //        for (auto& arg : args)
    //            msg += py::str(arg).cast<std::string>() + " ";

    //        Gep::Log::Info(msg);
    //    });
    //}

    //void ScriptingResource::LocateScripts()
    //{
    //    mKnownScripts.clear();

    //    // get all files in the scripts directory

    //    if (std::filesystem::exists("assets\\scripts"))
    //    {
    //        for (const auto& entry : std::filesystem::directory_iterator("assets\\scripts"))
    //        {
    //            mKnownScripts.insert(entry.path());
    //        }
    //    }
    //}

    //void ScriptingResource::ReloadModule(py::module& oldModule)
    //{
    //    py::object importlib = py::module::import("importlib");
    //    oldModule = importlib.attr("reload")(oldModule);
    //}

    //void ScriptingResource::UnloadAllModules()
    //{
    //    mModules.clear();
    //}

    //void ScriptingResource::PyCall(py::function& func) const
    //{
    //    // atempt to call the python function, if the scripts function has an error disable it
    //    try 
    //    {
    //        if (func)
    //            func();
    //    }
    //    catch (const py::error_already_set& e)
    //    {
    //        Gep::Log::Error(e.what());
    //        func = py::function();
    //    }
    //}

    //void ScriptingResource::ReloadAllModules()
    //{
    //    Gep::Log::Important("Reloading all modules...");
    //    for (auto& [name, module] : mModules) 
    //    {
    //        if (!module.is_none()) 
    //        {
    //            Gep::Log::Info("Reloading module, [", name, "]...");
    //            ReloadModule(module);
    //        }
    //    }
    //    Gep::Log::Important("Finished reloading all modules.");
    //}

    //void ScriptingResource::BindScriptToModule(Script& script, py::module module) const
    //{
    //    // binds function if it has one
    //    if (py::hasattr(module, "on_enabled"))
    //        script.on_enabled = module.attr("on_enabled");

    //    if (py::hasattr(module, "on_start"))
    //        script.on_start = module.attr("on_start");

    //    if (py::hasattr(module, "update"))
    //        script.update = module.attr("on_update");

    //    if (py::hasattr(module, "late_update"))
    //        script.late_update = module.attr("on_late_update");

    //    if (py::hasattr(module, "on_disable"))
    //        script.on_disable = module.attr("on_disable");

    //    if (py::hasattr(module, "on_destroy"))
    //        script.on_destroy = module.attr("on_destroy");
    //}

    //py::module ScriptingResource::GetModule(const std::string& name)
    //{
    //    if (!mModules.contains(name))
    //    {
    //        Gep::Log::Error("Attempting to get module: [", name, "] but it doesn't exist");
    //        return py::none();
    //    }

    //    return mModules.at(name);
    //}

    //py::module ScriptingResource::GetOrLoadModule(const std::filesystem::path& path)
    //{
    //    if (mModules.contains(path.string()))
    //        return mModules.at(path.string());

    //    py::module sys = py::module::import("sys");

    //    // update path to the location of the script
    //    auto syspath = sys.attr("path").cast<py::list>();
    //    if (!syspath.contains(path.parent_path().string()))
    //        syspath.append(path.parent_path().string());

    //    // imports the module and keeps track of it

    //    try
    //    {
    //        py::module module = py::module::import(path.stem().string().c_str());
    //        mModules[path.string()] = module;

    //        return module;
    //    }
    //    catch (const py::error_already_set& e)
    //    {
    //        Gep::Log::Error("\n", e.trace());
    //        Gep::Log::Error("\n", e.what());
    //        Gep::Log::Error("Failed to import: [", path.string(), "]");
    //    }

    //    return py::none();
    //}

    //const std::set<std::filesystem::path>& ScriptingResource::GetKnownScripts() const
    //{
    //    return mKnownScripts;
    //}
}
