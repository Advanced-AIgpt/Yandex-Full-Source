#include "misspell.h"

#include "begemot.h"
#include "on_utterance.h"
#include "query_tokens_stats.h"
#include "ut_helper.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/client.pb.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/experiments/flags.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;
using namespace testing;

Y_UNIT_TEST_SUITE_F(AppHostMegamindMisspell, TAppHostFixture) {
    Y_UNIT_TEST(SetupNotRequested) {
        auto ahCtx = CreateAppHostContext();

        TTestComponents<TTestEventComponent, TTestClientComponent> components;
        auto& event = static_cast<TTestEventComponent&>(components);
        auto& client = static_cast<TTestClientComponent&>(components);
        THashMap<TString, TMaybe<TString>> expFlags;
        EXPECT_CALL(client, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        {
            TEventComponent::TEventProto proto;
            proto.SetType(EEventType::text_input);
            event.UpdateEvent(std::move(proto));
            TMaybe<TString> utterance;
            auto status = AppHostMisspellSetup(ahCtx, components, utterance);
            UNIT_ASSERT_C(status.Defined(), "event with text_input but without text");
            UNIT_ASSERT(!utterance.Defined());
        }

        {
            TEventComponent::TEventProto proto;
            proto.SetType(EEventType::voice_input);
            event.UpdateEvent(std::move(proto));
            TMaybe<TString> utterance;
            auto status = AppHostMisspellSetup(ahCtx, components, utterance);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetResult(), NJson::TJsonValue{NJson::JSON_ARRAY});
            UNIT_ASSERT(!utterance.Defined());
        }
    }

    Y_UNIT_TEST(SetupForNotTextInput) {
        auto ahCtx = CreateAppHostContext();
        TTestComponents<TTestEventComponent, TTestClientComponent> components;
        auto& event = static_cast<TTestEventComponent&>(components);
        auto& client = static_cast<TTestClientComponent&>(components);
        THashMap<TString, TMaybe<TString>> expFlags;
        EXPECT_CALL(client, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        TEventComponent::TEventProto proto;
        proto.SetType(EEventType::voice_input);
        proto.MutableAsrResult()->Add()->AddWords()->SetValue("hello");
        event.UpdateEvent(std::move(proto));
        TMaybe<TString> utterance;
        auto status = AppHostMisspellSetup(ahCtx, components, utterance);
        UNIT_ASSERT(!status.Defined());
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetResult(), NJson::TJsonValue{NJson::JSON_ARRAY});
        UNIT_ASSERT(utterance.Defined());
        UNIT_ASSERT_VALUES_EQUAL(*utterance, "hello");
    }

    Y_UNIT_TEST(SetupForVoiceInputWithFlag) {
        auto ahCtx = CreateAppHostContext();
        TTestComponents<TTestEventComponent, TTestClientComponent> components;
        auto& event = static_cast<TTestEventComponent&>(components);
        auto& client = static_cast<TTestClientComponent&>(components);
        THashMap<TString, TMaybe<TString>> expFlags = {{TString(EXP_ENABLE_VOICE_MISSPELL), "1"}};
        EXPECT_CALL(client, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        TEventComponent::TEventProto proto;
        proto.SetType(EEventType::voice_input);
        proto.MutableAsrResult()->Add()->AddWords()->SetValue("hello");
        event.UpdateEvent(std::move(proto));
        TMaybe<TString> utterance;
        auto status = AppHostMisspellSetup(ahCtx, components, utterance);
        UNIT_ASSERT(!status.Defined());
        UNIT_ASSERT_VALUES_EQUAL(ahCtx.TestCtx().GetResult(), NJson::TJsonValue{NJson::JSON_ARRAY});
        UNIT_ASSERT(!utterance.Defined());
        NAppHostHttp::THttpRequest httpRequest;
        UNIT_ASSERT(ahCtx.TestCtx().GetOnlyProtobufItem(AH_ITEM_MISSPELL_HTTP_REQUEST_NAME).Fill(&httpRequest));
        UNIT_ASSERT(!httpRequest.GetPath().Empty());
    }

    Y_UNIT_TEST_F(SetupSuccess, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();

        TTestComponents<TTestEventComponent, TTestClientComponent> components;
        auto& event = components.Get<TTestEventComponent>();

        TEventComponent::TEventProto proto;
        proto.SetType(EEventType::text_input);
        proto.SetText("hello");
        event.UpdateEvent(std::move(proto));
        TMaybe<TString> utterance;
        auto status = AppHostMisspellSetup(ahCtx, components, utterance);
        UNIT_ASSERT(!status.Defined());
        UNIT_ASSERT(!utterance.Defined());
        NAppHostHttp::THttpRequest httpRequest;
        UNIT_ASSERT(ahCtx.TestCtx().GetOnlyProtobufItem(AH_ITEM_MISSPELL_HTTP_REQUEST_NAME).Fill(&httpRequest));
        UNIT_ASSERT(!httpRequest.GetPath().Empty());
    }

    Y_UNIT_TEST(PostSetup) {
        auto ahCtx = CreateAppHostContext();
        FakeSkrInit(ahCtx, TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent});

        NAppHostHttp::THttpResponse misspellResponse;
        misspellResponse.SetContent(R"({"code":201,"lang":"ru,en","rule":"Misspell","flags":0,"r":10000,"srcText":"пр­(е)­вет","text":"пр­(и)­вет","f":{}})");
        misspellResponse.SetStatusCode(200);
        ahCtx.TestCtx().AddProtobufItem(misspellResponse, AH_ITEM_MISSPELL_HTTP_RESPONSE_NAME, NAppHost::EContextItemKind::Input);

        NMegamindAppHost::TClientItem clientItem;
        clientItem.MutableClientInfo()->SetAppId("ru.yandex.quasar"); // Needs for query tokens stats
        ahCtx.TestCtx().DeleteItems(AH_ITEM_SKR_CLIENT_INFO, NAppHost::EContextItemSelection::Input);
        ahCtx.TestCtx().AddProtobufItem(clientItem, AH_ITEM_SKR_CLIENT_INFO, NAppHost::EContextItemKind::Input);

        TAppHostUtterancePostSetupNodeHandler postNodeHandler{GlobalCtx, false};
        const auto status = postNodeHandler.Execute(ahCtx);
        UNIT_ASSERT(!status.Defined());
        // Because of on_utterance.cpp (AppHostOnUtteranceReadySetup() -> AppHostQuertTokensStatsSetup()).
        UNIT_ASSERT_VALUES_EQUAL_C(ahCtx.TestCtx().GetItemRefs(AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME, NAppHost::EContextItemSelection::Output).size(), 1,
                                   "Because of on_utterance.cpp (AppHostOnUtteranceReadySetup() -> AppHostBegemotSetup())");

        // TODO (petrk) check for text normalization in AppHostOnUtteranceReadySetup().
    }
}

} // namespace
