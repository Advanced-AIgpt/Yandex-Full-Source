package ru.yandex.alice.kronstadt.core.rpc

import io.grpc.Status.Code
import io.grpc.StatusRuntimeException

class RpcException(msg: String, code: Code = Code.INTERNAL) :
    StatusRuntimeException(io.grpc.Status.fromCode(code).withDescription(msg)) {

    fun toErrorResponse(): ErrorRpcResponse = ErrorRpcResponse(this.status.description, this.status.code)
}
