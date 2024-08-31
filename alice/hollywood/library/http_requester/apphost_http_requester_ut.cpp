#include "apphost_http_requester.h"

#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <apphost/lib/service_testing/service_testing.h>
#include <library/cpp/json/json_value.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {
namespace NHollywood {

namespace {

Y_UNIT_TEST_SUITE(ApphostHttpRequester) {
    Y_UNIT_TEST(DefaultPort) {
        NScenarios::TRequestMeta requestMeta;
        NJson::TJsonValue apphostParams;
        const TString emptyBody;
        const THashMap<TString, TString> emptyHeaders;
        {
            NAppHost::NService::TTestContext ctx;
            THolder<IHttpRequester> requester
                = MakeApphostHttpRequester(ctx, requestMeta, apphostParams, TRTLogger::NullLogger(), "goodwin_", "GOODWIN_");
            requester->Add("req", IHttpRequester::EMethod::Get, "http://yandex.ru/", emptyBody, emptyHeaders);
            UNIT_ASSERT_EXCEPTION(requester->Start(), TAppHostNodeExecutionBreakException);
            const NJson::TJsonValue got = ctx.GetOnlyItem("goodwin_app_host_params_req");
            UNIT_ASSERT_VALUES_EQUAL("http://yandex.ru:80", got["srcrwr"]["GOODWIN_HTTP_PROXY"].GetString());
        }

        {
            NAppHost::NService::TTestContext ctx;
            THolder<IHttpRequester> requester
                = MakeApphostHttpRequester(ctx, requestMeta, apphostParams, TRTLogger::NullLogger(), "goodwin_", "GOODWIN_");
            requester->Add("req", IHttpRequester::EMethod::Get, "https://yandex.ru/", emptyBody, emptyHeaders);
            UNIT_ASSERT_EXCEPTION(requester->Start(), TAppHostNodeExecutionBreakException);
            const NJson::TJsonValue got = ctx.GetOnlyItem("goodwin_app_host_params_req");
            UNIT_ASSERT_VALUES_EQUAL("https://yandex.ru:443", got["srcrwr"]["GOODWIN_HTTP_PROXY"].GetString());
        }
    }
}

} // namespace

} // namespace NHollywood
} // namespace NAlice
