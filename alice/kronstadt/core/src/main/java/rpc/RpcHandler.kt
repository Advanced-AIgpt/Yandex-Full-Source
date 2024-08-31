package ru.yandex.alice.kronstadt.core.rpc

import com.google.protobuf.Message
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder

interface RpcHandler<RequestMessage : Message, ResponseMessage : Message> {

    val path: String

    val requestProtoDefaultInstance: RequestMessage

    val additionalHandlers: Map<String, ApphostRpcHandlerNode<RequestMessage>>

    fun setup(request: RpcRequest<RequestMessage>, responseBuilder: ScenePrepareBuilder)
    fun process(request: RpcRequest<RequestMessage>): RpcResponse<ResponseMessage>
}

typealias ApphostRpcHandlerNode<RequestMessage> = (RpcRequest<RequestMessage>, ScenePrepareBuilder) -> Unit
