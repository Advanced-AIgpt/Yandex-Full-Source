#pragma once

#include <library/cpp/monlib/dynamic_counters/counters.h>

namespace NRTLogAgent {
    struct TRTLogAgentCounters;

    struct TMessageSenderCounters {
        explicit TMessageSenderCounters(TRTLogAgentCounters& owner);

        NMonitoring::TDeprecatedCounter& OutputMessagesCount;
        NMonitoring::TDeprecatedCounter& OutputBytesCount;
        NMonitoring::TDeprecatedCounter& InflightOutputMessagesCount;
        NMonitoring::TDeprecatedCounter& InflightOutputBytesCount;
        NMonitoring::TDeprecatedCounter& OutputContention;
    };

    struct TSharedMemoryQueueCounters {
        TSharedMemoryQueueCounters(TRTLogAgentCounters& owner);

        NMonitoring::TDeprecatedCounter& InflightItemsCount;
        NMonitoring::TDeprecatedCounter& ItemsInQueueCount;
        NMonitoring::TDeprecatedCounter& BlockedEnqueueTransactionsCount;
        NMonitoring::TDeprecatedCounter& BlockedDequeueTransactionsCount;
        NMonitoring::TDeprecatedCounter& PendingEnqueueTransactionsCount;
        NMonitoring::TDeprecatedCounter& PendingDequeueTransactionsCount;
        NMonitoring::TDeprecatedCounter& EnqueueWaitCount;
        NMonitoring::TDeprecatedCounter& DequeueWaitCount;
        NMonitoring::TDeprecatedCounter& AllocatedPages;
        NMonitoring::TDeprecatedCounter& AllocatedBytes;
        NMonitoring::TDeprecatedCounter& UsedPages;
        NMonitoring::TDeprecatedCounter& UsedBytes;
    };

    struct TRTLogAgentCounters: public NMonitoring::TDynamicCounters {
        TRTLogAgentCounters();

        NMonitoring::TDeprecatedCounter& WrittenItemsCount;
        NMonitoring::TDeprecatedCounter& WrittenBytesCount;
        NMonitoring::TDeprecatedCounter& MaxItemSizeViolationsCount;
        NMonitoring::TDeprecatedCounter& ParseFailuresCount;
        NMonitoring::TDeprecatedCounter& ReadErrorsCount;
        NMonitoring::TDeprecatedCounter& DataBytes;
        NMonitoring::TDeprecatedCounter& DataSegments;
        NMonitoring::TDeprecatedCounter& DataSeconds;
        TMessageSenderCounters MessageSenderCounters;
        TSharedMemoryQueueCounters QueueCounters;
    };
}
