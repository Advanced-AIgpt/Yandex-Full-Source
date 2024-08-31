#pragma once

#include <alice/cachalot/library/golovan.h>

#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <apphost/api/service/cpp/service_context.h>


namespace NCachalot {

    class TCachaLog : public NAlice::NCuttlefish::TLogContext {
    public:
        explicit TCachaLog(NRTLog::TRequestLoggerPtr rtLog)
            // Cachalot writes to the log frame simultaneously from different threads
            // so we need "needAlwaysSafeAdd"
            : NAlice::NCuttlefish::TLogContext(
                NAlice::NCuttlefish::SpawnLogFrame(/* needAlwaysSafeAdd = */ true),
                std::move(rtLog)
            )
            , StartTime(TInstant::Now())
        {}

        TInstant GetStartTime() const {
            return StartTime;
        }

    protected:
        TInstant StartTime;
    };


    class TChronicler : public TThrRefBase {
    public:
        TChronicler()
            : CachaLog(NRTLog::TRequestLoggerPtr())
        {
        }

        explicit TChronicler(const TString& rtLogToken)
            : CachaLog(NAlice::NCuttlefish::TRtLogClient::Instance().CreateRequestLogger(rtLogToken, false))
        {
        }

        template <typename TEvent>
        void LogEvent(const TEvent& event) {
            CachaLog.Frame().LogEvent(event);
        }

        TCachaLog& Log() {
            return CachaLog;
        }

        TInstant GetStartTime() const {
            return CachaLog.GetStartTime();
        }

    protected:
        TCachaLog CachaLog;
    };

    using TChroniclerPtr = TIntrusivePtr<TChronicler>;


    TChroniclerPtr MakeLoggerWithRtLogTokenFromAppHostContext(const NAppHost::IServiceContext& ahContext);

}   // namespace NCachalot
