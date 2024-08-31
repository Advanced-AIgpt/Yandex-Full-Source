#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service_apphosted.h>
#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <apphost/lib/service_testing/service_testing.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/gtest/gtest.h>
#include <util/generic/strbuf.h>

namespace {

    using namespace NAlice::NCuttlefish;
    using namespace NAlice::NCuttlefish::NAppHostServices;

    using ::testing::_;

    struct TSpeakerServiceFixture : public ISpeakerDataService {
        MOCK_METHOD(void, OnMatch, (NAliceProtocol::TMatchVoiceprintResult matchResult, TTicket ticket), (override));
        MOCK_METHOD(void, OnNoMatch, (), (override));
        MOCK_METHOD(void, OnDatasyncResponse, (NAppHostHttp::THttpResponse response, TTicket ticket), (override));
        MOCK_METHOD(void, OnBlackboxResponse, (NAppHostHttp::THttpResponse response, TTicket ticket), (override));
    };

    class TApphostedSpeakerServiceTests : public ::testing::Test {
    public:
        TApphostedSpeakerServiceTests()
            : Metrics("test_metrics")
            , Sut(Metrics, TLogContext(new TSelfFlushLogFrame(nullptr), nullptr))
        {
        }

        void AddMatchVoiceprintResult(int iteration) {
            NAliceProtocol::TMatchVoiceprintResult matchVoiceprintResult;
            matchVoiceprintResult.MutableGuestOptions();

            AddProtobufItem(matchVoiceprintResult, ITEM_TYPE_VOICEPRINT_MATCH_RESULT, iteration);
        }

        void AddNoMatchVoiceprintResult() {
            NAliceProtocol::TMatchVoiceprintResult matchVoiceprintResult;
            matchVoiceprintResult.MutableGuestOptions();

            AddProtobufItem(matchVoiceprintResult, ITEM_TYPE_VOICEPRINT_NO_MATCH_RESULT);
        }

        void AddDatasyncResponse(int iteration) {
            NAppHostHttp::THttpResponse response;
            AddProtobufItem(std::move(response), ITEM_TYPE_GUEST_DATASYNC_HTTP_RESPONSE, iteration);
        }

        void AddBlackboxResponse(int iteration) {
            NAppHostHttp::THttpResponse response;
            AddProtobufItem(std::move(response), ITEM_TYPE_GUEST_BLACKBOX_HTTP_RESPONSE, iteration);
        }

    private:
        template <typename T>
        void AddProtobufItem(T&& item, TStringBuf itemType, int iteration) {
            AddProtobufItem(std::forward<T>(item), TStringBuilder() << itemType << '_' << iteration);
        };

        template <typename T>
        void AddProtobufItem(T&& item, TStringBuf itemType) {
            AhContext.AddProtobufItem(std::forward<T>(item), itemType, NAppHost::EContextItemKind::Input);
        };

    public:
        TSourceMetrics Metrics;
        NAppHost::NService::TTestContext AhContext;
        TApphostedSpeakerService<::testing::StrictMock<TSpeakerServiceFixture>> Sut;
    };

    TEST_F(TApphostedSpeakerServiceTests, WhenNoItemsReceived_ShouldPassNothingAndGracefullyFinish) {
        EXPECT_CALL(Sut, OnMatch).Times(0);
        EXPECT_CALL(Sut, OnNoMatch).Times(0);
        EXPECT_CALL(Sut, OnDatasyncResponse).Times(0);
        EXPECT_CALL(Sut, OnBlackboxResponse).Times(0);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSingleMatchVoiceprintResult_ShouldParseAndCallOnMatchOnce) {
        AddMatchVoiceprintResult(1);
        EXPECT_CALL(Sut, OnMatch(_, 1)).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSingleNoMatchVoiceprintResult_ShouldCallOnNoMatchOnce) {
        AddNoMatchVoiceprintResult();
        EXPECT_CALL(Sut, OnNoMatch).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSingleDatasyncResponse_ShouldParseAndCallOnDatasyncResponseOnce) {
        AddDatasyncResponse(1);
        EXPECT_CALL(Sut, OnDatasyncResponse(_, 1)).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSingleBlackboxResponse_ShouldParseAndCallBlackobxResponseOnce) {
        AddBlackboxResponse(1);
        EXPECT_CALL(Sut, OnBlackboxResponse(_, 1)).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSeveralMatchVoiceprintResults_ShouldParseAndCallOnMatchOncePerItem) {
        AddMatchVoiceprintResult(1);
        AddMatchVoiceprintResult(2);
        AddMatchVoiceprintResult(3);

        EXPECT_CALL(Sut, OnMatch(_, 1)).Times(1);
        EXPECT_CALL(Sut, OnMatch(_, 2)).Times(1);
        EXPECT_CALL(Sut, OnMatch(_, 3)).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSeveralNoMatchVoiceprintResults_ShouldCallOnNoMatchOncePerItem) {
        AddNoMatchVoiceprintResult();
        AddNoMatchVoiceprintResult();
        AddNoMatchVoiceprintResult();

        EXPECT_CALL(Sut, OnNoMatch).Times(3);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSeveralDatasyncResponses_ShouldParseAndCallOnDatasyncResponseOncePerItem) {
        AddDatasyncResponse(1);
        AddDatasyncResponse(2);
        AddDatasyncResponse(3);

        EXPECT_CALL(Sut, OnDatasyncResponse(_, 1)).Times(1);
        EXPECT_CALL(Sut, OnDatasyncResponse(_, 2)).Times(1);
        EXPECT_CALL(Sut, OnDatasyncResponse(_, 3)).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedSeveralBlackboxResponses_ShouldParseAndCallBlackobxResponseOncePerItem) {
        AddBlackboxResponse(1);
        AddBlackboxResponse(2);
        AddBlackboxResponse(3);

        EXPECT_CALL(Sut, OnBlackboxResponse(_, 1)).Times(1);
        EXPECT_CALL(Sut, OnBlackboxResponse(_, 2)).Times(1);
        EXPECT_CALL(Sut, OnBlackboxResponse(_, 3)).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedDifferentItemsForSameSpeaker_ShouldParseAndCallExpectedHandlersOncePerItem) {
        AddMatchVoiceprintResult(1);
        AddNoMatchVoiceprintResult();
        AddDatasyncResponse(1);
        AddBlackboxResponse(1);

        EXPECT_CALL(Sut, OnMatch(_, 1)).Times(1);
        EXPECT_CALL(Sut, OnNoMatch).Times(1);
        EXPECT_CALL(Sut, OnDatasyncResponse(_, 1)).Times(1);
        EXPECT_CALL(Sut, OnBlackboxResponse(_, 1)).Times(1);

        Sut.OnNextInput(AhContext);
    }

    TEST_F(TApphostedSpeakerServiceTests, WhenReceivedDifferentItemsForDifferentSpeakers_ShouldParseAndCallExpectedHandlersOncePerItem) {
        AddMatchVoiceprintResult(3);
        AddNoMatchVoiceprintResult();
        AddDatasyncResponse(1);
        AddBlackboxResponse(2);

        EXPECT_CALL(Sut, OnMatch(_, 3)).Times(1);
        EXPECT_CALL(Sut, OnNoMatch).Times(1);
        EXPECT_CALL(Sut, OnDatasyncResponse(_, 1)).Times(1);
        EXPECT_CALL(Sut, OnBlackboxResponse(_, 2)).Times(1);

        Sut.OnNextInput(AhContext);
    }
}