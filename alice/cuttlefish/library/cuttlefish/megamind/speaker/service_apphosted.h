#pragma once

#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>	
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/personalization.pb.h>

#include <apphost/api/service/cpp/service.h>
#include <util/generic/ptr.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    template <typename TSpeakerServiceImpl>
    class TApphostedSpeakerService : public TSpeakerServiceImpl {
    public:
        TApphostedSpeakerService(TSourceMetrics& metrics, TLogContext logContext)
            : Metrics(metrics)
            , LogContext(logContext) {
        }

        void OnNextInput(const NAppHost::IServiceContext& ahContext) {
            const auto& refs = ahContext.GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);

            for (auto it = refs.begin(); it != refs.end(); ++it) {
                TStringBuf itemType = it.GetType();

                // MatchVoiceprintResult
                if (itemType.StartsWith(ITEM_TYPE_VOICEPRINT_MATCH_RESULT)) {
                    int iteration = GetIterationFromItemType(itemType, ITEM_TYPE_VOICEPRINT_MATCH_RESULT);
                    TSpeakerServiceImpl::OnMatch(ParseProtobufItem<NAliceProtocol::TMatchVoiceprintResult>(*it), iteration);
                    TraceReceivedData(itemType);
                }

                // NoMatchVoiceprintResult
                if (itemType == ITEM_TYPE_VOICEPRINT_NO_MATCH_RESULT) {
                    TSpeakerServiceImpl::OnNoMatch();
                    TraceReceivedData(itemType);
                }

                // DatasyncHttpResponse
                if (itemType.StartsWith(ITEM_TYPE_GUEST_DATASYNC_HTTP_RESPONSE)) {
                    int iteration = GetIterationFromItemType(itemType, ITEM_TYPE_GUEST_DATASYNC_HTTP_RESPONSE);
                    TSpeakerServiceImpl::OnDatasyncResponse(ParseProtobufItem<NAppHostHttp::THttpResponse>(*it), iteration);
                    TraceReceivedData(itemType);
                }

                // BlackboxHttpResponse
                if (itemType.StartsWith(ITEM_TYPE_GUEST_BLACKBOX_HTTP_RESPONSE)) {
                    int iteration = GetIterationFromItemType(itemType, ITEM_TYPE_GUEST_BLACKBOX_HTTP_RESPONSE);
                    TSpeakerServiceImpl::OnBlackboxResponse(ParseProtobufItem<NAppHostHttp::THttpResponse>(*it), iteration);
                    TraceReceivedData(itemType);
                }
            }
        }

    private:
        void TraceReceivedData(TStringBuf itemType) const {
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "TApphostedSpeakerService: Received '" << itemType << '\'');
            Metrics.PushRate("mm_guest_context", itemType);	
        }

        static int GetIterationFromItemType(TStringBuf itemType, TStringBuf itemTypePrefix) {	
            itemType.Skip(itemTypePrefix.size() + 1); // 1 is a '_' delimiter before index	
            return IntFromString<int, 10>(itemType.data(), itemType.size());	
        }

    private:
        TSourceMetrics& Metrics;
        TLogContext LogContext;
    };

} // namespace NAlice::NCuttlefish::NAppHostServices
