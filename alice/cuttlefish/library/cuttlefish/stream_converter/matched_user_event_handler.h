#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/personalization.pb.h>
#include <apphost/api/service/cpp/service.h>

#include <voicetech/library/messages/message.h>
#include <util/generic/hash.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    class TMatchedUserEventHandler {
    public:
        TMatchedUserEventHandler(NAppHost::TServiceContextPtr ahContext, TLogContext logContext, TSourceMetrics& metrics);
        void OnEvent(const NVoicetech::NUniproxy2::TMessage& message, const TString& ipAddress);

    private:
        void OnMatch(const NAliceProtocol::TMatchVoiceprintResult& matchResult, const TString& ipAddress);
        void OnNoMatch(const NAliceProtocol::TMatchVoiceprintResult& matchResult);
        std::pair<int, bool> GetTicket(const TString& persId);

    private:
        THashMap<TString, int> Tickets;

        NAppHost::TServiceContextPtr AhContext;
        TLogContext LogContext;
        TSourceMetrics& Metrics;
    };

}