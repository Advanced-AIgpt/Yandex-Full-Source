#include "async_logger.h"

namespace NNlg {

TAsyncLogger::TAsyncLogger(size_t queueSize)
    : QueueSize(queueSize)
{
    TaskQueue.RunAdditionalThreads(1);
}

void TAsyncLogger::Write(const TString& message) {
    if (TaskQueue.GetQueueSize() >= QueueSize) {
        return;
    }
    TaskQueue.Exec([message](int){
        Cerr << message << Endl;
    }, 0, 0);
}

}

