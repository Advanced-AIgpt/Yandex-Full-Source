#pragma once

#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/processor.h>
#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/enrollment_repository.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/bio_context_sync.pb.h>
#include <voicetech/library/proto_api/yabio.pb.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/gtest_protobuf/matcher.h>
#include <library/cpp/testing/gtest/gtest.h>

namespace NAlice::NCuttlefish::NAppHostServices::Tests::Util {

    using namespace NAlice::NCuttlefish;
    using namespace NAlice::NCuttlefish::NAppHostServices;

    using ::testing::_;
    using ::testing::Return;
    using ::testing::ReturnPointee;

    THolder<NAliceProtocol::TEnrollmentUpdateDirective> ReturnDirectiveForRequest(NAlice::TEnrollmentHeader enrollmentHeader) {
        auto directiveHolder = MakeHolder<NAliceProtocol::TEnrollmentUpdateDirective>();
        directiveHolder->MutableHeader()->Swap(&enrollmentHeader);

        return directiveHolder;
    };

    THolder<NAliceProtocol::TEnrollmentUpdateDirective> ReturnNull(NAlice::TEnrollmentHeader) {
        return nullptr;
    };

    TVector<NAliceProtocol::TEnrollmentUpdateDirective> GetResponse(TVector<NAlice::TEnrollmentHeader> requests) {
        TVector<NAliceProtocol::TEnrollmentUpdateDirective> directives(requests.size());

        for (uint i = 0; i < requests.size(); i++) {
            directives[i].MutableHeader()->Swap(&requests[i]);
        }

        return directives;
    };

    NAlice::TEnrollmentHeaders SendRequest(TVector<NAlice::TEnrollmentHeader> requests) {
        NAlice::TEnrollmentHeaders proto;

        for (NAlice::TEnrollmentHeader& request : requests) {
            proto.MutableHeaders()->Add(std::move(request));
        }

        return proto;
    }

    NAlice::TEnrollmentHeader MakeRequest(NAlice::EUserType userType) {
        NAlice::TEnrollmentHeader user;
        user.SetUserType(userType);

        return user;
    }

    NAlice::TEnrollmentHeader Guest() {
        return MakeRequest(NAlice::GUEST);
    }

    NAlice::TEnrollmentHeader Owner() {
        return MakeRequest(NAlice::OWNER);
    }

    NAlice::TEnrollmentHeader __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() {
        return MakeRequest(NAlice::__SYSTEM_OWNER_DO_NOT_USE_AFTER_2021);
    }

    struct TMockEnrollmentRepository : public IEnrollmentRepository {
        MOCK_METHOD(bool, TryLoad, (const YabioProtobuf::YabioContext&), (override));
        MOCK_METHOD(THolder<NAliceProtocol::TEnrollmentUpdateDirective>, CheckUpdateFor, (NAlice::TEnrollmentHeader), (const, override));
    };

    template <typename TCollection>
    void AssertSameProtos(const TCollection& actual, const TCollection& expected) {
        ASSERT_EQ(actual.size(), expected.size());

        for (uint i = 0; i < expected.size(); i++) {
            EXPECT_THAT(actual[i], NGTest::EqualsProto(expected[i]));
        }
    }

    class Fixture : public ::testing::TestWithParam<std::pair<
        NAlice::TEnrollmentHeaders,
        TVector<NAliceProtocol::TEnrollmentUpdateDirective>
    >> {
    public:
        Fixture()
            : EnrollmentRepository(MakeIntrusive<TMockEnrollmentRepository>())
            , Metrics("test_metrics")
            , Processor(TLogContext(new TSelfFlushLogFrame(nullptr), nullptr), Metrics, EnrollmentRepository)
        {
        }

        void RunTest() {
            auto [requests, expectDirectives] = GetParam();
            AssertSameProtos(Unref(Processor.Process(requests, YabioConext)), expectDirectives);
        }

    private:
        TVector<NAliceProtocol::TEnrollmentUpdateDirective> Unref(TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>>&& holders) {
            TVector<NAliceProtocol::TEnrollmentUpdateDirective> unrefed(holders.size());

            for (uint i = 0; i < holders.size(); i++) {
                unrefed[i].Swap(holders[i].Get());
            }

            return unrefed;
        }

    public:
        TIntrusivePtr<TMockEnrollmentRepository> EnrollmentRepository;
        TSourceMetrics Metrics;
        TBioContextSyncProcessor Processor;

        YabioProtobuf::YabioContext YabioConext;
    };

    #define RUN_TEST(TestSuite, Fixture, Test, TestCases) \
        TEST_P(Fixture, Test) { RunTest(); }   \
        INSTANTIATE_TEST_SUITE_P(TestSuite, Fixture, TestCases);

    
}
