#include "framework.h"

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/proto/default_render.pb.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

// Stub scenario
class TStubScenario: public TScenario {
private:
    class TStubScene: public TScene<TProtoUtScene1> {
    public:
        TStubScene(const TScenario* owner)
            : TScene(owner, "stub") {
        }
        TRetMain Main(const TProtoUtScene1&, const TRunRequest&, TStorage&, const TSource&) const override {
            return TReturnValueRender(&TStubScenario::RenderIrrelevant, TProtoRenderIrrelevantNlg{});
        }
    };
public:
    TStubScenario(std::function<void(const TRequest::TFlags&)> fn)
        : TScenario("stub_scenario")
        , Fn_(fn)
    {
        Register(&TStubScenario::Dispatch);
        RegisterScene<TStubScene>([this]() {
            RegisterSceneFn(&TStubScene::Main);
        });
    }
    TRetScene Dispatch(const TRunRequest& runRequest, const TStorage&, const TSource&) const {
        Fn_(runRequest.Flags());
        return TReturnValueRenderIrrelevant(&TStubScenario::RenderIrrelevant, TProtoRenderIrrelevantNlg{});
    }
    static TRetResponse RenderIrrelevant(const TProtoRenderIrrelevantNlg&, TRender&) {
        return TReturnValueSuccess();
    }
private:
    std::function<void(const TRequest::TFlags&)> Fn_;
};

} // anonimous namespace

Y_UNIT_TEST_SUITE(RequestFlagsTest) {
    Y_UNIT_TEST(RequestFlagsTest) {
        TStubScenario sc([](const TRequest::TFlags& flags) {
            UNIT_ASSERT(flags.IsExperimentEnabled("experiment1"));
            UNIT_ASSERT(flags.GetValue<TString>("experiment1").Defined());
            UNIT_ASSERT_EQUAL(*flags.GetValue<TString>("experiment1"), "value1");

            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment2"));
            UNIT_ASSERT(!flags.GetValue<TString>("experiment2").Defined());
            UNIT_ASSERT(!flags.GetValue<ui32>("experiment2").Defined());

            UNIT_ASSERT(flags.IsExperimentEnabled("experiment3"));
            UNIT_ASSERT(flags.GetValue<i32>("experiment3").Defined());
            UNIT_ASSERT_EQUAL(*flags.GetValue<i32>("experiment3"), 123);
            UNIT_ASSERT_EQUAL(flags.GetValue<i32>("experiment3", 1), 123);

            UNIT_ASSERT(flags.IsExperimentEnabled("experiment4"));
            UNIT_ASSERT(!flags.GetValue<i32>("experiment4").Defined());
            UNIT_ASSERT_EQUAL(flags.GetValue<float>("experiment4", 0.f), 1.25f);

            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment5"));
            UNIT_ASSERT_EQUAL(flags.GetValue<ui64>("experiment5", 1122334455LU), 1122334455LU);

            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment6"));
            UNIT_ASSERT_EQUAL(flags.GetValue<ui64>("experiment6", 1122334455LU), 0);
            return;
        });
        InitializeLocalScenario(sc);

        TTestEnvironment testEnv("stub_scenario", "ru-ru");
        testEnv.AddExp("experiment1", "value1");
        testEnv.AddExp("experiment3", "123");
        testEnv.AddExp("experiment4", "1.25");
        testEnv.AddExp("experiment5");
        testEnv.AddExp("experiment6", "0");
        testEnv >> TTestDispatch(&TStubScenario::Dispatch) >> testEnv;
    }
    Y_UNIT_TEST(RequestFlagsSubTest) {
        TStubScenario sc([](const TRequest::TFlags& flags) {
            UNIT_ASSERT(flags.IsExperimentEnabled("experiment1"));

            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment2"));
            UNIT_ASSERT(flags.IsExperimentEnabled("experiment2=xxx"));
            UNIT_ASSERT(flags.GetSubValue<TString>("experiment2").Defined());
            UNIT_ASSERT(!flags.GetSubValue<i32>("experiment2").Defined());
            UNIT_ASSERT_EQUAL(*flags.GetSubValue<TString>("experiment2"), "xxx");
            UNIT_ASSERT_EQUAL(flags.GetSubValue<i64>("experiment2", -512365L), -512365L);

            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment3"));
            UNIT_ASSERT(flags.IsExperimentEnabled("experiment3=871"));
            UNIT_ASSERT(flags.GetSubValue<i32>("experiment3").Defined());
            UNIT_ASSERT_EQUAL(flags.GetSubValue("experiment3", 0), 871);

            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment4"));
            UNIT_ASSERT(flags.IsExperimentEnabled("experiment4=12.25"));
            UNIT_ASSERT(flags.GetSubValue<float>("experiment4").Defined());
            UNIT_ASSERT_EQUAL(*flags.GetSubValue<float>("experiment4"), 12.25);

            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment5"));
            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment5=0"));
            UNIT_ASSERT(!flags.IsExperimentEnabled("experiment5=1"));
            UNIT_ASSERT(!flags.GetSubValue<int>("experiment5").Defined());
        });
        InitializeLocalScenario(sc);

        TTestEnvironment testEnv("stub_scenario", "ru-ru");
        testEnv.AddExp("experiment1", "");
        testEnv.AddExp("experiment2=xxx", "1");
        testEnv.AddExp("experiment3=871", "1");
        testEnv.AddExp("experiment4=12.25", "yyy"); // Note this string generates warning during execution
        testEnv.AddExp("experiment5=0", "0");
        testEnv.AddExp("experiment5=1", "0");
        testEnv >> TTestDispatch(&TStubScenario::Dispatch) >> testEnv;
    }

    Y_UNIT_TEST(RequestFlagsEnum) {
        TStubScenario sc([](const TRequest::TFlags& expFlags) {
            int enumCount = 0;
            expFlags.ForEach([&enumCount](const TString& str, const TMaybe<TString>& value) -> bool {
                enumCount++;
                if (str == "experiment1") {
                    UNIT_ASSERT_STRINGS_EQUAL(*value, "");
                } else if (str == "experiment2") {
                    UNIT_ASSERT_STRINGS_EQUAL(*value, "value");
                } else if (str == "experiment3=xxx") {
                    UNIT_ASSERT(!value.Defined());
                } else if (str == "experiment4=871") {
                    UNIT_ASSERT(value.Defined());
                    UNIT_ASSERT_STRINGS_EQUAL(*value, "0");
                } else if (str == "experiment5=12.25") {
                    UNIT_ASSERT(value.Defined());
                    UNIT_ASSERT_STRINGS_EQUAL(*value, "1");
                } else {
                    UNIT_ASSERT(false); // Undefined name
                }
                return true;
            });
            UNIT_ASSERT_EQUAL(enumCount, 5);
            enumCount = 0;
            expFlags.ForEachSubval([&enumCount](const TString& str, const TString& subval, const TMaybe<TString>& value) -> bool {
                enumCount++;
                if (str == "experiment3") {
                    UNIT_ASSERT_STRINGS_EQUAL(subval, "xxx");
                    UNIT_ASSERT(!value.Defined());
                } else if (str == "experiment4") {
                    UNIT_ASSERT_STRINGS_EQUAL(subval, "871");
                    UNIT_ASSERT(value.Defined());
                    UNIT_ASSERT_STRINGS_EQUAL(*value, "0");
                } else if (str == "experiment5") {
                    UNIT_ASSERT_STRINGS_EQUAL(subval, "12.25");
                    UNIT_ASSERT(value.Defined());
                    UNIT_ASSERT_STRINGS_EQUAL(*value, "1");
                } else {
                    UNIT_ASSERT(false); // Undefined name
                }
                return true;
            });
            UNIT_ASSERT_EQUAL(enumCount, 3);
        });
        InitializeLocalScenario(sc);

        TTestEnvironment testEnv("stub_scenario", "ru-ru");
        testEnv.AddExp("experiment1", "");
        testEnv.AddExp("experiment2", "value");
        testEnv.AddExp("experiment3=xxx");
        testEnv.AddExp("experiment4=871", "0");
        testEnv.AddExp("experiment5=12.25", "1");
        testEnv >> TTestDispatch(&TStubScenario::Dispatch) >> testEnv;
    }
}

} // namespace NAlice::NHollywoodFw
