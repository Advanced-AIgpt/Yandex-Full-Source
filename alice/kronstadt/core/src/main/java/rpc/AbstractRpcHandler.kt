package ru.yandex.alice.kronstadt.core.rpc

import com.google.protobuf.Message
import org.springframework.core.annotation.AnnotatedElementUtils
import org.springframework.util.ReflectionUtils
import ru.yandex.alice.kronstadt.core.AdditionalHandler
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import java.lang.reflect.Method

/**
 * Base class for rpc handler.
 *
 * Developer mey define new method of signature similar to [setup] method annotated with [AdditionalHandler]
 * Such method will be added to the handler's apphost graph after setup
 */
abstract class AbstractRpcHandler<RequestMessage : Message, ResponseMessage : Message>(
    /**
     * path segment of apphost handler.
     * Resulting handler paths will be formed as:
     * `/kronstadt/scenario/${scenario_name}/rpc/${path}/setup`
     * `/kronstadt/scenario/${scenario_name}/rpc/${path}/process`
     */
    override val path: String,
    requestProto: RequestMessage,
) : RpcHandler<RequestMessage, ResponseMessage> {

    @Suppress("UNCHECKED_CAST")
    override val requestProtoDefaultInstance: RequestMessage = requestProto.defaultInstanceForType as RequestMessage

    override val additionalHandlers: Map<String, ApphostRpcHandlerNode<RequestMessage>>

    init {
        val handlers = mutableMapOf<String, ApphostRpcHandlerNode<RequestMessage>>()

        val methodCallback = { method: Method ->
            val path =
                AnnotatedElementUtils.findMergedAnnotation(method, AdditionalHandler::class.java)!!.path.trim('/')
            if (path.uppercase() == "SETUP" || path.uppercase() == "PROCESS") {
                throw RuntimeException("Additional handler path can't be 'setup' or 'process'")
            }

            handlers[path] =
                { req: RpcRequest<RequestMessage>, builder: ScenePrepareBuilder -> method.invoke(this, req, builder) }
        }

        val methodFilter = { method: Method ->
            !AnnotatedElementUtils.findMergedAnnotation(method, AdditionalHandler::class.java)?.path.isNullOrEmpty() &&
                method.parameters.getOrNull(0)?.type?.let { RpcRequest::class.java.isAssignableFrom(it) } ?: false &&
                method.parameters.getOrNull(1)?.type?.let { ScenePrepareBuilder::class.java.isAssignableFrom(it) } ?: false
        }
        ReflectionUtils.doWithMethods(this::class.java, methodCallback, methodFilter)

        this.additionalHandlers = handlers.toMap()
    }
}
