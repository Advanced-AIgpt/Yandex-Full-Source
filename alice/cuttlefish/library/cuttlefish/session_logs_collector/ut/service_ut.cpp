#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/session_logs_collector/service.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/uniproxy2.pb.h>

#include <apphost/api/service/cpp/service_context.h>
#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

Y_UNIT_TEST_SUITE(SessionLogsCollector) {

    Y_UNIT_TEST(TestNoInputItems) {
        NAppHost::NService::TTestContext testContext;

        SessionLogsCollector(testContext, TLogContext(new TSelfFlushLogFrame(nullptr), nullptr));
        UNIT_ASSERT(testContext.HasProtobufItem(ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS));

        const auto& directives = testContext.GetOnlyProtobufItem<NAliceProtocol::TUniproxyDirectives>(
            ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS
        );
        UNIT_ASSERT(directives.GetDirectives().empty());
    }

    Y_UNIT_TEST(TestWithInputItems) {
        NAppHost::NService::TTestContext testContext;

        testContext.AddProtobufItem(
            NAliceProtocol::TUniproxyDirective(),
            ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG_FROM_MM_RUN
        );
        testContext.AddProtobufItem(
            NAliceProtocol::TUniproxyDirective(),
            ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG_FROM_MM_RUN
        );
        testContext.AddProtobufItem(
            NAliceProtocol::TUniproxyDirective(),
            ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG_FROM_MM_APPLY
        );
        testContext.AddProtobufItem(
            NAliceProtocol::TUniproxyDirective(),
            "wrong_item_type"
        );

        SessionLogsCollector(testContext, TLogContext(new TSelfFlushLogFrame(nullptr), nullptr));
        UNIT_ASSERT(testContext.HasProtobufItem(ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS));

        const auto& directives = testContext.GetOnlyProtobufItem<NAliceProtocol::TUniproxyDirectives>(
            ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS
        );
        UNIT_ASSERT_EQUAL(directives.DirectivesSize(), 3);
    }
}
