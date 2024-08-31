#pragma once

#include <library/cpp/threading/local_executor/local_executor.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NNlg {

class TAsyncLogger {
public:
    TAsyncLogger(size_t queueSize);

    void Write(const TString& message);

private:
    const int QueueSize;
    NPar::TLocalExecutor TaskQueue;
};

}
