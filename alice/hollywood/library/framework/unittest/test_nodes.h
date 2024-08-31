#pragma once

//
//
// HOLLYWOOD FRAMEWORK ENTRY POINT (Unit tests)
//
//

#include "test_environment.h"

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw {

namespace NPrivate {

class TTestBase {
public:
    virtual ~TTestBase() = default;
    bool operator >> (TTestEnvironment& rhs) const;

public:
    mutable const TTestEnvironment* TestEnvironment;

protected:
    TTestBase()
        : TestEnvironment(nullptr)
    {
    }

private:
    virtual bool CallTest(const TScenario& sc, TTestEnvironment& te) const = 0;
};

} // namespace NPrivate

//
// TTestDispatch - wrapper class for TReturnValue (dispatcher functions)
//
class TTestDispatch final: public NPrivate::TTestBase {
public:
    // Default ctor to store user-defined protobuf (call Scene function with setup)
    template <class Object>
    TTestDispatch(TRetSetup (Object::*)(const TRunRequest&, const TStorage&) const)
        : InitialStage_(EStageDeclaration::DispatchSetup)
    {
    }
    // Default ctor to store user-defined protobuf (call Scene function without setup)
    template <class Object>
    TTestDispatch(TRetScene (Object::*)(const TRunRequest&, const TStorage&, const TSource&) const)
        : InitialStage_(EStageDeclaration::Dispatch)
    {
    }

private:
    bool CallTest(const TScenario& sc, TTestEnvironment& te) const override;
private:
    EStageDeclaration InitialStage_;
};

//
// TTestApphost - wrapper class for TReturnValue (dispatcher functions with all local graph)
//
class TTestApphost final: public NPrivate::TTestBase {
public:
    // Default ctor to store user-defined protobuf (call Scene function with setup)
    TTestApphost(TStringBuf nodeName)
        : NodeName_(nodeName)
    {
    }

private:
    bool CallTest(const TScenario& sc, TTestEnvironment& te) const override;
private:
    TString NodeName_;
};

//
// TTestScene - wrapper class for NPrivate::TRetSceneSelector (Renderer functions)
//
class TTestScene final: public NPrivate::TTestBase {
public:
    // Default ctor to store user-defined protobuf (call Scene function)
    template <class Object, class T, typename Ret>
    TTestScene(Ret (Object::*fn)(const T&, const TRunRequest&, const TStorage&) const, const T& proto)
        : SceneArgs_(fn, proto)
    {
    }
    template <class Object, class T, typename Ret>
    TTestScene(Ret (Object::*fn)(const T&, const TRunRequest&, TStorage&, const TSource&) const, const T& proto)
        : SceneArgs_(fn, proto)
    {
    }
private:
    bool CallTest(const TScenario& sc, TTestEnvironment& te) const override;
private:
    NPrivate::TRetSceneSelector SceneArgs_;
};

//
// TTestRender - wrapper class for TReturnValue (Renderer functions)
//
class TTestRender final: public NPrivate::TTestBase {
public:
    // Default ctor to store user-defined protobuf (call Render function (as a member of object))
    template <class Object, class TRenderArgsProto>
    TTestRender(TRetResponse (Object::*)(const TRenderArgsProto&, TRender& render) const, const TRenderArgsProto& proto)
        : RenderPath_(std::is_base_of<TScenario, Object>::value ? ERenderPath::Scenario : ERenderPath::Scene)
    {
        RenderProto_.PackFrom(proto);
    }
    // Default ctor to store user-defined protobuf (call Render function (as a free function))
    template <class TRenderArgsProto>
    TTestRender(TRetResponse (*)(const TRenderArgsProto&, TRender& render), const TRenderArgsProto& proto)
        : RenderPath_(ERenderPath::Standalone)
    {
        RenderProto_.PackFrom(proto);
    }

    // Setup text/voice answer directly from C++ code
    static void SetTextAnswer(TRender& render, const TString& answer);
    static void SetVoiceAnswer(TRender& render, const TString& answer);

private:
    bool CallTest(const TScenario& sc, TTestEnvironment& te) const override;
private:
    enum class ERenderPath {
        Standalone,
        Scenario,
        Scene
    };
    ERenderPath RenderPath_;
    google::protobuf::Any RenderProto_;
};

} // namespace NAlice::NHollywoodFw
