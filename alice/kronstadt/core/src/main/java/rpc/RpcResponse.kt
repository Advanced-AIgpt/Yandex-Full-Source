package ru.yandex.alice.kronstadt.core.rpc

import com.google.protobuf.Message
import io.grpc.Status
import io.grpc.StatusException
import io.grpc.StatusRuntimeException
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective

sealed interface BaseRpcResponse

data class RpcResponse<T : Message>(
    val payload: T,
    val analyticsInfo: AnalyticsInfo? = null,
    val serverDirectives: List<ServerDirective> = listOf(),
) : BaseRpcResponse

data class ErrorRpcResponse(val message: String?, val code: Status.Code = Status.Code.INTERNAL) : BaseRpcResponse {
    companion object {
        fun fromThrowable(e: Throwable, defaultMsg: String = "unknown exception"): ErrorRpcResponse {
            return when (e) {
                is StatusException -> ErrorRpcResponse(e.status.description, e.status.code)
                is StatusRuntimeException -> ErrorRpcResponse(e.status.description, e.status.code)
                else -> ErrorRpcResponse(e.message ?: defaultMsg, Status.Code.INTERNAL)
            }
        }
    }
}
