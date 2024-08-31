#include "counters.h"

namespace NRTLogAgent {
    using namespace NMonitoring;

    TMessageSenderCounters::TMessageSenderCounters(TRTLogAgentCounters& owner)
        : OutputMessagesCount(*owner.GetCounter("MessageSender/OutputMessagesCount", true))
        , OutputBytesCount(*owner.GetCounter("MessageSender/OutputBytesCount", true))
        , InflightOutputMessagesCount(*owner.GetCounter("MessageSender/InflightOutputMessagesCount", false))
        , InflightOutputBytesCount(*owner.GetCounter("MessageSender/InflightOutputBytesCount", false))
        , OutputContention(*owner.GetCounter("MessageSender/OutputContention", true))
    {
    }

    TSharedMemoryQueueCounters::TSharedMemoryQueueCounters(TRTLogAgentCounters& owner)
        : InflightItemsCount(*owner.GetCounter("Queue/InflightItemsCount", false))
        , ItemsInQueueCount(*owner.GetCounter("Queue/ItemsInQueueCount", false))
        , BlockedEnqueueTransactionsCount(*owner.GetCounter("Queue/BlockedEnqueueTransactionsCount", false))
        , BlockedDequeueTransactionsCount(*owner.GetCounter("Queue/BlockedDequeueTransactionsCount", false))
        , PendingEnqueueTransactionsCount(*owner.GetCounter("Queue/PendingEnqueueTransactionsCount", false))
        , PendingDequeueTransactionsCount(*owner.GetCounter("Queue/PendingDequeueTransactionsCount", false))
        , EnqueueWaitCount(*owner.GetCounter("Queue/EnqueueWaitCount", true))
        , DequeueWaitCount(*owner.GetCounter("Queue/DequeueWaitCount", true))
        , AllocatedPages(*owner.GetCounter("Queue/AllocatedPages", false))
        , AllocatedBytes(*owner.GetCounter("Queue/AllocatedBytes", false))
        , UsedPages(*owner.GetCounter("Queue/UsedPages", false))
        , UsedBytes(*owner.GetCounter("Queue/UsedBytes", false))
    {
    }

    TRTLogAgentCounters::TRTLogAgentCounters()
        : WrittenItemsCount(*GetCounter("WrittenItemsCount", true))
        , WrittenBytesCount(*GetCounter("WrittenBytesCount", true))
        , MaxItemSizeViolationsCount(*GetCounter("MaxItemSizeViolationsCount", true))
        , ParseFailuresCount(*GetCounter("ParseFailuresCount", true))
        , ReadErrorsCount(*GetCounter("ReadErrors", true))
        , DataBytes(*GetCounter("DataBytes", false))
        , DataSegments(*GetCounter("DataSegments", false))
        , DataSeconds(*GetCounter("DataSeconds", false))
        , MessageSenderCounters(*this)
        , QueueCounters(*this)
    {
    }
}
