#include <alice/hollywood/library/framework/ut/nlg/register.h>
#include <alice/hollywood/library/framework/ut/nlg/register.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/api/renderer/api.pb.h>

#include <alice/library/json/json.h>

#include <alice/protos/api/renderer/api.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice::NHollywoodFw;

//
// Hwo to add features into response
//
enum class ETestMode {
    None,           // Don't add TRunFeatures
    Dispatch,       // Add to custom irrelevant renderer in Dispatch
    DispatchStd,    // Add to default irrelevant renderer in Dispatch
    SceneRender,    // Add to usual renderer in sceneMain
    SceneRenderStd, // Add to standard renderer in sceneMain
    SceneContinue,  // Add to response with continue arguments
    SceneCommit,    // Add to response with commit arguments
    SceneApply      // Add to response with apply arguments
};

class TTestScene: public TScene<TProtoUtScene1> {
public:
    TTestScene(const TScenario* owner)
        : TScene(owner, "test_scene")
    {
    }
    TRetMain Main(const TProtoUtScene1&, const TRunRequest&, TStorage&, const TSource&) const override;
};

class TTestScenario: public TScenario {
public:
    TTestScenario(ETestMode mode)
        : TScenario("my_test_scenario")
        , Mode_(mode)
    {
        RegisterScene<TTestScene>([this]() {
            RegisterSceneFn(&TTestScene::Main);
        });
        Register(&TTestScenario::Dispatch);
        RegisterRenderer(&TTestScenario::RenderIrrelevant);

        SetNlgRegistration(NAlice::NHollywood::NLibrary::NFramework::NUt::NNlg::RegisterAll);
    }

    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        if (Mode_ == ETestMode::Dispatch) {
            TRunFeatures features;
            features.SetPlayerFeatures(true, 123);
            return TReturnValueRenderIrrelevant(&TTestScenario::RenderIrrelevant, {}, std::move(features));
        } else if (Mode_ == ETestMode::DispatchStd) {
            TRunFeatures features;
            features.SetPlayerFeatures(true, 456);
            return TReturnValueRenderIrrelevant("render_div2_test", "none", std::move(features));
        }
        TProtoUtScene1 scn;
        scn.SetValue((int)Mode_);
        return TReturnValueScene<TTestScene>(scn);
    }
    TRetResponse RenderIrrelevant(const TProtoUtRenderer1&, TRender&) const {
        return TReturnValueSuccess();
    }

private:
    ETestMode Mode_;
};

TRetMain TTestScene::Main(const TProtoUtScene1& sceneArgs, const TRunRequest&, TStorage&, const TSource&) const {
    ETestMode mode = (ETestMode)(sceneArgs.GetValue());

    if (mode == ETestMode::SceneRender) {
        TRunFeatures features;
        features.SetPlayerFeatures(true, 456);
        return TReturnValueRender(&TTestScenario::RenderIrrelevant, {}, std::move(features));
    } else if (mode == ETestMode::SceneRenderStd) {
        TProtoUtRenderer1 renderArgs;
        TRunFeatures features;
        features.SetPlayerFeatures(true, 456);
        return TReturnValueRender("render_div2_test", "none", renderArgs, std::move(features));
    } else if (mode == ETestMode::SceneContinue) {
        TRunFeatures features;
        features.SetPlayerFeatures(true, 456);
        return TReturnValueContinue(sceneArgs, std::move(features));
    } else if (mode == ETestMode::SceneCommit) {
        TRunFeatures features;
        features.SetPlayerFeatures(true, 456);
        return TReturnValueCommit(&TTestScenario::RenderIrrelevant, {}, sceneArgs, std::move(features));
    } else if (mode == ETestMode::SceneApply) {
        TRunFeatures features;
        features.SetPlayerFeatures(true, 456);
        return TReturnValueApply(sceneArgs, std::move(features));
    }
    return TReturnValueRender(&TTestScenario::RenderIrrelevant, {});
}


Y_UNIT_TEST_SUITE(HollywoodFrameworkRunFeatures) {

    Y_UNIT_TEST(RunFeaturesNone) {
        TTestScenario testScenario{ETestMode::None};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);

        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer() == false);
    } // Y_UNIT_TEST(RunFeaturesNone)

    Y_UNIT_TEST(RunFeaturesDispatch) {
        TTestScenario testScenario{ETestMode::Dispatch};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);

        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer());
        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetSecondsSincePause() == 123);
    } // Y_UNIT_TEST(RunFeaturesDispatch)

    Y_UNIT_TEST(RunFeaturesDispatchStd) {
        TTestScenario testScenario{ETestMode::DispatchStd};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);

        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer());
        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetSecondsSincePause() == 456);
    } // Y_UNIT_TEST(RunFeaturesDispatchStd)

    Y_UNIT_TEST(RunFeaturesSceneRender) {
        TTestScenario testScenario{ETestMode::SceneRender};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);
        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer());
    }
    Y_UNIT_TEST(RunFeaturesSceneRenderStd) {
        TTestScenario testScenario{ETestMode::SceneRenderStd};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);
        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer());
    }
    Y_UNIT_TEST(RunFeaturesSceneContinue) {
        TTestScenario testScenario{ETestMode::SceneContinue};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);
        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer());
    }
    Y_UNIT_TEST(RunFeaturesSceneCommit) {
        TTestScenario testScenario{ETestMode::SceneCommit};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);
        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer());
    }
    Y_UNIT_TEST(RunFeaturesSceneApply) {
        TTestScenario testScenario{ETestMode::SceneApply};
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);
        UNIT_ASSERT(testEnv.RunResponseFeartures.GetPlayerFeatures().GetRestorePlayer());
    }

} // Y_UNIT_TEST_SUITE(HollywoodFrameworkRunFeatures)

} // namespace
