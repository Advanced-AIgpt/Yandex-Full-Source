package ru.yandex.alice.kronstadt.server.apphost

import NAliceProtocol.ContextLoad.TContextLoadResponse
import NGProxy.Metadata.TMetadata
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import com.google.protobuf.Message
import com.google.protobuf.util.JsonFormat
import com.google.rpc.Status
import io.grpc.Status.Code
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.AdditionalSources
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import ru.yandex.alice.kronstadt.core.convert.response.AnalyticsInfoConverter
import ru.yandex.alice.kronstadt.core.convert.response.ServerDirectiveConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.rpc.BaseRpcResponse
import ru.yandex.alice.kronstadt.core.rpc.ErrorRpcResponse
import ru.yandex.alice.kronstadt.core.rpc.RpcException
import ru.yandex.alice.kronstadt.core.rpc.RpcHandler
import ru.yandex.alice.kronstadt.core.rpc.RpcRequest
import ru.yandex.alice.kronstadt.core.rpc.RpcResponse
import ru.yandex.alice.kronstadt.core.scenePrepareBuilder
import ru.yandex.alice.library.client.protos.ClientInfoProto.TClientInfoProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TMementoData
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRpcResponse
import ru.yandex.alice.megamind.protos.scenarios.ScenarioRequestMeta.TRequestMeta
import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter
import ru.yandex.alice.protos.api.rpc.StatusProto
import ru.yandex.web.apphost.api.AppHostPathHandler
import ru.yandex.web.apphost.api.DelegateApphostPathHandler
import ru.yandex.web.apphost.api.request.ApphostRequest
import ru.yandex.web.apphost.api.request.RequestItem
import java.time.Instant
import ru.yandex.web.apphost.api.request.RequestContext as ApphostRequestContext

typealias ApphostGrpcHandlerNode<RequestMessage> = (RpcRequest<RequestMessage>, ScenePrepareBuilder) -> Unit

class RpcHandlerAdapter<RequestMessage : Message, ResponseMessage : Message>(
    private val scenarioMeta: ScenarioMeta,
    private val handler: RpcHandler<RequestMessage, ResponseMessage>,
    private val httpConverter: HttpRequestConverter,
    private val requestContext: RequestContext,
    private val contextPopulator: ContextPopulator,
    private val serverDirectiveConverter: ServerDirectiveConverter,
    private val objectMapper: ObjectMapper,
) {

    private val requestDefaultProto = handler.requestProtoDefaultInstance
    private val requestClass: Class<RequestMessage> = requestDefaultProto.javaClass
    private val analyticsInfoConverter = AnalyticsInfoConverter(scenarioMeta)

    internal fun getHandlerAdapters(): List<AppHostPathHandler> {

        val rootPaths = listOf(
            "/kronstadt/scenario/${scenarioMeta.mmPath ?: scenarioMeta.name}/grpc/${handler.path.trimEnd('/')}",
            "/kronstadt/scenario/${scenarioMeta.mmPath ?: scenarioMeta.name}/rpc/${handler.path.trimEnd('/')}"
        )

        val additionalHandlers = handler.additionalHandlers.flatMap { (path, h) ->
            rootPaths.map { rootPath ->
                DelegateApphostPathHandler("${rootPath}/$path") {
                    additionalHandler(it, path, h)
                }
            }
        }

        return rootPaths.flatMap { rootPath ->
            listOf(
                DelegateApphostPathHandler("${rootPath}/setup") { setup(it) },
                DelegateApphostPathHandler("${rootPath}/process") {
                    val response = process(it)

                    if (it.getSingleRequestItemO("rpc_request").isPresent) {
                        val rpcResponse =
                            when (response) {
                                is RpcResponse<*> -> {
                                    TScenarioRpcResponse.newBuilder().apply {
                                        responseBody = com.google.protobuf.Any.pack(response.payload)
                                        response.analyticsInfo?.let { ai ->
                                            analyticsInfo = analyticsInfoConverter.convert(ai)
                                        }
                                        addAllServerDirectives(response.serverDirectives.map {
                                            serverDirectiveConverter.convert(it, ToProtoContext())
                                        })
                                    }.build()
                                }

                                is ErrorRpcResponse -> {
                                    TScenarioRpcResponse.newBuilder().apply {
                                        error = StatusProto.TStatus.newBuilder().apply {
                                            code = response.code.value()
                                            message = response.message
                                        }.build()
                                    }.build()
                                }
                            }
                        it.addProtobufItem("rpc_response", rpcResponse)
                    } else {
                        when (response) {
                            is ErrorRpcResponse -> addError(it, response)
                            is RpcResponse<*> -> it.addProtobufItem(
                                "response",
                                com.google.protobuf.Any.pack(response.payload)
                            )
                        }
                    }

                }
            )
        } + additionalHandlers
    }

    private fun additionalHandler(
        apphostRequestContext: ApphostRequestContext,
        path: String,
        h: ApphostGrpcHandlerNode<RequestMessage>
    ) {
        try {
            val rpcRequest: RpcRequest<RequestMessage>
            try {
                rpcRequest = createRequest(apphostRequestContext)
            } catch (e: Exception) {
                logger.error("Scenario request parsing failed", e)
                addError(apphostRequestContext, ErrorRpcResponse.fromThrowable(e))
                return
            }

            h.invoke(rpcRequest, scenePrepareBuilder(apphostRequestContext, httpConverter))
        } catch (e: Exception) {
            addError(apphostRequestContext, ErrorRpcResponse.fromThrowable(e, "RPC request '${path}' stage failed"))
        }
    }

    private fun createRequest(apphostRequestContext: ru.yandex.web.apphost.api.request.RequestContext): RpcRequest<RequestMessage> =
        if (apphostRequestContext.getSingleRequestItemO("rpc_request").isPresent) {
            createMmRpcRequest(apphostRequestContext)
        } else {
            createFromGrpcRequest(apphostRequestContext)
        }

    private fun setup(apphostRequestContext: ApphostRequestContext) {
        try {
            val request: RpcRequest<RequestMessage>
            try {
                request = createRequest(apphostRequestContext)
            } catch (e: Exception) {
                logger.error("Scenario request parsing failed", e)
                addError(apphostRequestContext, ErrorRpcResponse.fromThrowable(e))
                return
            }

            handler.setup(request = request, scenePrepareBuilder(apphostRequestContext, httpConverter))
        } catch (e: Exception) {
            addError(apphostRequestContext, ErrorRpcResponse.fromThrowable(e, "RPC request 'setup' stage failed"))
        }
    }

    private fun addError(apphostRequestContext: ApphostRequestContext, e: ErrorRpcResponse) {
        val status = Status.newBuilder().apply {
            code = e.code.value()
            message = e.message
            //addDetails
        }.build()
        apphostRequestContext.addProtobufItem("error", status)
    }

    private fun process(apphostRequestContext: ApphostRequestContext): BaseRpcResponse {

        val rpcRequest: RpcRequest<RequestMessage>
        try {
            rpcRequest = createRequest(apphostRequestContext)
        } catch (e: Exception) {
            logger.error("Scenario request parsing failed", e)
            return ErrorRpcResponse.fromThrowable(e, "Failed to parse scenario rpc request")
        }

        // TODO: handle exceptions
        return try {
            handler.process(request = rpcRequest)
        } catch (e: Exception) {
            ErrorRpcResponse.fromThrowable(e, "RPC request 'process' stage failed")
        }
    }

    private fun createMmRpcRequest(context: ApphostRequestContext): RpcRequest<RequestMessage> {
        val request = context.getSingleRequestItem("rpc_request")
            .getProtobufData(RequestProto.TScenarioRpcRequest.getDefaultInstance())

        if (!request.request.`is`(requestClass)) {
            throw RpcException(
                "Unexpected request type. Expected ${requestClass.name} but got ${request.request.typeUrl}",
                Code.INVALID_ARGUMENT
            )
        }

        val requestMeta = context.getSingleRequestItem("mm_scenario_request_meta")
            .getProtobufData(TRequestMeta.getDefaultInstance())

        val clientInfoProto = request.baseRequest.clientInfo

        val clientInfo = ClientInfo(
            appId = clientInfoProto.appId,
            appVersion = clientInfoProto.appVersion,
            platform = clientInfoProto.platform,
            osVersion = clientInfoProto.osVersion,
            uuid = clientInfoProto.uuid,
            deviceId = clientInfoProto.deviceId,
            lang = clientInfoProto.lang,
            timezone = clientInfoProto.timezone,
            deviceModel = clientInfoProto.deviceModel,
            deviceManufacturer = clientInfoProto.deviceManufacturer,
        )

        val rpcRequest: RpcRequest<RequestMessage> = RpcRequest(
            requestId = requestMeta.requestId,
            requestBody = request.request.unpack(requestClass),
            scenarioMeta = scenarioMeta,
            clientInfo = clientInfo,
            userId = requestContext.currentUserId,
            serverTime = Instant.ofEpochMilli(request.baseRequest.serverTimeMs),
            randomSeed = request.baseRequest.randomSeed,
            experiments = request.baseRequest.experiments.fieldsMap.keys,
            mementoData = TMementoData.getDefaultInstance(),
            additionalSources = additionalSources(context)
        )

        contextPopulator.populateRequestAndLoggingContext(requestMeta, userAgent = null)

        return rpcRequest
    }

    private fun createFromGrpcRequest(context: ApphostRequestContext): RpcRequest<RequestMessage> {

        val requestWrapper = context.getSingleRequestItem("request")
            .getProtobufData(com.google.protobuf.Any.getDefaultInstance())
        if (!requestWrapper.`is`(requestClass)) {
            throw RpcException(
                "Unexpected request type. Expected ${requestClass.name} but got ${requestWrapper.typeUrl}",
                Code.INVALID_ARGUMENT
            )
        }
        val request = requestWrapper.unpack(requestClass)

        val requestMeta = context.getSingleRequestItem("mm_scenario_request_meta")
            .getProtobufData(TRequestMeta.getDefaultInstance())
        val contextLoad = context.getSingleRequestItem("context_load_response")
            .getProtobufData(TContextLoadResponse.getDefaultInstance())
        val metadata = context.getSingleRequestItem("metadata")
            .getProtobufData(TMetadata.getDefaultInstance())
        val memento: TMementoData = context.getSingleRequestItemO("memento_data")
            .map { it.getProtobufData(TMementoData.getDefaultInstance()) }
            .orElse(TMementoData.getDefaultInstance())

        val clientInfoProto = TClientInfoProto.newBuilder().also {
            JsonFormat.parser().merge(metadata.application, it)
        }.build()

        val clientInfo = ClientInfo(
            appId = clientInfoProto.appId,
            appVersion = clientInfoProto.appVersion,
            platform = clientInfoProto.platform,
            osVersion = clientInfoProto.osVersion,
            uuid = clientInfoProto.uuid,
            deviceId = clientInfoProto.deviceId,
            lang = clientInfoProto.lang,
            timezone = clientInfoProto.timezone,
            deviceModel = clientInfoProto.deviceModel,
            deviceManufacturer = clientInfoProto.deviceManufacturer,
        )

        val rpcRequest: RpcRequest<RequestMessage> = RpcRequest(
            requestId = requestMeta.requestId,
            requestBody = request,
            scenarioMeta = scenarioMeta,
            clientInfo = clientInfo,
            userId = requestContext.currentUserId,
            serverTime = /*Instant.ofEpochMilli(metadata.serverTimeMs)*/Instant.now(),
            randomSeed = requestMeta.randomSeed,
            experiments = if (metadata.hasExperiments())
                contextLoad.flagsInfo.voiceFlags.storageMap.keys + objectMapper.readValue<Set<String>>(metadata.experiments)
            else
                contextLoad.flagsInfo.voiceFlags.storageMap.keys,
            mementoData = memento,
            additionalSources = additionalSources(context)
            //experiments = metadata.
        )

        contextPopulator.populateRequestAndLoggingContext(requestMeta, userAgent = null)

        return rpcRequest
    }

    private fun additionalSources(apphostRequest: ApphostRequest): AdditionalSources {
        val items: Map<String, List<(Message) -> Message>> =
            apphostRequest.requestItems
                .filter { !KNOWN_TYPES.contains(it.type) }
                .groupBy(RequestItem::getType) {
                    return@groupBy { defaultInstance ->
                        it.getProtobufData(defaultInstance)
                    }
                }
        return AdditionalSources(items, httpConverter)
    }

    companion object {
        private val KNOWN_TYPES: Set<String> = hashSetOf(
            "mm_scenario_request_meta",
            "rpc_request",
            "context_load_response",
            "metadata",
            "request"
        )

        private val logger = LogManager.getLogger(RpcHandlerAdapter::class.java)
    }
}
