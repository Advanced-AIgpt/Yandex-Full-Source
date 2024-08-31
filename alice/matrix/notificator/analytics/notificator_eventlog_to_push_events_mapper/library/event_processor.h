#pragma once

#include "table_helper.h"

#include <alice/matrix/notificator/analytics/common/column_names.h>
#include <alice/matrix/notificator/analytics/common/yt_helpers.h>

#include <library/cpp/eventlog/evdecoder.h>

#include <functional>

namespace NMatrix::NNotificator::NAnalytics {

namespace NPrivate {

template <typename TProto>
const TProto& Cast(const TEvent* e) {
    return *VerifyDynamicCast<const TProto*>(e->GetProto());
}

} // namespace NPrivate

template <typename TNode>
using TEventProcessor = std::function<void(TIntrusiveConstPtr<TEvent> event, NYT::TTableWriter<TNode>* writer)>;

template <typename TFilterEvent, typename TNode>
void ProcessEventWithPushId(TIntrusiveConstPtr<TEvent> event, NYT::TTableWriter<TNode>* writer) {
    auto downcastedEvent = NPrivate::Cast<TFilterEvent>(event.Get());
    
    auto pushId = downcastedEvent.GetPushId();
    downcastedEvent.ClearPushId();

    writer->AddRow(
        GetMapperResultRow(
            pushId,
            EventTimestampToYtTimestamp(event->Timestamp),
            event->Class,
            downcastedEvent.SerializeAsString()
        )
    );
}

} // namespace NMatrix::NNotificator::NAnalytics
