#include "framework.h"

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/core/scenario_factory.h>
#include <alice/hollywood/library/framework/proto/default_render.pb.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

// Stub scenario
class TSelfScenario: public TScenario {
private:
    class TSelfScene: public TScene<TProtoUtScene1> {
    public:
        TSelfScene(const TScenario* owner)
            : TScene(owner, "self") {
        }
        TRetMain Main(const TProtoUtScene1&, const TRunRequest&, TStorage&, const TSource&) const override {
            return TReturnValueRender(&TSelfScenario::Render, TProtoRenderIrrelevantNlg{});
        }
    };
public:
    TSelfScenario()
        : TScenario("self_scenario")
    {
        Register(&TSelfScenario::Dispatch);
        RegisterScene<TSelfScene>([this]() {
            RegisterSceneFn(&TSelfScene::Main);
        });
    }
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        return TReturnValueRenderIrrelevant(&TSelfScenario::Render, TProtoRenderIrrelevantNlg{});
    }
    static TRetResponse Render(const TProtoRenderIrrelevantNlg&, TRender&) {
        return TReturnValueSuccess();
    }
};

} // anonimous namespace

HW_REGISTER(TSelfScenario);

Y_UNIT_TEST_SUITE(RequestSelfTests) {
    Y_UNIT_TEST(RequestSelfTest) {
        TTestEnvironment testEnv("self_scenario", "ru-ru");
        testEnv >> TTestDispatch(&TSelfScenario::Dispatch) >> testEnv;
    }
}

} // namespace NAlice::NHollywoodFw
