#include "polyglot.h"

#include "begemot.h"
#include "misspell.h"
#include "on_utterance.h"
#include "query_tokens_stats.h"
#include "ut_helper.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/client.pb.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/experiments/flags.h>

#include <alice/library/experiments/experiments.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;
using namespace testing;

Y_UNIT_TEST_SUITE(AppHostMegamindPolyglot) {
    Y_UNIT_TEST_F(SetupSuccess, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();

        TTestComponents<TTestEventComponent, TTestClientComponent> components;
        auto& event = components.Get<TTestEventComponent>();

        TEventComponent::TEventProto proto;
        proto.SetType(EEventType::text_input);
        proto.SetText("hello");
        event.UpdateEvent(std::move(proto));

        TMaybe<TString> utterance = "string that should be discarded";
        UNIT_ASSERT(!AppHostPolyglotSetup(ahCtx, components, utterance, "en-ru").Defined());
        UNIT_ASSERT(!utterance.Defined());

        auto httpRequest =
            ahCtx.TestCtx().GetOnlyProtobufItem<NAppHostHttp::THttpRequest>(AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME, NAppHost::EContextItemSelection::Output);
        UNIT_ASSERT(!httpRequest.GetPath().Empty());
    }
    Y_UNIT_TEST_F(PostSetup, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();
        FakeSkrInit(ahCtx, TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent});

        NAppHostHttp::THttpResponse misspellResponse;
        misspellResponse.SetContent(
            R"({"code":201,"lang":"ru,en","rule":"Misspell","flags":0,"r":10000,"srcText":"пр­(е)­вет","text":"пр­(и)­вет","f":{}})");
        misspellResponse.SetStatusCode(200);
        ahCtx.TestCtx().AddProtobufItem(misspellResponse, AH_ITEM_MISSPELL_HTTP_RESPONSE_NAME,
                                        NAppHost::EContextItemKind::Input);

        NAppHostHttp::THttpResponse polyglotResponse;
        polyglotResponse.SetContent(R"({"code":200,"lang":"en-ru","text":["Здравствуй"]})");
        polyglotResponse.SetStatusCode(200);
        ahCtx.TestCtx().AddProtobufItem(polyglotResponse, AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME,
                                        NAppHost::EContextItemKind::Input);

        NMegamindAppHost::TClientItem clientItem;
        clientItem.MutableClientInfo()->SetAppId("ru.yandex.quasar"); // Needs for query tokens stats

        ahCtx.TestCtx().DeleteItems(AH_ITEM_SKR_CLIENT_INFO, NAppHost::EContextItemSelection::Input);
        ahCtx.TestCtx().AddProtobufItem(clientItem, AH_ITEM_SKR_CLIENT_INFO, NAppHost::EContextItemKind::Input);

        TAppHostUtterancePostSetupNodeHandler postNodeHandler{GlobalCtx, false};
        const auto status = postNodeHandler.Execute(ahCtx);
        UNIT_ASSERT(!status.Defined());
        // Because of on_utterance.cpp (AppHostOnUtteranceReadySetup() -> AppHostQuertTokensStatsSetup()).
        UNIT_ASSERT_VALUES_EQUAL_C(
            ahCtx.TestCtx().GetItemRefs(AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME, NAppHost::EContextItemSelection::Output).size(), 1,
            "Because of on_utterance.cpp (AppHostOnUtteranceReadySetup() -> AppHostBegemotSetup())");

        auto begemotRequest =
            ahCtx.TestCtx().GetOnlyItem(AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME, NAppHost::EContextItemSelection::Output);

        NJson::TJsonValue utterance;
        UNIT_ASSERT(begemotRequest.GetValueByPath("params.text", utterance));
        UNIT_ASSERT_EQUAL_C(utterance.GetType(), NJson::EJsonValueType::JSON_ARRAY,
                            "begemot_request.params.text should be string array");

        UNIT_ASSERT_EQUAL(utterance.GetArraySafe().size(), 1);
        UNIT_ASSERT_VALUES_EQUAL_C(
            utterance.GetArraySafe().at(0), "Здравствуй",
            "Usage of utterance returned by POLYGLOT expected, but another utterance was chosen instead");
    }
}

} // namespace
