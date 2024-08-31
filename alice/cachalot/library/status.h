#pragma once

#include <alice/cachalot/api/protos/cachalot.pb.h>

#include <library/cpp/neh/rpc.h>
#include <library/cpp/threading/future/future.h>


namespace NCachalot {


using EResponseStatus = NCachalotProtocol::EResponseStatus;


struct TStatus {
    TStatus() { }

    TStatus(EResponseStatus s) : Status(s) { }

    TStatus(const TStatus& other) = default;

    inline operator bool() const {
        return Status == EResponseStatus::OK
            || Status == EResponseStatus::CREATED
            || Status == EResponseStatus::NO_CONTENT;
    }

    TStatus& SetStatus(EResponseStatus s) {
        Status = s;
        return *this;
    }

    operator NNeh::IRequest::TResponseError() const;

    EResponseStatus Status { EResponseStatus::INTERNAL_ERROR };
};


using TStatusPromise = NThreading::TPromise<TStatus>;

using TAsyncStatus = NThreading::TFuture<TStatus>;

}   // namespace NCachalot
