/*****************************************************************//**
 * \file   ScriptingResource.cpp
 * \brief  implementation for the ScriptingResource
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "ScriptingResource.hpp"

namespace Client
{
    ScriptingResource::ScriptingResource()
    {
        // if the user has python installed access their packages
        py::module sys = py::module::import("sys");
        auto path = sys.attr("path").cast<py::list>();
        path.append("C:/Users/2018t/AppData/Local/Programs/Python/Python313/Lib/site-packages");

        // vec3 class
        py::class_<glm::vec3>(mMain, "Vec3")
            .def(py::init<float, float, float>(), py::arg("x") = 0, py::arg("y") = 0, py::arg("z") = 0)
            .def_readwrite("x", &glm::vec3::x)
            .def_readwrite("y", &glm::vec3::y)
            .def_readwrite("z", &glm::vec3::z);

        // override print so it outputs to the logger
        py::module builtins = py::module::import("builtins");
        builtins.attr("print") = py::cpp_function([](py::args args, py::kwargs kwargs) {
            std::string msg;
            for (auto& arg : args) {
                msg += py::str(arg).cast<std::string>() + " ";
            }

            Gep::Log::Info(msg);
        });
    }

    void ScriptingResource::LocateScripts()
    {
        mKnownScripts.clear();

        // get all files in the scripts directory

        if (std::filesystem::exists("assets\\scripts"))
        {
            for (const auto& entry : std::filesystem::directory_iterator("assets\\scripts"))
            {
                mKnownScripts.insert(entry.path());
            }
        }
    }

    const std::set<std::filesystem::path>& ScriptingResource::GetKnownScripts() const
    {
        return mKnownScripts;
    }
}
