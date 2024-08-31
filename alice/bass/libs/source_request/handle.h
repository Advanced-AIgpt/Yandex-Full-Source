#pragma once

#include <alice/bass/util/error.h>

namespace NBASS {

template <class TResponse>
class IRequestHandle {
public:
    virtual ~IRequestHandle() = default;
    virtual TResultValue WaitAndParseResponse(TResponse* response) = 0;
};

template <class TResponse>
class TDummyRequestHandle : public IRequestHandle<TResponse> {
public:
    TResultValue WaitAndParseResponse(TResponse* /*response*/) override {
        return TResultValue{};
    }
};

} // namespace NBASS
