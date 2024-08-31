#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>
#include <alice/hollywood/library/framework/ut/nlg/register.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

class TTestSceneWithSetup: public TScene<TProtoUtScene2> {
public:
    TTestSceneWithSetup(const TScenario* owner)
        : TScene(owner, "test_scene")
    {
    }
    TRetSetup MainSetup(const TProtoUtScene2&, const TRunRequest&, const TStorage&) const override;
    TRetMain Main(const TProtoUtScene2&, const TRunRequest&, TStorage&, const TSource&) const override;
};

class TTestScenario: public TScenario {
public:
    TTestScenario()
        : TScenario{"my_test_scenario"}
    {
        Register(&TTestScenario::Dispatch);
        RegisterScene<TTestSceneWithSetup>([this]() {
            RegisterSceneFn(&TTestSceneWithSetup::MainSetup);
            RegisterSceneFn(&TTestSceneWithSetup::Main);
        });
        SetNlgRegistration(NAlice::NHollywood::NLibrary::NFramework::NUt::NNlg::RegisterAll);
    }

private:
    TRetScene Dispatch(const TRunRequest& , const TStorage&, const TSource&) const {
        return TReturnValueScene<TTestSceneWithSetup>(TProtoUtScene2{});
    }
};

/*
    Scene setup function
    Simulate network request
*/
TRetSetup TTestSceneWithSetup::MainSetup(const TProtoUtScene2&, const TRunRequest& request, const TStorage&) const {
    TSetup setup(request);
    NHollywood::THttpProxyRequestBuilder builder("/sample", request.GetRequestMeta(), request.Debug().Logger());
    THttpProxyRequest httpRequest = builder.Build();
    setup.Attach(httpRequest);
    return setup;
}
TRetMain TTestSceneWithSetup::Main(const TProtoUtScene2& renderArgs, const TRunRequest&, TStorage&, const TSource& source) const {
    const auto json = source.GetHttpResponseJson("", false);
    UNIT_ASSERT_STRINGS_EQUAL(json.GetString(), "ABC");
    return TReturnValueRender("render_div2_test", "none", renderArgs);
}

Y_UNIT_TEST_SUITE(HollywoodFrameworkSetupSource) {
    Y_UNIT_TEST(NormalRequest) {
        TTestScenario testScenario;
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

        // Validate data inside testEnv
        const auto* req = testEnv.FindSetupRequest(NHollywood::PROXY_REQUEST_KEY_DEFAULT);
        UNIT_ASSERT(req);
        const NAppHostHttp::THttpRequest* httpreq = dynamic_cast<const NAppHostHttp::THttpRequest*>(req);
        UNIT_ASSERT(httpreq);
        UNIT_ASSERT_STRINGS_EQUAL(httpreq->GetPath(), "/sample");
        UNIT_ASSERT(httpreq->GetScheme() == NAppHostHttp::THttpRequest_EScheme_Http);

        // Mock answer to next node
        NAppHostHttp::THttpResponse httpResponse;
        httpResponse.SetStatusCode(200);
        httpResponse.SetContent("ABC");
        testEnv.AddHttpAnswer(NHollywood::PROXY_RESPONSE_KEY_DEFAULT, httpResponse);

        UNIT_ASSERT(testEnv >> TTestApphost("main") >> testEnv);
    } // Y_UNIT_TEST(NormalRequest)
} // Y_UNIT_TEST_SUITE(HollywoodFrameworkSetupSource)

} // namespace
