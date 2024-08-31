#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service_apphosted.h>
#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/context.h>
#include <library/cpp/testing/gtest_protobuf/matcher.h>
#include <library/cpp/testing/gtest/gtest.h>
#include <variant>

namespace {

    using namespace NAlice::NCuttlefish::NAppHostServices;

    struct TFakeMatchItem : public NAliceProtocol::TMatchVoiceprintResult {};
    struct TFakeNoMatchItem {};
    struct TFakeDatasyncItem : public NAppHostHttp::THttpResponse {};
    struct TFakeBlackboxItem : public NAppHostHttp::THttpResponse {};

    using TFakeItem = std::variant<
        TFakeMatchItem,
        TFakeNoMatchItem,
        TFakeDatasyncItem,
        TFakeBlackboxItem
    >;

    using TSpeakerContextPtr = TAtomicSharedPtr<TSpeakerContext>;

    const TString PERS_ID_0 = "TEST_PERS_ID_0";
    const TString PERS_ID_1 = "TEST_PERS_ID_1";

    void AssertSame(const TSpeakerContext& actual, const TSpeakerContext& expected) {
        EXPECT_THAT(actual.GuestUserData, NGTest::EqualsProto(expected.GuestUserData));
        EXPECT_THAT(actual.GuestUserOptions, NGTest::EqualsProto(expected.GuestUserOptions));
    }

    TFakeMatchItem MakeMatchVoiceprintResult(TString persId) {
        TFakeMatchItem matchVoiceprintResult;
        matchVoiceprintResult.MutableGuestOptions()->SetPersId(persId);

        return matchVoiceprintResult;
    }

    TFakeDatasyncItem MakeDatasyncResponse(TString persId) {
        TFakeDatasyncItem response;
        response.SetContent(TStringBuilder() << "datasync" << '_' << persId);

        return response;
    }

    TFakeBlackboxItem MakeBlackboxResponse(TString persId) {
        TFakeBlackboxItem response;
        response.SetContent(TStringBuilder() << R"(
        {
           "dbfields" : {
              "userinfo.lastname.uid" : "lastname",
              "userinfo.firstname.uid" : "firstname"
           },
           "uid" : {
              "value" : ")" << persId << R"("
           },
           "user_ticket" : "mock_user_ticket",
           "phones" : [
              {
                 "id" : "111",
                 "attributes" : {
                    "108" : "1",
                    "102" : "main_phone",
                    "107" : "default_phone"
                 }
              }
           ],
           "attributes" : {
              "1015" : "1"
           },
           "address-list" : [
              {
                 "address" : "root@yandex.ru"
              }
           ],
           "aliases" : {
              "13" : "root"
           },
           "billing_features" : {
              "basic-kinopoisk" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-plus" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-music" : {
                  "in_trial": true,
                  "region_id": 225
              }
           }
        })");

        return response;
    }

    void EnrichSpeakerContext(TSpeakerContext& speakerContext, const TFakeItem& item) {
        if (const auto* matchItem = std::get_if<TFakeMatchItem>(&item)) {
            speakerContext.Update(*matchItem, TSpeakerContext::MatchResultTag);
        } else if (const auto* datasyncResponse = std::get_if<TFakeDatasyncItem>(&item)) {
            speakerContext.Update(*datasyncResponse, TSpeakerContext::DatasyncResponseTag);
        } else if (const auto* blackboxResponse = std::get_if<TFakeBlackboxItem>(&item)) {
            speakerContext.Update(*blackboxResponse, TSpeakerContext::BlackboxResponseTag);
        }
    }

    TSpeakerContextPtr MakeSpeakerContext(const TFakeItem& item) {
        TSpeakerContextPtr speakerContext = MakeAtomicShared<TSpeakerContext>();
        EnrichSpeakerContext(*speakerContext, item);

        return speakerContext;
    }

    class SpeakerServiceTest : public ::testing::Test {
    public:
        TSpeakerService Sut;
    };

    namespace WhenHaveNoActiveSpeaker {

        class WhenHaveNoSpeakeFixture : public SpeakerServiceTest {};

        TEST_F(WhenHaveNoSpeakeFixture, ReceivedMatchVoiceprintResult_SpeakerChanged) {
            TFakeMatchItem matchVoiceprintResult = MakeMatchVoiceprintResult(PERS_ID_0);
            TSpeakerContextPtr expectedSpeakerContext = MakeSpeakerContext(matchVoiceprintResult);

            Sut.OnMatch(matchVoiceprintResult, 0);

            AssertSame(*Sut.GetActiveSpeaker(), *expectedSpeakerContext);
        }

        TEST_F(WhenHaveNoSpeakeFixture, ReceivedNoMatchVoiceprintResult_NoSpeaker) {
            Sut.OnNoMatch();

            ASSERT_EQ(Sut.GetActiveSpeaker(), nullptr);
        }

        TEST_F(WhenHaveNoSpeakeFixture, ReceivedBlackboxResponse_NoSpeaker) {
            TFakeBlackboxItem blackboxResponse = MakeBlackboxResponse(PERS_ID_0);

            Sut.OnBlackboxResponse(blackboxResponse, 0);

            ASSERT_EQ(Sut.GetActiveSpeaker(), nullptr);
        }

        TEST_F(WhenHaveNoSpeakeFixture, ReceivedDatasyncResponse_NoSpeaker) {
            TFakeDatasyncItem datasyncResponse = MakeDatasyncResponse(PERS_ID_0);

            Sut.OnDatasyncResponse(datasyncResponse, 0);

            ASSERT_EQ(Sut.GetActiveSpeaker(), nullptr);
        }

    }

    namespace WhenHaveActiveSpeakerAndReceivedUpdateForHim {

        class WhenHaveActiveSpeakerAndReceivedUpdateForHim : public SpeakerServiceTest {
        public:
            WhenHaveActiveSpeakerAndReceivedUpdateForHim()
                : MatchVoiceprintResult(MakeMatchVoiceprintResult(PERS_ID_0))
                , DatasyncResponse(MakeDatasyncResponse(PERS_ID_0))
                , BlackboxResponse(MakeBlackboxResponse(PERS_ID_0))
                , SpeakerContext(MakeAtomicShared<TSpeakerContext>())
            {
                EnrichSpeakerContext(*SpeakerContext, MatchVoiceprintResult);
            }

        protected:
            void SetUp() override {
                Sut.OnMatch(MatchVoiceprintResult, 0);
            }

        public:
            TFakeMatchItem MatchVoiceprintResult;
            TFakeDatasyncItem DatasyncResponse;
            TFakeBlackboxItem BlackboxResponse;

            TSpeakerContextPtr SpeakerContext;
        };

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForHim, ReceivedMatchVoiceprintResult_SpeakerNotChangedAndUpdated) {
            EnrichSpeakerContext(*SpeakerContext, MatchVoiceprintResult);

            Sut.OnMatch(MatchVoiceprintResult, 0);

            AssertSame(*Sut.GetActiveSpeaker(), *SpeakerContext);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForHim, ReceivedNoMatchVoiceprintResult_SpeakerChangedToNobody) {
            Sut.OnNoMatch();

            ASSERT_EQ(Sut.GetActiveSpeaker(), nullptr);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForHim, ReceivedDatasyncResponseForSameSpeaker_SpeakerNotChangedAndUpdated) {
            EnrichSpeakerContext(*SpeakerContext, DatasyncResponse);

            Sut.OnDatasyncResponse(DatasyncResponse, 0);

            AssertSame(*Sut.GetActiveSpeaker(), *SpeakerContext);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForHim, ReceivedBlackboxResponseForSameSpeaker_SpeakerNotChangedAndUpdated) {
            EnrichSpeakerContext(*SpeakerContext, BlackboxResponse);

            Sut.OnBlackboxResponse(BlackboxResponse, 0);

            AssertSame(*Sut.GetActiveSpeaker(), *SpeakerContext);
        }

    }

    namespace WhenHaveActiveSpeakerAndReceivedUpdateForNewOne {

        class WhenHaveActiveSpeakerAndReceivedUpdateForNewOne : public SpeakerServiceTest {
        public:
            WhenHaveActiveSpeakerAndReceivedUpdateForNewOne()
                : MatchVoiceprintResult(MakeMatchVoiceprintResult(PERS_ID_0))
                , DatasyncResponse(MakeDatasyncResponse(PERS_ID_0))
                , BlackboxResponse(MakeBlackboxResponse(PERS_ID_0))
                , SpeakerContext(MakeAtomicShared<TSpeakerContext>())
            {
                EnrichSpeakerContext(*SpeakerContext, MatchVoiceprintResult);
            }

        protected:
            void SetUp() override {
                Sut.OnMatch(MatchVoiceprintResult, 0);
            }

        public:
            TFakeMatchItem MatchVoiceprintResult;
            TFakeDatasyncItem DatasyncResponse;
            TFakeBlackboxItem BlackboxResponse;

            TSpeakerContextPtr SpeakerContext;
        };

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForNewOne, ReceivedMatchVoiceprintResult_SpeakerChangedToANewOne) {
            TFakeMatchItem matchVoiceprintResult = MakeMatchVoiceprintResult(PERS_ID_1);
            TSpeakerContextPtr expectedSpeakerContext = MakeSpeakerContext(matchVoiceprintResult);

            Sut.OnMatch(matchVoiceprintResult, 1);

            AssertSame(*Sut.GetActiveSpeaker(), *expectedSpeakerContext);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForNewOne, ReceivedNoMatchVoiceprintResult_SpeakerChangedToNobody) {
            Sut.OnNoMatch();

            ASSERT_EQ(Sut.GetActiveSpeaker(), nullptr);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForNewOne, ReceivedDatasyncResponseForSameSpeaker_SpeakerNotChangedAndNotUpdated) {
            TFakeDatasyncItem datasyncResponse = MakeDatasyncResponse(PERS_ID_1);

            Sut.OnDatasyncResponse(datasyncResponse, 1);

            AssertSame(*Sut.GetActiveSpeaker(), *SpeakerContext);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForNewOne, ReceivedBlackboxResponseForSameSpeaker_SpeakerNotChangedAndNotUpdated) {
            TFakeBlackboxItem blackboxResponse = MakeBlackboxResponse(PERS_ID_1);

            Sut.OnBlackboxResponse(blackboxResponse, 1);

            AssertSame(*Sut.GetActiveSpeaker(), *SpeakerContext);
        }

    }

    namespace WhenHaveActiveSpeakerAndReceivedUpdateForExistingButNotActive {

        class WhenHaveActiveSpeakerAndReceivedUpdateForExistingButNotActive : public SpeakerServiceTest {
        public:
            WhenHaveActiveSpeakerAndReceivedUpdateForExistingButNotActive()
                : InactiveMatchVoiceprintResult(MakeMatchVoiceprintResult(PERS_ID_0))
                , ActiveMatchVoiceprintResult(MakeMatchVoiceprintResult(PERS_ID_1))
                , InactiveSpeakerContext(MakeAtomicShared<TSpeakerContext>())
                , ActiveSpeakerContext(MakeAtomicShared<TSpeakerContext>())
            {
                EnrichSpeakerContext(*InactiveSpeakerContext, InactiveMatchVoiceprintResult);
                EnrichSpeakerContext(*ActiveSpeakerContext, ActiveMatchVoiceprintResult);
            }

        protected:
            void SetUp() override {
                Sut.OnMatch(InactiveMatchVoiceprintResult, 0);
                Sut.OnMatch(ActiveMatchVoiceprintResult, 1);
            }

        public:
            TFakeMatchItem InactiveMatchVoiceprintResult;
            TFakeMatchItem ActiveMatchVoiceprintResult;
            TSpeakerContextPtr InactiveSpeakerContext;
            TSpeakerContextPtr ActiveSpeakerContext;
        };

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForExistingButNotActive, ReceivedMatchVoiceprintResult_SwitchedSpeakerToInactive) {
            TFakeMatchItem matchVoiceprintResult = MakeMatchVoiceprintResult(PERS_ID_0);

            Sut.OnMatch(matchVoiceprintResult, 0);

            AssertSame(*Sut.GetActiveSpeaker(), *InactiveSpeakerContext);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForExistingButNotActive, ReceivedNoMatchVoiceprintResult_SpeakerChangedToNobody) {
            Sut.OnNoMatch();

            ASSERT_EQ(Sut.GetActiveSpeaker(), nullptr);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForExistingButNotActive, ReceivedDatasyncResponseForSameSpeaker_SpeakerNotChangedAndNotUpdated) {
            TFakeDatasyncItem datasyncResponse = MakeDatasyncResponse(PERS_ID_0);
            EnrichSpeakerContext(*InactiveSpeakerContext, datasyncResponse);

            Sut.OnDatasyncResponse(datasyncResponse, 0);

            AssertSame(*Sut.GetActiveSpeaker(), *ActiveSpeakerContext);
        }

        TEST_F(WhenHaveActiveSpeakerAndReceivedUpdateForExistingButNotActive, ReceivedBlackboxResponseForSameSpeaker_SpeakerNotChangedAndNotUpdated) {
            TFakeBlackboxItem blackboxResponse = MakeBlackboxResponse(PERS_ID_0);
            EnrichSpeakerContext(*InactiveSpeakerContext, blackboxResponse);

            Sut.OnBlackboxResponse(blackboxResponse, 0);

            AssertSame(*Sut.GetActiveSpeaker(), *ActiveSpeakerContext);
        }

    }

}
