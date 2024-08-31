#include "personal_intents.h"

#include <alice/library/proto/proto.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/components.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

#include <search/session/compression/report.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

Y_UNIT_TEST_SUITE_F(AppHostMegamindPersonalIntents, TAppHostFixture) {
    Y_UNIT_TEST(SetupSmoke) {
        auto ahCtx = CreateAppHostContext();

        TSpeechKitApiRequestBuilder builder;
        auto initCtx = TSpeechKitRequestBuilder{builder.SetPredefinedClient(TSpeechKitApiRequestBuilder::EClient::Quasar).BuildJson()}
                                               .BuildInitContext();

        TTestComponents<TTestClientComponent> components{initCtx};

        auto status = AppHostPersonalIntentsSetup(ahCtx, components);
        UNIT_ASSERT(!status.Defined());

        NAppHostHttp::THttpRequest httpRequest;

        UNIT_ASSERT(ahCtx.TestCtx().GetOnlyProtobufItem(AH_ITEM_PERS_INTENT_HTTP_REQUEST_NAME, NAppHost::EContextItemSelection::Output).Fill(&httpRequest));
        UNIT_ASSERT(!httpRequest.GetPath().Empty());
    }

    Y_UNIT_TEST(PostSetup) {
        auto ahCtx = CreateAppHostContext();

        {
            NAppHostHttp::THttpResponse httpResponse;
            httpResponse.SetContent(NResource::Find("/personal_intents_response"));
            httpResponse.SetStatusCode(200);
            ahCtx.TestCtx().AddProtobufItem(httpResponse, AH_ITEM_PERS_INTENT_HTTP_RESPONSE_NAME, NAppHost::EContextItemKind::Input);
        }

        {
            const auto canonJs = NJson::ReadJsonFastTree(NResource::Find("/personal_intents_response_json"));
            NKvSaaS::TPersonalIntentsResponse response;
            auto error = AppHostPersonalIntentsPostSetup(ahCtx, response);
            UNIT_ASSERT(!error.Defined());
            UNIT_ASSERT(response.ProtoResponse().Defined());
            UNIT_ASSERT_VALUES_EQUAL(NJson::ReadJsonFastTree(NProtobufJson::Proto2Json(*response.ProtoResponse())), canonJs);
        }
    }
}

} // namespace
