#include "processor.h"

#include <alice/wonderlogs/rt/protos/uniproxy_event.pb.h>
#include <alice/wonderlogs/rt/protos/uuid_message_id.pb.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/parsers/uniproxy.h>

#include <ads/bsyeti/big_rt/lib/queue/qyt/queue.h>

#include <library/cpp/framing/unpacker.h>
#include <library/cpp/json/json_reader.h>

#include <utility>

namespace NAlice::NWonderlogs {

TUniproxyCreator::TGroupedChunk TUniproxyCreator::PrepareGroupedChunk(TString /* dataSource */, TManager& stateManager,
                                                                      NBigRT::TMessageBatch data) {
    TGroupedChunk groupedChunk;

    auto ctx = NSFStats::TSolomonContext(SensorsContext, /* labels= */ {}).Detached();

    ui64 unpackFailsPerSecond = 0;
    ui64 unpacksPerSecond = 0;
    ui64 failsParseProto = 0;
    ui64 invalidUuid = 0;
    ui64 invalidMessageId = 0;
    ui64 corruptedMessages = 0;

    for (auto& message : data.Messages) {
        if (!message.Unpack()) {
            ++unpackFailsPerSecond;
            continue;
        }
        ++unpacksPerSecond;
        TUniproxyEvent uniproxyEvent;
        if (!uniproxyEvent.ParseFromString(message.Data)) {
            ++failsParseProto;
            continue;
        }

        TMaybe<TString> uuid;
        TMaybe<TString> messageId;

        switch (uniproxyEvent.GetValueCase()) {
            case TUniproxyEvent::ValueCase::kMegamindRequest: {
                uuid = uniproxyEvent.GetMegamindRequest().GetUuid();
                messageId = uniproxyEvent.GetMegamindRequest().GetMessageId();
                break;
            }
            case TUniproxyEvent::ValueCase::kMegamindResponse: {
                uuid = uniproxyEvent.GetMegamindResponse().GetUuid();
                messageId = uniproxyEvent.GetMegamindResponse().GetMessageId();
                break;
            }
            case TUniproxyEvent::ValueCase::VALUE_NOT_SET: {
                continue;
                break;
            }
        }
        if (uuid && messageId) {
            auto groupId = std::tuple(*uuid, *messageId);
            auto request = stateManager.RequestState(std::move(groupId));
            groupedChunk[request].push_back(std::move(uniproxyEvent));
        }
    }

    ctx.Get<NSFStats::TSumMetric<ui64>>("fails_unpack_per_second").Inc(unpackFailsPerSecond);
    ctx.Get<NSFStats::TSumMetric<ui64>>("unpacks_per_second").Inc(unpacksPerSecond);
    ctx.Get<NSFStats::TSumMetric<ui64>>("fails_parse_proto_per_second").Inc(failsParseProto);
    ctx.Get<NSFStats::TSumMetric<ui64>>("invalid_uuid_per_second").Inc(invalidUuid);
    ctx.Get<NSFStats::TSumMetric<ui64>>("invalid_message_id_per_second").Inc(invalidMessageId);
    ctx.Get<NSFStats::TSumMetric<ui64>>("corrupted_messages_per_second").Inc(corruptedMessages);

    return groupedChunk;
}

void TUniproxyCreator::ProcessGroupedChunk(TString /* dataSource */, TGroupedChunk groupedRows) {
    auto ctx = NSFStats::TSolomonContext(SensorsContext, /* labels= */ {}).Detached();
    for (const auto& [request, values] : groupedRows) {
        auto& statePtr = request->GetState();
        Y_ENSURE(statePtr);

        {
            const auto& keys = request->GetStateId();
            TUuidMessageId uuidMessageId;
            uuidMessageId.SetUuid(get<0>(keys));
            uuidMessageId.SetMessageId(get<1>(keys));
            uuidMessageId.SetTimestampMs(TInstant::Now().MilliSeconds());
            const auto hash = HashStringToUi64(uuidMessageId.GetUuid());
            UuidMessageId[hash % Config.GetUuidMessageIdQueue().GetShardsCount()].push_back(
                uuidMessageId.SerializeAsString());
        }

        auto& uniproxyPreparedWrapper = statePtr->GetMessage();
        auto& uniproxyPrepared = *uniproxyPreparedWrapper.MutableValue();

        for (const auto& message : values) {
            switch (message.GetValueCase()) {
                case TUniproxyEvent::VALUE_NOT_SET:
                    continue;
                case TUniproxyEvent::kMegamindResponse: {
                    const auto& megamindResponse = message.GetMegamindResponse();
                    uniproxyPrepared.SetUuid(megamindResponse.GetUuid());
                    uniproxyPrepared.SetMessageId(megamindResponse.GetMessageId());
                    uniproxyPrepared.SetMegamindRequestId(megamindResponse.GetMegamindRequestId());
                    uniproxyPrepared.SetTimestampLogMs(megamindResponse.GetTimestampLogMs());
                    uniproxyPrepared.SetMegamindResponseId(megamindResponse.GetMegamindResponseId());
                    break;
                }
                case TUniproxyEvent::kMegamindRequest: {
                    const auto& megamindRequest = message.GetMegamindRequest();
                    uniproxyPrepared.SetUuid(megamindRequest.GetUuid());
                    uniproxyPrepared.SetMessageId(megamindRequest.GetMessageId());
                    uniproxyPrepared.SetMegamindRequestId(megamindRequest.GetMegamindRequestId());
                    uniproxyPrepared.SetTimestampLogMs(megamindRequest.GetTimestampLogMs());
                    break;
                }
            }
        }
        statePtr->SetDirty();
    }
}

NYT::TFuture<TUniproxyCreator::TPrepareForAsyncWriteResult> TUniproxyCreator::PrepareForAsyncWrite() {
    auto dataForWriteUuidMessageId = MakeAtomicShared<decltype(UuidMessageId)>(std::move(UuidMessageId));
    UuidMessageId.clear();
    return NYT::MakeFuture<TPrepareForAsyncWriteResult>(
        {.AsyncWriter = [this, dataForWriteUuidMessageId](NYT::NApi::ITransactionPtr tx) {
            NBigRT::TYtQueue{Config.GetUuidMessageIdQueue().GetOutputQueue(), tx->GetClient()}.Write(
                tx, *dataForWriteUuidMessageId, Config.GetUuidMessageIdQueue().GetOutputCodec());
        }});
}

} // namespace NAlice::NWonderlogs
