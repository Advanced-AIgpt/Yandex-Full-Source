package ru.yandex.alice.kronstadt.core.directive

import com.fasterxml.jackson.annotation.JsonProperty
import com.google.protobuf.GeneratedMessageV3
import java.util.Base64

@Directive(value = "grpc_response", ignoreAnswer = true)
@Deprecated("Use pure Rpc handlers instead of grpc directive in ordinary Alice response")
class GrpcDirective(response: GeneratedMessageV3) : CallbackDirective {

    @JsonProperty("grpc_response")
    private val grpcResponse: String = Base64.getEncoder().encodeToString(response.toByteArray())
}
