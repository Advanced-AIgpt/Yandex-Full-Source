#pragma once

#include "event_processor.h"

#include <mapreduce/yt/interface/common.h>
#include <mapreduce/yt/interface/operation.h>

namespace NMatrix::NNotificator::NAnalytics {

class TNotificatorLogToPushIdInfoMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override;

private:
    static const THashMap<TEventClass, TEventProcessor<NYT::TNode>> EVENT_PROCESSORS;
};

} // namespace NMatrix::NNotificator::NAnalytics
