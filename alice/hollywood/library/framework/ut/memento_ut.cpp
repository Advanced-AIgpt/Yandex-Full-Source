#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>
#include <alice/hollywood/library/framework/ut/nlg/register.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/library/json/json.h>
#include <alice/memento/proto/api.pb.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

const TStringBuf SCENARIO_NAME = "my_test_scenario";

enum class EScenarioMode {
    Node1_FullRw, // Use 1 node graph (Request->Run->Response) and full memento writer
    Node2_PartRw, // Use 2 node graph (Request->Run->Main->Response) and partial functions
    Node3_PartRw  // Use 3 node graph (Request->Run->Main->Render->Response) and partial functions
};

class TTestScene1: public TScene<TProtoUtScene1> {
public:
    TTestScene1(const TScenario* owner)
        : TScene(owner, "test_scene1")
    {
        RegisterRenderer(&TTestScene1::Render);
    }
    TRetMain Main(const TProtoUtScene1& sceneArgs, const TRunRequest&, TStorage& storage, const TSource&) const override {
        TProtoTestPack scenarioData;
        TProtoTestPack surfaceScenarioData;
        UNIT_ASSERT(storage.GetMementoScenarioData(scenarioData));
        UNIT_ASSERT(storage.GetMementoSurfaceScenarioData(surfaceScenarioData));
        UNIT_ASSERT_STRINGS_EQUAL(scenarioData.GetValue(), "ScenarioData");
        UNIT_ASSERT_STRINGS_EQUAL(surfaceScenarioData.GetValue(), "SurfaceScenarioData");

        // Setup new data for memento
        EScenarioMode mode = static_cast<EScenarioMode>(sceneArgs.GetValue());
        switch (mode) {
            case EScenarioMode::Node1_FullRw: {
                ru::yandex::alice::memento::proto::TDeviceConfigsKeyAnyPair cfg;
                cfg.SetKey(ru::yandex::alice::memento::proto::DCK_DUMMY);
                cfg.MutableValue()->PackFrom(sceneArgs);
                ru::yandex::alice::memento::proto::TDeviceConfigs dcfg;
                dcfg.SetDeviceId("DEVICEID");
                *dcfg.AddDeviceConfigs() = cfg;
                NScenarios::TMementoChangeUserObjectsDirective newDirectives;
                *newDirectives.MutableUserObjects()->AddDevicesConfigs() = dcfg;
                storage.SetMementoConfig(std::move(newDirectives));
                break;
            }
            case EScenarioMode::Node2_PartRw: {
                storage.AddMementoUserConfig(ru::yandex::alice::memento::proto::EConfigKey::CK_UNDEFINED, sceneArgs);
                break;
            }
            case EScenarioMode::Node3_PartRw: {
                storage.AddMementoSurfaceScenarioData("KEY", sceneArgs);
                break;
            }
        }

        TProtoUtRenderer1 renderArgs;
        renderArgs.SetValue(sceneArgs.GetValue());
        return TReturnValueRender(&TTestScene1::Render, renderArgs);
    }
    TRetResponse Render(const TProtoUtRenderer1&, TRender&) const {
        return TReturnValueSuccess();
    }
};

class TTestScenario: public TScenario {
public:
    TTestScenario(EScenarioMode mode)
        : TScenario(SCENARIO_NAME)
        , Mode_(mode)
    {
        Register(&TTestScenario::Dispatch);
        RegisterScene<TTestScene1>([this]() {
            RegisterSceneFn(&TTestScene1::Main);
        });
        switch (mode) {
            case EScenarioMode::Node1_FullRw:
                SetApphostGraph(ScenarioRequest() >> NodeRun() >> ScenarioResponse());
                break;
            case EScenarioMode::Node2_PartRw:
                SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> ScenarioResponse());
                break;
            case EScenarioMode::Node3_PartRw:
                SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> NodeRender() >> ScenarioResponse());
                break;
        }
    }

private:
    TRetScene Dispatch(const TRunRequest&, const TStorage& storage, const TSource&) const {
        TProtoTestPack scenarioData;
        TProtoTestPack surfaceScenarioData;
        UNIT_ASSERT(storage.GetMementoScenarioData(scenarioData));
        UNIT_ASSERT(storage.GetMementoSurfaceScenarioData(surfaceScenarioData));
        UNIT_ASSERT_STRINGS_EQUAL(scenarioData.GetValue(), "ScenarioData");
        UNIT_ASSERT_STRINGS_EQUAL(surfaceScenarioData.GetValue(), "SurfaceScenarioData");

        TProtoUtScene1 sceneArgs;
        sceneArgs.SetValue((int)Mode_);
        return TReturnValueScene<TTestScene1>(sceneArgs);
    }

private:
    EScenarioMode Mode_;
};

} // anonimous namespace

Y_UNIT_TEST_SUITE(HollywoodFrameworkMementoTest) {
    Y_UNIT_TEST(MementoTest1) {
        TTestScenario testScenario(EScenarioMode::Node1_FullRw);
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        TProtoTestPack scenarioData;
        TProtoTestPack surfaceScenarioData;

        scenarioData.SetValue("ScenarioData");
        surfaceScenarioData.SetValue("SurfaceScenarioData");
        testEnv.RunRequest.MutableBaseRequest()->MutableMemento()->MutableScenarioData()->PackFrom(scenarioData);
        testEnv.RunRequest.MutableBaseRequest()->MutableMemento()->MutableSurfaceScenarioData()->PackFrom(surfaceScenarioData);
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

        // Validate final server directive
        UNIT_ASSERT(testEnv.ResponseBody.GetServerDirectives().size() == 1);
        UNIT_ASSERT(testEnv.ResponseBody.GetServerDirectives()[0].HasMementoChangeUserObjectsDirective());
        const auto& d = testEnv.ResponseBody.GetServerDirectives()[0].GetMementoChangeUserObjectsDirective();

        UNIT_ASSERT(d.GetUserObjects().GetDevicesConfigs().size() == 1);
        UNIT_ASSERT_STRINGS_EQUAL(d.GetUserObjects().GetDevicesConfigs()[0].GetDeviceId(), "DEVICEID");
    } // Y_UNIT_TEST(MementoTest1)

    Y_UNIT_TEST(MementoTest2) {
        TTestScenario testScenario(EScenarioMode::Node2_PartRw);
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        TProtoTestPack scenarioData;
        TProtoTestPack surfaceScenarioData;

        scenarioData.SetValue("ScenarioData");
        surfaceScenarioData.SetValue("SurfaceScenarioData");
        testEnv.RunRequest.MutableBaseRequest()->MutableMemento()->MutableScenarioData()->PackFrom(scenarioData);
        testEnv.RunRequest.MutableBaseRequest()->MutableMemento()->MutableSurfaceScenarioData()->PackFrom(surfaceScenarioData);
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);

        // Validate final server directive
        UNIT_ASSERT(testEnv.ResponseBody.GetServerDirectives().size() == 1);
        UNIT_ASSERT(testEnv.ResponseBody.GetServerDirectives()[0].HasMementoChangeUserObjectsDirective());
        const auto& d = testEnv.ResponseBody.GetServerDirectives()[0].GetMementoChangeUserObjectsDirective();

        UNIT_ASSERT(d.GetUserObjects().GetUserConfigs().size() == 1);
        UNIT_ASSERT(d.GetUserObjects().GetUserConfigs()[0].GetKey() == ru::yandex::alice::memento::proto::EConfigKey::CK_UNDEFINED);
        TProtoUtScene1 testValue;
        UNIT_ASSERT(d.GetUserObjects().GetUserConfigs()[0].GetValue().UnpackTo(&testValue));
        UNIT_ASSERT(testValue.GetValue() == 1);
    } // Y_UNIT_TEST(MementoTest2)

    Y_UNIT_TEST(MementoTest3) {
        TTestScenario testScenario(EScenarioMode::Node3_PartRw);
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        TProtoTestPack scenarioData;
        TProtoTestPack surfaceScenarioData;

        scenarioData.SetValue("ScenarioData");
        surfaceScenarioData.SetValue("SurfaceScenarioData");
        testEnv.RunRequest.MutableBaseRequest()->MutableMemento()->MutableScenarioData()->PackFrom(scenarioData);
        testEnv.RunRequest.MutableBaseRequest()->MutableMemento()->MutableSurfaceScenarioData()->PackFrom(surfaceScenarioData);
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        UNIT_ASSERT(!testEnv.GetProtoHwScene().HasMementoDirective());
        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);
        UNIT_ASSERT(testEnv.GetProtoHwScene().HasMementoDirective());
        UNIT_ASSERT(testEnv >> TTestApphost("render") >> testEnv);

        // Validate final server directive
        UNIT_ASSERT(testEnv.ResponseBody.GetServerDirectives().size() == 1);
        UNIT_ASSERT(testEnv.ResponseBody.GetServerDirectives()[0].HasMementoChangeUserObjectsDirective());
        const auto& d = testEnv.ResponseBody.GetServerDirectives()[0].GetMementoChangeUserObjectsDirective();

        TProtoUtScene1 testValue;
        UNIT_ASSERT(d.GetUserObjects().GetSurfaceScenarioData().GetScenarioData().at("KEY").UnpackTo(&testValue));
        UNIT_ASSERT(testValue.GetValue() == 2);
    } // Y_UNIT_TEST(MementoTest3)
} // Y_UNIT_TEST_SUITE(HollywoodFrameworkMementoTest)

} // namespace
