#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/uniproxy2.pb.h>

#include <voicetech/library/idl/log/events.ev.pb.h>

namespace {

    class TSessionLogsCollector {
    public:
        TSessionLogsCollector(NAppHost::IServiceContext& serviceCtx, const NAlice::NCuttlefish::TLogContext& logCtx)
            : ServiceCtx_(serviceCtx)
            , LogCtx_(logCtx)
            , Metrics_(ServiceCtx_, "session_logs_collector") {}

        void Do() {
            using namespace NAlice::NCuttlefish;

            NAliceProtocol::TUniproxyDirectives directives;

            AddItems(directives, ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG_FROM_MM_RUN);
            AddItems(directives, ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG_FROM_MM_APPLY);

            ServiceCtx_.AddProtobufItem(directives, ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS);

            LogCtx_.LogEventInfoCombo<NEvClass::SendToAppHostUniproxyDirective>(directives.ShortUtf8DebugString());
            Metrics_.PushRate(ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS, "sent");
        }

    private:
        void AddItems(NAliceProtocol::TUniproxyDirectives& directives, const TStringBuf type) {
            if (!ServiceCtx_.HasProtobufItem(type)) {
                return;
            }

            auto items = ServiceCtx_.GetProtobufItemRefs(type);

            for (auto it = items.begin(); it != items.end(); ++it) {
                try {
                    NAliceProtocol::TUniproxyDirective directive;
                    NAlice::NCuttlefish::ParseProtobufItem(*it, directive);

                    Metrics_.PushRate(type, "received");
                    LogCtx_.LogEventInfoCombo<NEvClass::RecvFromAppHostUniproxyDirective>(directive.ShortUtf8DebugString());

                    *directives.AddDirectives() = directive;
                } catch (...) {
                    Metrics_.PushRate(type, "error");
                    LogCtx_.LogEventErrorCombo<NEvClass::SessionLogsCollectorAddDirectiveError>(CurrentExceptionMessage());
                }
            }
        }

        NAppHost::IServiceContext& ServiceCtx_;
        const NAlice::NCuttlefish::TLogContext& LogCtx_;
        NAlice::NCuttlefish::TSourceMetrics Metrics_;
    };

} // namespace

namespace NAlice::NCuttlefish::NAppHostServices {

    void SessionLogsCollector(NAppHost::IServiceContext& serviceCtx, TLogContext logCtx) {
        TSessionLogsCollector sessionLogsCollector(serviceCtx, logCtx);
        sessionLogsCollector.Do();
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
