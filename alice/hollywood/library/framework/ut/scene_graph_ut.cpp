#include "framework.h"

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/proto/default_render.pb.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

constexpr TStringBuf SCENE_NAME1 = "scene1";
constexpr TStringBuf SCENE_NAME2 = "scene2";
constexpr TStringBuf SCENE_NAME3 = "scene1"; // Note SCENE_NAME3 is the same as Scene1 (used for error testing)

} // anonimous namespace

//
// Simple scenario with different versions of exception
//
class TSimplestScenario: public TScenario {
public:
    enum class Behavior {
        NoScenarioFn,
        DispatchSetupOnly,
        DispatchSetupNoNode,
        DispatchTwoEqScenes1Node,
        DispatchTwoEqScenes2Node,
        DispatchWithoutScene,
        DispatchSceneSetupWithoutMain,
        DispatchScenesSetup1Nodes,
        DispatchSceneRender1Node,
        DispatchSceneRender2Node,
        ApphostNameUppercase,
        ApphostMainWithoutRun,
        ApphostTwoClearRun,
        ApphostTwoRunWithExp,
        ApphostRunWithExpWithoutClear,
        ApphostNoFunctions,
        ApphostRunNormal,
        DispatchTwice,
        RenderSameProtoInMembers,
        RenderSameProtoInFreeFn,
        RenderSameProtoDiffTypesScenario,
        RenderSameProtoStaticScenarioScene,
        RenderSameProtoInScenarioScene,
    };

    class TScene1: public TScene<TProtoUtScene1> {
    public:
        TScene1(const TScenario* owner)
            : TScene(owner, SCENE_NAME1)
        {
        }
        TRetSetup MainSetup(const TProtoUtScene1&, const TRunRequest&, const TStorage&) const override {
            return TError(TError::EErrorDefinition::Unknown);
        }
        TRetMain Main(const TProtoUtScene1&, const TRunRequest&, TStorage&, const TSource&) const override {
            return TError(TError::EErrorDefinition::Unknown);
        }
    };
    class TScene2: public TScene<TProtoUtScene2> {
    public:
        TScene2(const TScenario* owner)
            : TScene(owner, SCENE_NAME2)
        {
            RegisterRenderer(&TScene2::Render1a);
            RegisterRenderer(&TScene2::Render2a);
        }
        TRetMain Main(const TProtoUtScene2&, const TRunRequest&, TStorage&, const TSource&) const override {
            return TError(TError::EErrorDefinition::Unknown);
        }
        TRetResponse Render1a(const TProtoUtRenderer1&, TRender&) const {
            return TReturnValueSuccess();
        }
        static TRetResponse Render2a(const TProtoUtRenderer1&, TRender&) {
            return TReturnValueSuccess();
        }
    };
    class TScene3: public TScene<TProtoUtScene2> {
    public:
        TScene3(const TScenario* owner)
            : TScene(owner, SCENE_NAME3)
        {
        }
        TRetMain Main(const TProtoUtScene2&, const TRunRequest&, TStorage&, const TSource&) const override {
            return TError(TError::EErrorDefinition::Unknown);
        }
    };
    TSimplestScenario(Behavior mode)
        : TScenario("simplest_scenario")
    {
        EnableDebugGraph();
        switch (mode) {
            case Behavior::NoScenarioFn:
                break;
            case Behavior::DispatchSetupOnly:
                Register(&TSimplestScenario::DispatchSetup);
                break;
            case Behavior::DispatchSetupNoNode:
                Register(&TSimplestScenario::DispatchSetup);
                break;
            case Behavior::DispatchTwoEqScenes1Node:
                Register(&TSimplestScenario::Dispatch);
                RegisterScene<TSimplestScenario::TScene1>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene1::Main);
                });
                RegisterScene<TSimplestScenario::TScene3>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene3::Main);
                });
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                ScenarioResponse());
                break;
            case Behavior::DispatchTwoEqScenes2Node:
                Register(&TSimplestScenario::Dispatch);
                RegisterScene<TSimplestScenario::TScene1>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene1::Main);
                });
                RegisterScene<TSimplestScenario::TScene3>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene3::Main);
                });
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                NodeMain() >>
                                ScenarioResponse());
                break;
            case Behavior::DispatchWithoutScene:
                Register(&TSimplestScenario::Dispatch);
                break;
            case Behavior::DispatchSceneSetupWithoutMain:
                Register(&TSimplestScenario::Dispatch);
                RegisterScene<TSimplestScenario::TScene1>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene1::MainSetup);
                });
                break;
            case Behavior::DispatchScenesSetup1Nodes:
                Register(&TSimplestScenario::Dispatch);
                RegisterScene<TSimplestScenario::TScene1>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene1::MainSetup);
                    RegisterSceneFn(&TSimplestScenario::TScene1::Main);
                });
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                ScenarioResponse());
                break;
            case Behavior::DispatchSceneRender1Node:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                ScenarioResponse());
                break;
            case Behavior::DispatchSceneRender2Node:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                NodeMain() >>
                                ScenarioResponse());
                break;
            case Behavior::ApphostNameUppercase:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("Run") >>
                                ScenarioResponse());
                break;
            case Behavior::ApphostMainWithoutRun:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeMain() >>
                                ScenarioResponse());
                break;
            case Behavior::ApphostTwoClearRun:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run") >>
                                ScenarioResponse());
                break;
            case Behavior::ApphostTwoRunWithExp:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "EXP1") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "EXP1") >>
                                ScenarioResponse());
                break;
            case Behavior::ApphostRunWithExpWithoutClear:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "EXP1") >>
                                ScenarioResponse());
                break;
            case Behavior::ApphostNoFunctions:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioCommit() >>
                                NodeCommitSetup() >>
                                NodeCommit() >>
                                ScenarioResponse());
                break;
            case Behavior::ApphostRunNormal:
                RegisterStandardScenes();
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "EXP1") >>
                                NodeMain() >>
                                ScenarioResponse());
                break;
            case Behavior::DispatchTwice:
                RegisterStandardScenes();
                Register(&TSimplestScenario::Dispatch); // 2nd Dispatch
                break;
            case Behavior::RenderSameProtoInMembers:
                RegisterStandardScenes();
                RegisterRenderer(&TSimplestScenario::Render1a);
                RegisterRenderer(&TSimplestScenario::Render1b);
                break;
            case Behavior::RenderSameProtoInFreeFn:
                RegisterStandardScenes();
                RegisterRenderer(&TSimplestScenario::Render2a);
                RegisterRenderer(&TSimplestScenario::Render2b);
                break;
            case Behavior::RenderSameProtoDiffTypesScenario:
                RegisterStandardScenes();
                RegisterRenderer(&TSimplestScenario::Render1a);
                RegisterRenderer(&TSimplestScenario::Render2a);
                break;
            case Behavior::RenderSameProtoStaticScenarioScene:
                RegisterStandardScenes();
                RegisterScene<TSimplestScenario::TScene2>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene2::Main);
                });
                RegisterRenderer(&TSimplestScenario::Render2a); // The same static FN
                break;
            case Behavior::RenderSameProtoInScenarioScene:
                RegisterStandardScenes();
                RegisterScene<TSimplestScenario::TScene2>([this]() {
                    RegisterSceneFn(&TSimplestScenario::TScene2::Main);
                });
                RegisterRenderer(&TSimplestScenario::Render1a); // Same proto but different objects
                break;
        }
    }
private:
    TRetSetup DispatchSetup(const TRunRequest&, const TStorage&) const {
        return TError(TError::EErrorDefinition::Unknown);
    }
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        return TError(TError::EErrorDefinition::Unknown);
    }
    TRetResponse Render1a(const TProtoUtRenderer1&, TRender&) const {
        return TReturnValueSuccess();
    }
    TRetResponse Render1b(const TProtoUtRenderer1&, TRender&) const {
        return TReturnValueSuccess();
    }
    static TRetResponse Render2a(const TProtoUtRenderer1&, TRender&) {
        return TReturnValueSuccess();
    }
    static TRetResponse Render2b(const TProtoUtRenderer1&, TRender&) {
        return TReturnValueSuccess();
    }
    void RegisterStandardScenes() {
        Register(&TSimplestScenario::Dispatch);
        RegisterScene<TSimplestScenario::TScene1>([this]() {
            RegisterSceneFn(&TSimplestScenario::TScene1::Main);
        });
    }
};

//
// This suite checks different normal and abnormal situations during scenario initialization, i.e.
//    - wrong or incomplete functions declarations
//    - apphost node name misprints
//    - 1-node and 2-node apphoat graphs
//
Y_UNIT_TEST_SUITE(SimplestScenarioTest) {
    Y_UNIT_TEST(SimplestScenario1) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::NoScenarioFn);
        // Expecting: Y_ENSURE(At least one dispatch function must be inited inside scenario simplest_scenario)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario2) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchSetupOnly);
        // Expecting: Y_ENSURE(At least one scene must be inited inside scenario simplest_scenario)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario3) {
        // Expecting: Y_ENSURE(Apphost node name must be set for DispatchSetup() function. Scenario simplest_scenario)
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchSetupNoNode);
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario4) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchTwoEqScenes1Node);
        // Expected: Y_ENSURE(Two scenes has the same name: 'scene1'. Scenario simplest_scenario)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario5) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchTwoEqScenes2Node);
        // Expected: Y_ENSURE(Two scenes has the same name: 'scene1'. Scenario simplest_scenario)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario6) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchWithoutScene);
        // Expected: Y_ENSURE(At least one scene must be inited inside scenario simplest_scenario)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario7) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchSceneSetupWithoutMain);
        // Expected: Y_ENSURE(MainScene function must exist. Scenario: 'simplest_scenario', Scene: scene1)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario8) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchScenesSetup1Nodes);
        // Expected: Y_ENSURE(The current scenario graph is not compatible with declared functions. Missing function: SceneMainSetup)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }
    Y_UNIT_TEST(SimplestScenario9) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchSceneRender1Node);
        InitializeLocalScenario(simplestScenario);
    }
    Y_UNIT_TEST(SimplestScenario10) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchSceneRender2Node);
        InitializeLocalScenario(simplestScenario);
    }
    Y_UNIT_TEST(SimplestScenario11) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::ApphostNameUppercase);
        // Expected: Y_ENSURE(Apphost nodename must be lower_case: 'Run')
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario12) {
        auto fn = []() {
            TSimplestScenario simplestScenario(TSimplestScenario::Behavior::ApphostMainWithoutRun);
            InitializeLocalScenario(simplestScenario);
        };
        // Expected: Y_ENSURE(Invalid or unsupported stage for local graph)
        UNIT_CHECK_GENERATED_EXCEPTION(
            fn(), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario13) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::ApphostTwoClearRun);
        // Expected: Y_ENSURE(Two apphost nodes 'name' have the same experiments definition: '')
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario14) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::ApphostTwoRunWithExp);
        // Expected: Y_ENSURE(Two apphost nodes 'name' have the same experiments definition: 'EXP1')
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario15) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::ApphostRunWithExpWithoutClear);
        // Expected: Y_ENSURE(Apphost node "run" has experiment but default path doesn't exist)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario16) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::ApphostNoFunctions);
        // Expected: Y_ENSURE(Apphost graph node "commit" requires missing function ''. Scenario simplest_scenario)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario17) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::ApphostRunNormal);
        // Normal initialization
        InitializeLocalScenario(simplestScenario);
    }

    Y_UNIT_TEST(SimplestScenario18) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::DispatchTwice);
        // Expected: Y_ENSURE(Dispatch function was registered twice)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario19) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::RenderSameProtoInMembers);
        // Expected: Y_ENSURE(Two render functions has the same protobuf)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario20) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::RenderSameProtoInFreeFn);
        // Expected: Y_ENSURE(Two render functions has the same protobuf)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario21) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::RenderSameProtoDiffTypesScenario);
        // Normal initialization
        InitializeLocalScenario(simplestScenario);
    }

    Y_UNIT_TEST(SimplestScenario22) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::RenderSameProtoStaticScenarioScene);
        // Expected: Y_ENSURE(Two render functions has the same protobuf)
        UNIT_CHECK_GENERATED_EXCEPTION(
            InitializeLocalScenario(simplestScenario), yexception
        );
    }

    Y_UNIT_TEST(SimplestScenario23) {
        TSimplestScenario simplestScenario(TSimplestScenario::Behavior::RenderSameProtoInScenarioScene);
        // Normal initialization
        InitializeLocalScenario(simplestScenario);
    }

}

} // namespace NAlice::NHollywoodFw
