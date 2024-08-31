#include "processor.h"

#include <alice/wonderlogs/rt/protos/uuid_message_id.pb.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/parsers/megamind.h>

#include <ads/bsyeti/big_rt/lib/queue/qyt/queue.h>

#include <library/cpp/framing/unpacker.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <utility>

namespace NAlice::NWonderlogs {

TMegamindCreator::TGroupedChunk TMegamindCreator::PrepareGroupedChunk(TString /* dataSource */, TManager& stateManager,
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
        TMegamindPrepared::TMegamindRequestResponse requestResponse;
        if (!requestResponse.ParseFromString(message.Data)) {
            ++failsParseProto;
            continue;
        }

        TString uuid = requestResponse.GetUuid();
        TString messageId = requestResponse.GetMessageId();

        auto groupId = std::tuple(uuid, messageId);
        auto request = stateManager.RequestState(std::move(groupId));
        groupedChunk[request].push_back(std::move(requestResponse));
    }

    ctx.Get<NSFStats::TSumMetric<ui64>>("fails_unpack_per_second").Inc(unpackFailsPerSecond);
    ctx.Get<NSFStats::TSumMetric<ui64>>("unpacks_per_second").Inc(unpacksPerSecond);
    ctx.Get<NSFStats::TSumMetric<ui64>>("fails_parse_proto_per_second").Inc(failsParseProto);
    ctx.Get<NSFStats::TSumMetric<ui64>>("invalid_uuid_per_second").Inc(invalidUuid);
    ctx.Get<NSFStats::TSumMetric<ui64>>("invalid_message_id_per_second").Inc(invalidMessageId);
    ctx.Get<NSFStats::TSumMetric<ui64>>("corrupted_messages_per_second").Inc(corruptedMessages);

    return groupedChunk;
}

void TMegamindCreator::ProcessGroupedChunk(TString /* dataSource */, TGroupedChunk groupedRows) {
    auto ctx = NSFStats::TSolomonContext(SensorsContext, /* labels= */ {}).Detached();

    for (const auto& [request, values] : groupedRows) {
        auto& statePtr = request->GetState();
        Y_ENSURE(statePtr);
        const auto& keys = request->GetStateId();

        {
            TUuidMessageId uuidMessageId;
            uuidMessageId.SetUuid(get<0>(keys));
            uuidMessageId.SetMessageId(get<1>(keys));
            uuidMessageId.SetTimestampMs(TInstant::Now().MilliSeconds());
            const auto hash = HashStringToUi64(uuidMessageId.GetUuid());
            UuidMessageId[hash % Config.GetUuidMessageIdQueue().GetShardsCount()].push_back(
                uuidMessageId.SerializeAsString());
        }

        auto& megamindPreparedWrapper = statePtr->GetMessage();
        TMegamindPreparedWrapper::TValues mmValues;

        for (const auto& message : values) {
            *mmValues.AddValues() = message;
        }

        megamindPreparedWrapper.SetValuesBase64(Base64Encode(mmValues.SerializeAsString()));
        statePtr->SetDirty();
    }
}

NYT::TFuture<TMegamindCreator::TPrepareForAsyncWriteResult> TMegamindCreator::PrepareForAsyncWrite() {
    auto dataForWriteUuidMessageId = MakeAtomicShared<decltype(UuidMessageId)>(std::move(UuidMessageId));
    UuidMessageId.clear();
    return NYT::MakeFuture<TPrepareForAsyncWriteResult>(
        {.AsyncWriter = [this, dataForWriteUuidMessageId](NYT::NApi::ITransactionPtr tx) {
            NBigRT::TYtQueue{Config.GetUuidMessageIdQueue().GetOutputQueue(), tx->GetClient()}.Write(
                tx, *dataForWriteUuidMessageId, Config.GetUuidMessageIdQueue().GetOutputCodec());
        }});
}

} // namespace NAlice::NWonderlogs
