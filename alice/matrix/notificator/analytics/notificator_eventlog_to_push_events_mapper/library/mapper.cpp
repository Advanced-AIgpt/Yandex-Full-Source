#include "mapper.h"

#include <alice/matrix/library/logging/events/events.ev.pb.h>

#include <library/cpp/eventlog/evdecoder.h>
#include <library/cpp/eventlog/logparser.h>

namespace NMatrix::NNotificator::NAnalytics {

const THashMap<TEventClass, TEventProcessor<NYT::TNode>> TNotificatorLogToPushIdInfoMapper::EVENT_PROCESSORS = {
    {
        NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::ID, 
        ProcessEventWithPushId<NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult, NYT::TNode>,
    },
    {
        NEvClass::TMatrixNotificatorAnalyticsNewTechnicalPush::ID, 
        ProcessEventWithPushId<NEvClass::TMatrixNotificatorAnalyticsNewTechnicalPush, NYT::TNode>,
    },
    {
        NEvClass::TMatrixNotificatorAnalyticsTechnicalPushDeliveryAcknowledge::ID, 
        ProcessEventWithPushId<NEvClass::TMatrixNotificatorAnalyticsTechnicalPushDeliveryAcknowledge, NYT::TNode>,
    },
};

namespace {

const TString FRAME_COLUMN_NAME = "frame";

TVector<TConstEventPtr> ParseFrame(const TFrame& frame) {
    TVector<TConstEventPtr> result;

    try {
        TEventFilter filter(false);
        filter.AddEventClass(TEndOfFrameEvent::EventClass);

        TFrameDecoder decoder(
            frame,
            &filter,
            false,
            true
        );

        if (decoder.Avail()) {
            do {
                result.push_back(*decoder);
            } while (decoder.Next());
        }
    } catch (...) {
        // TODO(ZION-257) save rows we failed to process to a dedicated table
        Cerr << "Failed to parse events from frame: " << CurrentExceptionMessage() << Endl;
    }

    return result;
}

TFrame ReadFrame(TMemoryInput* frameInput) {
    auto frame = FindNextFrame(frameInput, NEvClass::Factory());
    Y_ENSURE(frame, "cant FindNextFrame");
    return std::move(*frame);
}

} // namespace

void TNotificatorLogToPushIdInfoMapper::Do(TReader* reader, TWriter* writer) {
    for (const auto& cursor : *reader) {
        auto row = cursor.GetRow();

        TString rawFrame = row.ChildAsString(FRAME_COLUMN_NAME);
        TMemoryInput frameInput(rawFrame.data(), rawFrame.size());

        auto frame = ReadFrame(&frameInput);
        auto events = ParseFrame(frame);

        for (const auto& event : events) {
            if (const auto* eventProcessor = EVENT_PROCESSORS.FindPtr(event->Class)) {
                (*eventProcessor)(event, writer);
            }
        }
    }
}

REGISTER_MAPPER(TNotificatorLogToPushIdInfoMapper);

} // namespace NMatrix::NNotificator::NAnalytics
