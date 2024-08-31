#include <alice/cuttlefish/library/cuttlefish/context_save/client/ut/common.h>

#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_save/client/starter.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAliceProtocol;
using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NContextSaveClient;


NAlice::NSpeechKit::TDirective* MakeSpeechKitDirective(TString name) {
    static NAlice::NSpeechKit::TDirective directive;
    directive.SetName(std::move(name));
    return &directive;
}

NAlice::NSpeechKit::TProtobufUniproxyDirective* MakeProtobufUniproxyDirective(TString directiveId) {
    static NAlice::NSpeechKit::TProtobufUniproxyDirective directive;
    directive.MutableContextSaveDirective()->SetDirectiveId(directiveId);
    return &directive;
}

Y_UNIT_TEST_SUITE(ContextSaveRequestBuilderTest) {

    Y_UNIT_TEST(TestWithFilter) {
        TContextSaveRequestBuilder builder(
            [](const NAlice::NSpeechKit::TDirective& directive) {
                return directive.GetName() == "good";
            },
            [](const NAlice::NSpeechKit::TProtobufUniproxyDirective& directive) {
                return directive.GetContextSaveDirective().GetDirectiveId() == "two";
            }
        );

        UNIT_ASSERT(!builder.TryAddDirective(*MakeSpeechKitDirective("bad")));
        UNIT_ASSERT(builder.TryAddDirective(*MakeSpeechKitDirective("good")));
        UNIT_ASSERT(!builder.TryAddDirective(*MakeSpeechKitDirective("normal")));

        UNIT_ASSERT(!builder.TryAddContextSaveDirective(*MakeProtobufUniproxyDirective("one")));
        UNIT_ASSERT(builder.TryAddContextSaveDirective(*MakeProtobufUniproxyDirective("two")));
        UNIT_ASSERT(!builder.TryAddContextSaveDirective(*MakeProtobufUniproxyDirective("three")));

        const TContextSaveRequest result = std::move(builder).Build();

        UNIT_ASSERT_EQUAL(result.GetDirectives().size(), 1);
        UNIT_ASSERT_EQUAL(result.GetDirectives()[0].GetName(), "good");

        UNIT_ASSERT_EQUAL(result.GetContextSaveDirectives().size(), 1);
        UNIT_ASSERT_EQUAL(result.GetContextSaveDirectives()[0].GetDirectiveId(), "two");
    }

    Y_UNIT_TEST(TestWithoutFilters) {
        TContextSaveRequestBuilder builder;

        UNIT_ASSERT(builder.TryAddDirective(*MakeSpeechKitDirective("bad")));
        UNIT_ASSERT(builder.TryAddDirective(*MakeSpeechKitDirective("good")));

        UNIT_ASSERT(builder.TryAddContextSaveDirective(*MakeProtobufUniproxyDirective("one")));
        UNIT_ASSERT(builder.TryAddContextSaveDirective(*MakeProtobufUniproxyDirective("two")));

        const TContextSaveRequest result = std::move(builder).Build();
        UNIT_ASSERT_EQUAL(result.GetDirectives().size(), 2);
        UNIT_ASSERT_EQUAL(result.GetDirectives()[0].GetName(), "bad");
        UNIT_ASSERT_EQUAL(result.GetDirectives()[1].GetName(), "good");

        UNIT_ASSERT_EQUAL(result.GetContextSaveDirectives().size(), 2);
        UNIT_ASSERT_EQUAL(result.GetContextSaveDirectives()[0].GetDirectiveId(), "one");
        UNIT_ASSERT_EQUAL(result.GetContextSaveDirectives()[1].GetDirectiveId(), "two");
    }

}


Y_UNIT_TEST_SUITE(ContextSaveStarterTest) {

    Y_UNIT_TEST(TestLikeGproxy) {
        TTestFixture fixture;

        TContextSaveStarter starter;
        starter.SetRequestId("123");
        starter.SetAppId("telek");
        starter.SetPuid("paxakor");
        starter.AddClientExperiment("use_memento", "yep");
        starter.AddClientExperiment("use_drugs", "nope");

        NAlice::TSpeechKitResponseProto mmResponse;
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("shla"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("sasha"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("po"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("shosse"));

        const auto filter = [](const NAlice::NSpeechKit::TDirective& directive) {
            return directive.GetName()[0] == 's';
        };

        UNIT_ASSERT_EQUAL(starter.AddDirectives(mmResponse, filter), 3);
        std::move(starter).Finalize(*fixture.AppHostContext, ITEM_TYPE_CONTEXT_SAVE_REQUEST, "");

        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_REQUEST_CONTEXT));
        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_SESSION_CONTEXT));
        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_CONTEXT_SAVE_REQUEST));

        const auto reqCtx = fixture.AppHostContext->GetOnlyProtobufItem<TRequestContext>(
            ITEM_TYPE_REQUEST_CONTEXT
        );
        UNIT_ASSERT_EQUAL(reqCtx.GetHeader().GetReqId(), "123");
        UNIT_ASSERT(NExpFlags::ConductingExperiment(reqCtx, "use_memento"));
        UNIT_ASSERT(NExpFlags::ConductingExperiment(reqCtx, "use_drugs"));

        const auto sessionCtx = fixture.AppHostContext->GetOnlyProtobufItem<TSessionContext>(
            ITEM_TYPE_SESSION_CONTEXT
        );
        UNIT_ASSERT_EQUAL(sessionCtx.GetAppId(), "telek");
        UNIT_ASSERT_EQUAL(sessionCtx.GetUserInfo().GetPuid(), "paxakor");

        const auto csReq = fixture.AppHostContext->GetOnlyProtobufItem<TContextSaveRequest>(
            ITEM_TYPE_CONTEXT_SAVE_REQUEST
        );
        UNIT_ASSERT_EQUAL(csReq.GetDirectives().size(), 3);
    }

    void TestLikeUniproxyImpl(bool withAudio) {
        TTestFixture fixture;

        TContextSaveStarter starter;
        NAlice::TSpeechKitResponseProto mmResponse;
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("shla"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("pasha"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("po"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("shosse"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("save_user_audio"));

        const auto filter = [withAudio](const NAlice::NSpeechKit::TDirective& directive) {
            return (
                (directive.GetName()[0] == 'p') ||
                (withAudio && (directive.GetName() == "save_user_audio"))
            );
        };

        const uint32_t expectedCount = withAudio ? 3 : 2;

        UNIT_ASSERT_EQUAL(starter.AddDirectives(mmResponse, filter), expectedCount);
        std::move(starter).Finalize(*fixture.AppHostContext, ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_REQUEST, "my_mega_flag");

        UNIT_ASSERT(!fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_REQUEST_CONTEXT));
        UNIT_ASSERT(!fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_SESSION_CONTEXT));
        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_REQUEST));
        UNIT_ASSERT_EQUAL(fixture.AppHostContext->GetFlags().contains("my_mega_flag"), withAudio);

        const auto csReq = fixture.AppHostContext->GetOnlyProtobufItem<TContextSaveRequest>(
            ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_REQUEST
        );
        UNIT_ASSERT_EQUAL(csReq.GetDirectives().size(), static_cast<int>(expectedCount));
    }

    Y_UNIT_TEST(TestLikeUniproxyWithAudio) {
        TestLikeUniproxyImpl(true);
    }

    Y_UNIT_TEST(TestLikeUniproxyWithoutAudio) {
        TestLikeUniproxyImpl(false);
    }

    Y_UNIT_TEST(TestNoRequest) {
        TContextSaveStarter starter;
        NAlice::TSpeechKitResponseProto mmResponse;
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("ololo"));

        const auto filter = [](const NAlice::NSpeechKit::TDirective&) {
            return false;
        };

        UNIT_ASSERT_EQUAL(starter.AddDirectives(mmResponse, filter), 0);
    }

    Y_UNIT_TEST(TestNoFilter) {
        TContextSaveStarter starter;
        NAlice::TSpeechKitResponseProto mmResponse;
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("ololo"));
        mmResponse.MutableVoiceResponse()->AddDirectives()->Swap(MakeSpeechKitDirective("azaza"));

        UNIT_ASSERT_EQUAL(starter.AddDirectives(mmResponse), 2);
    }

}
