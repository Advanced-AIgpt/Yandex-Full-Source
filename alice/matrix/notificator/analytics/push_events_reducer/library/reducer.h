#pragma once

#include <mapreduce/yt/interface/common.h>
#include <mapreduce/yt/interface/operation.h>

namespace NMatrix::NNotificator::NAnalytics {

class TPushEventsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override;
};

} // namespace NMatrix::NNotificator::NAnalytics
