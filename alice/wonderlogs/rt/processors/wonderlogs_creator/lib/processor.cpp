#include "processor.h"

#include "alice/wonderlogs/library/builders/megamind.h"
#include "alice/wonderlogs/library/common/utils.h"

#include <alice/wonderlogs/rt/library/common/utils.h>

#include <alice/wonderlogs/library/parsers/uniproxy.h>

#include <ads/bsyeti/big_rt/lib/queue/qyt/queue.h>

#include <library/cpp/framing/unpacker.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <chrono>
#include <thread>

namespace NAlice::NWonderlogs {

TWonderlogsCreator::TGroupedChunk TWonderlogsCreator::PrepareGroupedChunk(TString /* dataSource */,
                                                                          TWonderlogsCreator::TManager& stateManager,
                                                                          NBigRT::TMessageBatch data) {
    TGroupedChunk groupedChunk;

    auto ctx = NSFStats::TSolomonContext(SensorsContext, /* labels= */ {}).Detached();

    ui64 unpackFailsPerSecond = 0;
    ui64 corruptedMessages = 0;
    ui64 failsParseProto = 0;

    for (auto& message : data.Messages) {
        if (!message.Unpack()) {
            ++unpackFailsPerSecond;
            continue;
        }
        TUuidMessageId uuidMessageId;
        if (!uuidMessageId.ParseFromString(message.Data)) {
            ++failsParseProto;
            continue;
        }

        {
            // use delayed events
            i64 passedMilliseconds =
                (TInstant::Now() - TInstant::MilliSeconds(uuidMessageId.GetTimestampMs())).MilliSeconds();
            if (passedMilliseconds < static_cast<i64>(Config.GetSleepMilliseconds())) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(Config.GetSleepMilliseconds() - passedMilliseconds));
            }
        }

        auto stateRequest = stateManager.RequestState();
        const auto key = std::make_tuple(uuidMessageId.GetUuid(), uuidMessageId.GetMessageId());
        stateRequest->Set(StateDescriptors.UniproxyPreparedDescriptor, key);
        stateRequest->Set(StateDescriptors.MegamindPreparedDescriptor, key);
        stateRequest->Set(StateDescriptors.UuidMessageIdDescriptor, key);
        groupedChunk[std::move(stateRequest)].push_back(std::move(uuidMessageId));
    }
    ctx.Get<NSFStats::TSumMetric<ui64>>("fails_unpack_per_second").Inc(unpackFailsPerSecond);
    ctx.Get<NSFStats::TSumMetric<ui64>>("fails_parse_proto_per_second").Inc(failsParseProto);
    ctx.Get<NSFStats::TSumMetric<ui64>>("corrupted_messages_per_second").Inc(corruptedMessages);
    return groupedChunk;
}

void TWonderlogsCreator::ProcessGroupedChunk(TString /* dataSource */, TWonderlogsCreator::TGroupedChunk groupedRows) {
    auto ctx = NSFStats::TSolomonContext(SensorsContext, /* labels= */ {}).Detached();
    for (auto& [request, values] : groupedRows) {
        auto& uuidMessageIdState = request->Get(StateDescriptors.UuidMessageIdDescriptor)->GetState();
        if (uuidMessageIdState->GetMessage().HasTimestampMs()) {
            continue;
        }
        auto sortedValues = values;
        Sort(sortedValues,
             [](const auto& lhs, const auto& rhs) { return lhs.GetTimestampMs() < rhs.GetTimestampMs(); });

        const auto& uniproxyPreparedWrapper =
            request->Get(StateDescriptors.UniproxyPreparedDescriptor)->GetState()->GetMessage();
        const auto& keys = request->Get(StateDescriptors.UniproxyPreparedDescriptor)->GetStateId();
        const auto& megamindRequesResponses =
            request->Get(StateDescriptors.MegamindPreparedDescriptor)->GetState()->GetMessage();
        TMegamindPreparedWrapper::TValues megamindValues;
        if (!megamindValues.ParseFromString(Base64Decode(megamindRequesResponses.GetValuesBase64()))) {
            continue;
        }
        TWonderlog wonderlog;
        wonderlog.SetUuid(get<0>(keys));
        wonderlog.SetMessageId(get<1>(keys));
        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared;
        if (uniproxyPreparedWrapper.HasValue()) {
            successfulUniproxyPrepared = uniproxyPreparedWrapper.GetValue();
            wonderlog.SetMegamindRequestId(successfulUniproxyPrepared->GetMegamindRequestId());
        }

        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse;

        for (size_t i = 0; i < megamindValues.ValuesSize(); ++i) {
            if (PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                          megamindValues.GetValues(i))) {
                successfulMegamindRequestResponse = megamindValues.GetValues(i);
            }
        }

        if (successfulMegamindRequestResponse) {
            wonderlog.SetMegamindRequestId(successfulMegamindRequestResponse->GetRequestId());
            *wonderlog.MutableSpeechkitRequest() = successfulMegamindRequestResponse->GetSpeechKitRequest();
            *wonderlog.MutableSpeechkitResponse() = successfulMegamindRequestResponse->GetSpeechKitResponse();
            const auto hash = HashStringToUi64(wonderlog.GetUuid());
            Data[hash % Config.GetShardsCount()].push_back(wonderlog.SerializeAsString());
        }
    }
}

NYT::TFuture<TWonderlogsCreator::TPrepareForAsyncWriteResult> TWonderlogsCreator::PrepareForAsyncWrite() {
    auto data(std::move(Data));
    Data.clear();

    return NYT::MakeFuture<TPrepareForAsyncWriteResult>(
        {.AsyncWriter = [this, dataToWrite = std::move(data)](NYT::NApi::ITransactionPtr tx) {
            NBigRT::TYtQueue(Config.GetOutputQueue(), tx->GetClient())
                .Write(tx, dataToWrite, Config.GetCompressionCodec());
        }});
}

} // namespace NAlice::NWonderlogs
