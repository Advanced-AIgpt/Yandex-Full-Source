#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>
#include <alice/hollywood/library/framework/ut/nlg/register.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

class TMyTestFastData : public IFastData {
public:
    TMyTestFastData(int value)
        : Value_(value)
    {
    }
    inline int GetValue() const {
        return Value_;
    }
private:
    int Value_;
};

class TTestScene: public TScene<TProtoUtScene2> {
public:
    TTestScene(const TScenario* owner)
        : TScene(owner, "test_scene")
    {
    }
    TRetMain Main(const TProtoUtScene2&, const TRunRequest&, TStorage&, const TSource&) const override {
        HW_ERROR("Impossible");
    }
};

class TTestScenario: public TScenario {
public:
    TTestScenario()
        : TScenario{"my_test_scenario"}
    {
        Register(&TTestScenario::Dispatch);
        RegisterScene<TTestScene>([this]() {
            RegisterSceneFn(&TTestScene::Main);
        });
    }

private:
    TRetScene Dispatch(const TRunRequest& runRequest, const TStorage&, const TSource&) const {
        const auto fd = runRequest.System().GetFastData().GetFastData<TMyTestFastData>();
        Y_ENSURE(fd);
        Y_ENSURE(fd->GetValue() == 123);
        return TError(TError::EErrorDefinition::Unknown);
    }
};

} // anonimous namespace

Y_UNIT_TEST_SUITE(HollywoodFrameworkFastDataTest) {
    Y_UNIT_TEST(FastDataTest) {
        TTestScenario testScenario;
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        testEnv.AttachFastdata(std::shared_ptr<IFastData>(new TMyTestFastData(123)));
        testEnv.DisableErrorReporting();

        UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testEnv));
    } // Y_UNIT_TEST(NormalRequest)
} // Y_UNIT_TEST_SUITE(HollywoodFrameworkSetupSource)

} // namespace
