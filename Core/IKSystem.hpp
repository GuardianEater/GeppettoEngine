// inverse kinematics system
// uses the ik component

#pragma once

#include "ISystem.hpp"

namespace Gep::Event
{
    template<typename T> struct ComponentEditorRender;
}

namespace Client
{
    struct IKTarget;
    struct EditorResource;
}

namespace Gep
{
    class OpenGLRenderer;
}

namespace Client
{
    class IKSystem : public Gep::ISystem
    {
    public:
        IKSystem(Gep::EngineManager& em);

        void Initialize() override;
        void Update(float dt) override;

    private:

        void OnIKTargetEditorRender(const Gep::Event::ComponentEditorRender<Client::IKTarget>& event);

        EditorResource& mEditor;
        Gep::OpenGLRenderer& mRenderer;
    };
}