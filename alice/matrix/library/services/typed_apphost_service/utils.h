#pragma once

#include <apphost/api/service/cpp/service_exceptions.h>

#include <library/cpp/threading/future/future.h>

namespace NMatrix {

template <typename TResponse>
NThreading::TFuture<TResponse> CreateTypedAppHostServiceIsSuspendedFastError() {
    return NThreading::MakeErrorFuture<TResponse>(
        std::make_exception_ptr(
            NAppHost::NService::TFastError() << "Service is suspended"
        )
    );
}

} // namespace NMatrix
