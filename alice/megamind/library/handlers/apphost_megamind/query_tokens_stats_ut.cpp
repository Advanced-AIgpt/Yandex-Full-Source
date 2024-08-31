#include "query_tokens_stats.h"

#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/components.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>
#include <alice/megamind/library/experiments/flags.h>

#include <alice/library/proto/proto.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

#include <search/session/compression/report.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

Y_UNIT_TEST_SUITE_F(AppHostMegamindQueryTokensStats, TAppHostFixture) {
    Y_UNIT_TEST(SetupSmoke) {
        auto ahCtx = CreateAppHostContext();

        TSpeechKitApiRequestBuilder builder;
        auto initCtx = TSpeechKitRequestBuilder{
            builder.SetPredefinedClient(TSpeechKitApiRequestBuilder::EClient::Quasar)
                   .EnableExpFlag(TString{EXP_ENABLE_QUERY_TOKEN_STATS})
                   .BuildJson()
        }.BuildInitContext();

        TTestComponents<TTestClientComponent> components{initCtx};

        {
            auto status = AppHostQueryTokensStatsSetup(ahCtx, "", components);
            UNIT_ASSERT(!status.Defined());

            UNIT_ASSERT(!ahCtx.TestCtx().HasItem(AH_ITEM_QUERY_TOKENS_STATS_HTTP_REQUEST_NAME, NAppHost::EContextItemSelection::Output));
        }

        {
            auto status = AppHostQueryTokensStatsSetup(ahCtx, "hello", components);
            UNIT_ASSERT(!status.Defined());

            NAppHostHttp::THttpRequest httpRequest;

            UNIT_ASSERT(ahCtx.TestCtx().GetOnlyProtobufItem(AH_ITEM_QUERY_TOKENS_STATS_HTTP_REQUEST_NAME).Fill(&httpRequest));
            UNIT_ASSERT(!httpRequest.GetPath().Empty());
        }
    }

    Y_UNIT_TEST(PostSetupSmoke) {
        auto ahCtx = CreateAppHostContext();

        {
            NAppHostHttp::THttpResponse httpResponse;
            httpResponse.SetContent(NResource::Find("/query_stats_response"));
            httpResponse.SetStatusCode(200);
            ahCtx.TestCtx().AddProtobufItem(httpResponse, AH_ITEM_QUERY_TOKENS_STATS_HTTP_RESPONSE_NAME, NAppHost::EContextItemKind::Input);
        }

        {
            NKvSaaS::TTokensStatsResponse response;
            auto error = AppHostQueryTokensStatsPostSetup(ahCtx, response);
            UNIT_ASSERT(!error.Defined());
            UNIT_ASSERT(!response.GetTokensStatsByClients().empty());
        }
    }
}

} // namespace
