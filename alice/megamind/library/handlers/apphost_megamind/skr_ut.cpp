#include "skr.h"

#include "blackbox.h"
#include "misspell.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/components.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <apphost/lib/common/constants.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

namespace {

Y_UNIT_TEST_SUITE(AppHostMegamindNodeSkr) {
    Y_UNIT_TEST_F(FailSmoke, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();

        TAppHostSkrNodeHandler nodeHandler{GlobalCtx, false};
        UNIT_ASSERT_EXCEPTION(nodeHandler.Execute(ahCtx), TRequestCtx::TBadRequestException);
    }

    Y_UNIT_TEST_F(FailWithoutEvent, TAppHostFixture) {
        auto httpRequest = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}.BuildHttpRequestProtoItem();

        auto ahCtx = CreateAppHostContext();
        ahCtx.TestCtx().AddProtobufItem(httpRequest, NAppHost::PROTO_HTTP_REQUEST, NAppHost::EContextItemKind::Input);

        TAppHostSkrNodeHandler nodeHandler{GlobalCtx, false};
        UNIT_ASSERT_EXCEPTION(nodeHandler.Execute(ahCtx), TRequestCtx::TBadRequestException);
    }

    Y_UNIT_TEST_F(SuccessSmokeWithoutOAuth, TAppHostFixture) {
        TSpeechKitApiRequestBuilder skrBuilder;
        skrBuilder.SetTextInput("hello");
        auto httpRequest = TSpeechKitRequestBuilder{skrBuilder.BuildJson()}.BuildHttpRequestProtoItem();

        auto ahCtx = CreateAppHostContext();
        ahCtx.TestCtx().AddProtobufItem(httpRequest, NAppHost::PROTO_HTTP_REQUEST, NAppHost::EContextItemKind::Input);

        TAppHostSkrNodeHandler nodeHandler{GlobalCtx, false};
        auto status = nodeHandler.Execute(ahCtx);
        UNIT_ASSERT(!status.Defined());

        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SKR_EVENT).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SKR_CLIENT_INFO).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SPEECHKIT_REQUEST).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_UNIPROXY_REQUEST).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_FAKE_ITEM).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_MISSPELL_HTTP_REQUEST_NAME).size(), 1);
    }

    Y_UNIT_TEST_F(SuccessSmokeWithOAuth, TAppHostFixture) {
        TSpeechKitApiRequestBuilder skrBuilder;
        skrBuilder.SetTextInput("hello");
        skrBuilder.SetOAuthToken("super-duper-token");
        auto httpRequest = TSpeechKitRequestBuilder{skrBuilder.BuildJson()}.BuildHttpRequestProtoItem();

        auto ahCtx = CreateAppHostContext();
        ahCtx.TestCtx().AddProtobufItem(httpRequest, NAppHost::PROTO_HTTP_REQUEST, NAppHost::EContextItemKind::Input);

        TAppHostSkrNodeHandler nodeHandler{GlobalCtx, false};
        auto status = nodeHandler.Execute(ahCtx);
        UNIT_ASSERT(!status.Defined());

        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SKR_EVENT).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SKR_CLIENT_INFO).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SPEECHKIT_REQUEST).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_UNIPROXY_REQUEST).size(), 1);

        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_BLACKBOX_HTTP_REQUEST_NAME).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_MISSPELL_HTTP_REQUEST_NAME).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_FAKE_ITEM).size(), 1);
    }

    Y_UNIT_TEST_F(CheckAsrHypothesesReranking, TAppHostFixture) {
        for (const auto disableReranking : {false, true}) {
            TSpeechKitApiRequestBuilder skrBuilder;
            TVector<TString> shortHypoWords{"алиса", "включи"}, longHypoWords{"алиса", "включи", "радио", "energy"};
            skrBuilder.SetAsrResult({shortHypoWords, longHypoWords});

            if (disableReranking) {
                skrBuilder.SetValueExpFlag(TString{EXP_DISABLE_ASR_HYPOTHESES_RERANKING}, "1");
            }

            auto httpRequest = TSpeechKitRequestBuilder{skrBuilder.BuildJson()}.BuildHttpRequestProtoItem();

            auto ahCtx = CreateAppHostContext();
            ahCtx.TestCtx().AddProtobufItem(httpRequest, NAppHost::PROTO_HTTP_REQUEST, NAppHost::EContextItemKind::Input);

            TAppHostSkrNodeHandler nodeHandler{GlobalCtx, false};
            auto status = nodeHandler.Execute(ahCtx);
            UNIT_ASSERT(!status.Defined());

            const auto& skrEvents = ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SKR_EVENT);
            UNIT_ASSERT_VALUES_EQUAL(skrEvents.size(), 1);

            NAlice::TEvent event;
            skrEvents.begin()->Fill(&event);
            const auto correctUtterance = JoinStrings(disableReranking ? shortHypoWords : longHypoWords, " ");
            UNIT_ASSERT_VALUES_EQUAL(event.GetAsrResult(0).GetUtterance(), correctUtterance);
            UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_FAKE_ITEM).size(), 1);
        }
    }
}

} // namespace
