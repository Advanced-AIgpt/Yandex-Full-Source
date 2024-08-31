package ru.yandex.alice.kronstadt.server.apphost.warmup

import com.google.common.base.Stopwatch
import com.google.protobuf.Empty
import com.google.protobuf.Struct
import com.google.protobuf.util.JsonFormat
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.ApplicationListener
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSceneArguments
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSelectedSceneForApply
import ru.yandex.alice.kronstadt.server.apphost.middleware.ApphostStartupWarmupInterceptor
import ru.yandex.alice.kronstadt.server.apphost.warmup.scenario.DUMMY_SCENARIO_META
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioApplyRequest
import ru.yandex.alice.megamind.protos.scenarios.ScenarioRequestMeta
import ru.yandex.alice.paskills.common.apphost.spring.ApphostServiceInitializedEvent
import ru.yandex.alice.protos.endpoint.CapabilityProto
import ru.yandex.kronstadt.alice.scenarios.video_call.proto.TVideoCallScenarioData
import ru.yandex.web.apphost.api.request.ApphostRequestBuilder
import ru.yandex.web.apphost.grpc.Client
import java.util.concurrent.TimeUnit

@Component
class ApphostWarmupApplicationListener(
    private val warmupGuard: ApphostStartupWarmupInterceptor
) : ApplicationListener<ApphostServiceInitializedEvent> {

    private val logger = LogManager.getLogger()

    @Value("\${warmup.requests:2}")
    private val warmupRequests: Int = 0
    private val resolver = PathMatchingResourcePatternResolver()

    private val warmupCallRequests = listOf(
        WarmUpRequest(
            request = getProtoFromFile("dummy/request"),
            capability = getProtoFromFile("dummy/capability"),
            selectedScene = getProtoFromFile("dummy/selected_scene"),
        )
    )

    override fun onApplicationEvent(event: ApphostServiceInitializedEvent) {
        if (warmupRequests == 0) {
            logger.info("Skipping warmup requests")
            warmupGuard.ready.set(true)
            return
        }
        val grpcClient = Client("localhost", event.source.port)

        val swTotal = Stopwatch.createStarted()
        grpcClient.use { apphostClient ->
            logger.info("Apphost on start warmup requests started")

            val swRunTotal = Stopwatch.createStarted()

            for (i in 1..warmupRequests) {
                for (requestData in warmupCallRequests) {
                    with(requestData) {
                        apphostClient.warmUpDoRequest("run/pre_select") {
                            composeCallRequest(request, capability)
                        }
                        apphostClient.warmUpDoRequest("run/select_scene") {
                            composeCallRequest(request, capability)
                        }
                        apphostClient.warmUpDoRequest("run/common") {
                            composeCallRequest(request, capability, selectedScene)
                        }
                        apphostClient.warmUpDoRequest("apply/common") {
                            composeCallRequest(request, capability, selectedScene, true)
                        }
                    }
                }
            }
            logger.info("Apphost warmup run requests done in ${swRunTotal.elapsed(TimeUnit.MILLISECONDS)}ms")
        }

        warmupGuard.ready.set(true)

        logger.info("All warmup requests done in ${swTotal.elapsed(TimeUnit.MILLISECONDS)}ms")
    }

    private fun Client.warmUpDoRequest(node: String, body: ApphostRequestBuilder.() -> Unit) {
        var sw: Stopwatch? = null
        try {

            call("/kronstadt/scenario/${DUMMY_SCENARIO_META.name}/$node") {
                body()
                logger.info("DummyScenario warmup \"$node\" request started")
                sw = Stopwatch.createStarted()
            }
            logger.info("DummyScenario warmup \"$node\" request done in ${sw?.elapsed(TimeUnit.MILLISECONDS)}ms")
        } catch (e: Exception) {
            logger.error("DummyScenario \"$node\" warmup request failed", e)
        }
    }

    private fun ApphostRequestBuilder.composeCallRequest(
        request: String,
        capability: String,
        selectedScene: String? = null,
        apply: Boolean = false
    ) {
        addRequestMeta(getProtoFromFile("request_meta"))
        addDataSource("BLACK_BOX", getProtoFromFile("user_info"))
        addDataSource("CONTACTS_LIST", getProtoFromFile("contacts"))

        if (!apply) {
            addRunRequest(request)
        } else {
            addApplyRequest(request)
        }

        addDataSource("ENVIRONMENT_STATE", capability)
        selectedScene?.apply { addSelectedScene(selectedScene) }

        addFlag(ApphostStartupWarmupInterceptor.WARMUP_FLAG)
    }

    private fun getProtoFromFile(filename: String): String {
        val path = "warmup/${filename}.json"
        val resource = resolver.getResource(path)
        if (!resource.exists()) {
            throw RuntimeException("Resource at path '$path' does not exists")
        }
        return String(resource.inputStream.readBytes())
    }

    private data class WarmUpRequest(
        val request: String,
        val capability: String,
        val selectedScene: String,
    )

    private val parser = JsonFormat.parser().usingTypeRegistry(
        JsonFormat.TypeRegistry.newBuilder()
            .add(TSceneArguments.getDescriptor())
            .add(TVideoCallScenarioData.getDescriptor())
            .add(CapabilityProto.TVideoCallCapability.getDescriptor())
            .build()
    )

    private fun ApphostRequestBuilder.addRunRequest(requestJsonString: String) = addProtobufItem(
        "mm_scenario_request",
        RequestProto.TScenarioRunRequest.newBuilder().apply {
            parser.merge(requestJsonString, this)
        }.build()
    )

    private fun ApphostRequestBuilder.addApplyRequest(
        requestJsonString: String
    ): ApphostRequestBuilder {
        val runRequest = RequestProto.TScenarioRunRequest.newBuilder().apply {
            parser.merge(requestJsonString, this)
        }.build()

        return addProtobufItem(
            "mm_scenario_request",
            TScenarioApplyRequest.newBuilder()
                .setBaseRequest(runRequest.baseRequest)
                .setInput(runRequest.input)
                .setArguments(
                    com.google.protobuf.Any.pack(
                        TSelectedSceneForApply.newBuilder()
                            .setSelectedScene(
                                TSceneArguments.newBuilder()
                                    .setSceneName("dummy_scene")
                                    .setArgs(Struct.getDefaultInstance())
                                    .build()
                            )
                            .setApplyArguments(com.google.protobuf.Any.pack(Empty.getDefaultInstance()))
                            .build()

                    )
                )
                .build()
        )
    }

    private fun ApphostRequestBuilder.addSelectedScene(selectedSceneJsonString: String) = addProtobufItem(
        "kronstadt_selected_scene_run",
        ApplyArgsProto.TSelectedScene.newBuilder().apply {
            parser.merge(selectedSceneJsonString, this)
        }.build()
    )

    private fun ApphostRequestBuilder.addRequestMeta(requestMetaJsonString: String) = addProtobufItem(
        "mm_scenario_request_meta",
        ScenarioRequestMeta.TRequestMeta.newBuilder().apply {
            parser.merge(requestMetaJsonString, this)
        }.build()
    )

    private fun ApphostRequestBuilder.addDataSource(name: String, dataSourceJsonString: String) = addProtobufItem(
        "datasource_$name",
        RequestProto.TDataSource.newBuilder().apply {
            parser.merge(dataSourceJsonString, this)
        }.build()
    )
}
