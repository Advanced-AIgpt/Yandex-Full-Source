#include <alice/cuttlefish/library/cuttlefish/stream_converter/matched_user_event_handler.h>
#include <alice/cuttlefish/library/cuttlefish/common/blackbox.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/library/json/json.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    TMatchedUserEventHandler::TMatchedUserEventHandler(
        NAppHost::TServiceContextPtr ahContext,
        TLogContext logContext,
        TSourceMetrics& metrics
    )
        : AhContext(ahContext)
        , LogContext(logContext)
        , Metrics(metrics)
    {}

    void TMatchedUserEventHandler::OnEvent(
        const NVoicetech::NUniproxy2::TMessage& message,
        const TString& ipAddress
    ) {
        if (const auto* node = message.Json.GetValueByPath("event.payload.request.guest_user_options"); node && node->IsMap()) {
            NAliceProtocol::TMatchVoiceprintResult matchVoiceprintResult;
            *matchVoiceprintResult.MutableGuestOptions() = JsonToProto<NAlice::TGuestOptions>(*node);

            if (matchVoiceprintResult.GetGuestOptions().GetStatus() == NAlice::TGuestOptions::Match) {
                OnMatch(matchVoiceprintResult, ipAddress);
            } else {
                OnNoMatch(matchVoiceprintResult);
            }
        }
    }

    void TMatchedUserEventHandler::OnMatch(
        const NAliceProtocol::TMatchVoiceprintResult& matchResult,
        const TString& ipAddress
    ) {
        auto [ticket, isLoaded] = GetTicket(matchResult.GetGuestOptions().GetPersId());

        // Notify Megamind, that speaking guest has been recognized
        // ITEM_TYPE_VOICEPRINT_MATCH_RESULT_(0..N), N - max number of guests
        {
            Metrics.PushRate("match_voiceprint_result", "match", "multiacc");
            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMatchVoiceprintResult>(matchResult);

            AhContext->AddProtobufItem(
                matchResult,
                TStringBuilder() << ITEM_TYPE_VOICEPRINT_MATCH_RESULT << '_' << ticket);
        }

        if (!isLoaded) {
            // Blackbox request (initiate context load)
            // ITEM_TYPE_GUEST_BLACKBOX_HTTP_REQUEST_(0..N), N - max number of guests
            {
                Metrics.PushRate("request", "oauth_token", "blackbox");
                LogContext.LogEventInfoCombo<NEvClass::SendToAppHostBlackboxHttpRequest>("oauth_token");

                AhContext->AddProtobufItem(
                    TBlackboxClient::GetUidForOAuth(matchResult.GetGuestOptions().GetOAuthToken(), ipAddress),
                    TStringBuilder() << ITEM_TYPE_GUEST_BLACKBOX_HTTP_REQUEST << '_' << ticket);
            }
        }
        // Else loading contexts for this guest has already starded
    }

    void TMatchedUserEventHandler::OnNoMatch(const NAliceProtocol::TMatchVoiceprintResult& matchResult) {
        // TODO @aradzevich: send NO_MATCH-es directly to megamind when client start sending match events only on recognition result change
        Metrics.PushRate("match_voiceprint_result", "no_match", "multiacc");
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostNoMatchVoiceprintResult>();
        AhContext->AddProtobufItem(matchResult, ITEM_TYPE_VOICEPRINT_NO_MATCH_RESULT);
    }

    std::pair<int, bool> TMatchedUserEventHandler::GetTicket(const TString& persId) {
        auto it = Tickets.find(persId);
        if (it != Tickets.end()) {
            return { it->second, true };
        }

        int newTicket = Tickets.size() + 1;
        Tickets[persId] = newTicket;

        return { newTicket, false };
    }

}


