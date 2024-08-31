#pragma once

namespace NGenerativeBoltalka {

template <typename TProtoRequest, typename TProtoResponse>
struct IRequestHandler {
    virtual TProtoResponse HandleRequest(const TProtoRequest& request) = 0;
    virtual ~IRequestHandler() = default;
};

}
