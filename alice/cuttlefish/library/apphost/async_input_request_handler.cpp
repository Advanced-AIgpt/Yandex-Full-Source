#include "async_input_request_handler.h"

using namespace NAlice::NCuttlefish;

bool TInputAppHostAsyncRequestHandler::TryBeginProcessing() {
    const int wasState = AtomicGetAndCas(&State_, Processing, Idle);
    Y_ASSERT(wasState == Idle || wasState == Finished);
    return wasState == Idle;
}

void TInputAppHostAsyncRequestHandler::EndProcessing() {
    const int wasState = AtomicGetAndCas(&State_, Idle, Processing);
    if (wasState == Processing)
        return;

    Y_ASSERT(wasState == NeedFinish);
    AtomicSet(State_, Finished);

    std::exception_ptr e;
    with_lock (ExceptionLock_) {
        e = Exception_;
    }

    if (e) {
        Promise_.TrySetException(e);
    } else {
        Promise_.TrySetValue();
    }
}

void TInputAppHostAsyncRequestHandler::Finish() {
    const int wasState = AtomicGetAndCas(&State_, Finished, Idle);
    if (wasState == Idle) {
        Y_ASSERT(!Exception_);
        Promise_.TrySetValue();
        return;
    }

    if (wasState == Processing) {
        AtomicCas(&State_, NeedFinish, Processing);
    }
}

void TInputAppHostAsyncRequestHandler::SetException(std::exception_ptr e) {
    const int wasState = AtomicGetAndCas(&State_, Finished, Idle);
    if (wasState == Idle) {
        Promise_.TrySetException(e);
        return;
    }

    if (wasState == Finished) {
        return;
    }

    with_lock (ExceptionLock_) {
        Exception_ = e;
    }
    if (wasState == Processing) {
        AtomicCas(&State_, NeedFinish, Processing);
    }
}
