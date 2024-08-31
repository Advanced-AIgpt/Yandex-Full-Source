#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/ut/service_ut_util.h>

namespace {

    using namespace NAlice::NCuttlefish::NAppHostServices::Tests::Util;

    TEST_F(BioContextSyncServiceTest, WhenReceivedAllRequiredItemsShouldReturnDirectives) {
        // arrange
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_ENROLLMENT_HEADERS)).WillByDefault(Return(true));
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_YABIO_CONTEXT)).WillByDefault(Return(true));

        // assert
        TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>> directives = MakeEnrollmentUpdateDirectives();
        EXPECT_CALL(MockProcessor, Process(NGTest::EqualsProto(EnrollmentHeaders), NGTest::EqualsProto(YabioContext))).Times(1);
        EXPECT_CALL(MockContext, AddEnrollmentUpdateDirective(NGTest::EqualsProto(*directives[0]), ITEM_TYPE_UPDATE_CLIENT_ENROLLMENT_DIRECTIVE)).Times(1);
        EXPECT_CALL(MockContext, AddEnrollmentUpdateDirective(NGTest::EqualsProto(*directives[1]), ITEM_TYPE_UPDATE_CLIENT_ENROLLMENT_DIRECTIVE)).Times(1);
        EXPECT_CALL(MockContext, AddEnrollmentUpdateDirective(NGTest::EqualsProto(*directives[2]), ITEM_TYPE_UPDATE_CLIENT_ENROLLMENT_DIRECTIVE)).Times(1);

        RunTest();
    }

    TEST_F(BioContextSyncServiceTest, WhenReceivedOnlyEnrollmentHeadersShouldReturnNoDirecives) {
        // arrange
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_ENROLLMENT_HEADERS)).WillByDefault(Return(true));
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_YABIO_CONTEXT)).WillByDefault(Return(false));

        // assert
        EXPECT_CALL(MockContext, GetYabioContext(_)).Times(0);
        EXPECT_CALL(MockProcessor, Process(_, _)).Times(0);
        EXPECT_CALL(MockContext, AddEnrollmentUpdateDirective(_, _)).Times(0);

        RunTest();
    }

    TEST_F(BioContextSyncServiceTest, WhenReceivedOnlyYabioContextShouldReturnNoDirecives) {
        // arrange
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_ENROLLMENT_HEADERS)).WillByDefault(Return(false));
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_YABIO_CONTEXT)).WillByDefault(Return(true));

        // assert
        EXPECT_CALL(MockContext, GetEnrollmentHeaders(_)).Times(0);
        EXPECT_CALL(MockProcessor, Process(_, _)).Times(0);
        EXPECT_CALL(MockContext, AddEnrollmentUpdateDirective(_, _)).Times(0);

        RunTest();
    }

    TEST_F(BioContextSyncServiceTest, WhenReceivedNoInputItemsShouldReturnNoDirectives) {
        // arrange
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_ENROLLMENT_HEADERS)).WillByDefault(Return(false));
        ON_CALL(MockContext, HasProtobufItem(ITEM_TYPE_YABIO_CONTEXT)).WillByDefault(Return(false));

        // assert
        EXPECT_CALL(MockContext, GetEnrollmentHeaders(_)).Times(0);
        EXPECT_CALL(MockContext, GetYabioContext(_)).Times(0);
        EXPECT_CALL(MockProcessor, Process(_, _)).Times(0);
        EXPECT_CALL(MockContext, AddEnrollmentUpdateDirective(_, _)).Times(0);

        RunTest();
    }
}
