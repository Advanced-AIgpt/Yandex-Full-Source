#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/ut/processor_ut_util.h>

namespace {

    using namespace NAlice::NCuttlefish::NAppHostServices::Tests::Util;

    class EnrollmentRepositoryReturnsNoData : public Fixture {
    protected:
        void SetUp() override {
            ON_CALL((*EnrollmentRepository), TryLoad(NGTest::EqualsProto(YabioConext))).WillByDefault(Return(false));
        }
    };

    class EnrollmentRepositoryHasDataButHasNoUpdates : public Fixture {
    protected:
        void SetUp() override {
            ON_CALL((*EnrollmentRepository), TryLoad(NGTest::EqualsProto(YabioConext))).WillByDefault(Return(true));
            ON_CALL((*EnrollmentRepository), CheckUpdateFor).WillByDefault(ReturnNull);
        }
    };

    class EnrollmentRepositoryHasFreshUpdates : public Fixture {
    protected:
        void SetUp() override {
            ON_CALL((*EnrollmentRepository), TryLoad(NGTest::EqualsProto(YabioConext))).WillByDefault(Return(true));
            ON_CALL((*EnrollmentRepository), CheckUpdateFor).WillByDefault(ReturnDirectiveForRequest);
        }
    };

    RUN_TEST(WhenHasNoBiometryForUser, EnrollmentRepositoryReturnsNoData, ShouldReturnNoDirectives, ::testing::Values(
        std::make_pair(
            SendRequest({}),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Guest() }),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Owner() }),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() }),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Guest(), Owner(), __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() }),
            GetResponse({})
        )
    ));

    RUN_TEST(WhenHasBiometryForUserNotChanged, EnrollmentRepositoryHasDataButHasNoUpdates, ShouldReturnNoDirectives, ::testing::Values(
        std::make_pair(
            SendRequest({}),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Guest() }),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Owner() }),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() }),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Guest(), Owner(), __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() }),
            GetResponse({})
        )
    ));

    RUN_TEST(WhenHasBiometryForUserUpdated, EnrollmentRepositoryHasFreshUpdates, ShouldReturnDirectivesForEveryoneExceptGuest, ::testing::Values(
        std::make_pair(
            SendRequest({}),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Guest() }),
            GetResponse({})
        ),
        std::make_pair(
            SendRequest({ Owner() }),
            GetResponse({ Owner() })
        ),
        std::make_pair(
            SendRequest({ __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() }),
            GetResponse({ __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() })
        ),
        std::make_pair(
            SendRequest({ Guest(), Owner(), __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() }),
            GetResponse({ Owner(), __SYSTEM_OWNER_DO_NOT_USE_AFTER_2021() })
        )
    ));
}
