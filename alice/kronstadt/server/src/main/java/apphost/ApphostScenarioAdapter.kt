package ru.yandex.alice.kronstadt.server.apphost

import com.google.protobuf.Message
import org.apache.logging.log4j.LogManager
import org.springframework.util.ReflectionUtils
import ru.yandex.alice.kronstadt.core.AdditionalSources
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.ScenarioCompositeResponse
import ru.yandex.alice.kronstadt.core.VersionProvider
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.scenario.ApplyingSceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.CommittingSceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.ContinuingSceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.IScenario
import ru.yandex.alice.kronstadt.core.scenario.Scene
import ru.yandex.alice.kronstadt.core.scenario.SceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.kronstadt.core.scenePrepareBuilder
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSelectedScene
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSelectedScene.SelectedCase
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSelectedScene.TErrorScene
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSelectedScene.TIrrelevantScene
import ru.yandex.alice.kronstadt.server.SELECTED_SCENE_SETRACE_MARKER
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TDataSource
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioApplyRequest
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioRunRequest
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioApplyResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioCommitResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioContinueResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse
import ru.yandex.alice.megamind.protos.scenarios.ScenarioRequestMeta.TRequestMeta
import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter
import ru.yandex.alice.paskills.common.apphost.spring.ApphostKey
import ru.yandex.alice.paskills.common.apphost.spring.HandlerAdapter
import ru.yandex.alice.paskills.common.apphost.spring.HandlerScanner
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace.SETRACE_TAG_MARKER_PARENT
import ru.yandex.alice.protos.api.renderer.Api
import ru.yandex.web.apphost.api.AppHostPathHandler
import ru.yandex.web.apphost.api.request.ApphostRequest
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder
import ru.yandex.web.apphost.api.request.RequestItem
import java.lang.reflect.Method
import ru.yandex.web.apphost.api.request.RequestContext as ApphostRequestContext

class ApphostScenarioAdapter<State>(
    private val scenario: IScenario<State>,
    private val versionProvider: VersionProvider,
    private val protoUtil: ProtoUtil,
    private val requestContext: RequestContext,
    private val contextPopulator: ContextPopulator,
    private val scanner: HandlerScanner,
    private val httpConverter: HttpRequestConverter,
) {

    @Retention(AnnotationRetention.RUNTIME)
    @Target(AnnotationTarget.FUNCTION)
    private annotation class ScenarioHandler(vararg val paths: String)

    internal fun getHandlerAdapters(): List<AppHostPathHandler> {

        val rootPath = "/kronstadt/scenario/${scenario.scenarioMeta.mmPath ?: scenario.scenarioMeta.name}/"

        val methods = mutableListOf<Method>()
        ReflectionUtils.doWithMethods(javaClass, methods::add) { method ->
            method.isAnnotationPresent(ScenarioHandler::class.java)
        }

        return methods.flatMap { method ->
            val methodSpec = scanner.parseMethodSpecification(method)
            val methodPaths = method.getAnnotation(ScenarioHandler::class.java).paths

            return@flatMap methodPaths.map { subPath ->
                HandlerAdapter(this, methodSpec.handler(rootPath + subPath))
            }
        }
    }

    @ScenarioHandler("run/pre_select")
    fun preSelectScene(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioRunRequest,
        apphostRequestContext: ApphostRequestContext,
    ) {
        //noop
        try {

            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val request = scenario.convertMegaMindMRequest(
                userId = requestContext.currentUserId,
                baseRequest = requestProto.baseRequest,
                input = requestProto.input,
                dataSourcesMap = requestProto.dataSourcesMap + getDataSourcesMap(apphostRequestContext),
            )

            logger.debug(
                "Scenario {} [run:pre_select] phase started with megamind_request=\n{}",
                scenario.name, request
            )

            scenario.doPrepareSelectScene(request, scenePrepareBuilder(apphostRequestContext, httpConverter))
        } catch (e: Exception) {
            logger.error("Scenario ${scenario.name} [run:pre_select] phase failed", e)
        }
    }

    @ScenarioHandler("run/select_scene")
    fun selectScene(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioRunRequest,
        apphostContext: ApphostRequestContext,
    ) {
        val selectedSceneItem: TSelectedScene = try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val request = scenario.convertMegaMindMRequest(
                userId = requestContext.currentUserId,
                baseRequest = requestProto.baseRequest,
                input = requestProto.input,
                dataSourcesMap = requestProto.dataSourcesMap + getDataSourcesMap(apphostContext),
                additionalSources = additionalSources(apphostContext),
            )

            logger.debug(
                "Scenario {} [run:select_scene] phase started with megamind_request=\n{}",
                scenario.name,
                request
            )

            val selectedScene = scenario.doSelectScene(request)
            logger.info(
                SELECTED_SCENE_SETRACE_MARKER,
                "Scenario {} [run:select_scene] phase resulted with selected_scene={}",
                scenario.name,
                selectedScene?.name
            )
            val selectedSceneItem = if (selectedScene != null) {
                if (selectedScene.scene is SceneWithPrepare<State, *>) {
                    selectedScene.prepareRun(request, scenePrepareBuilder(apphostContext, httpConverter))
                    // flags are used to determine apphost graph path
                    addSelectedSceneFlag(apphostContext, selectedScene.scene)
                }

                TSelectedScene.newBuilder()
                    .setSelectedScene(scenario.packSceneArguments(selectedScene))
                    .build()
            } else {
                TSelectedScene.newBuilder()
                    .setIrrelevantScene(TIrrelevantScene.newBuilder().setIsIrrelevant(true))
                    .build()
            }

            selectedSceneItem
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} select_scene phase failed", ex)
            TSelectedScene.newBuilder()
                .setErrorScene(
                    TErrorScene.newBuilder()
                        .setMessage(ex.message)
                        .setType(ex.javaClass.name)
                ).build()
        }
        apphostContext.addProtobufItem("kronstadt_selected_scene_run", selectedSceneItem)
    }

    @ScenarioHandler("run/common")
    fun renderRun(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioRunRequest,
        @ApphostKey("kronstadt_selected_scene_run") selectedSceneArg: TSelectedScene,
        apphostRequestContext: ApphostRequestContext,
    ) {
        val (scenarioResponse, renderData) = try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val request = scenario.convertMegaMindMRequest(
                userId = requestContext.currentUserId,
                baseRequest = requestProto.baseRequest,
                input = requestProto.input,
                dataSourcesMap = requestProto.dataSourcesMap + getDataSourcesMap(apphostRequestContext),
                additionalSources = additionalSources(apphostRequestContext),
            )

            logger.debug(
                "Scenario {} [run:common_render] phase started with selectedSceneArg=\n{}\n AND megamind_request=\n {}",
                scenario.name,
                selectedSceneArg,
                request
            )

            when (selectedSceneArg.selectedCase) {
                SelectedCase.SELECTEDSCENE -> {
                    val scene: SelectedScene.Running<State, *> =
                        scenario.chooseRunningScene(selectedSceneArg.selectedScene)
                    scenario.renderRunResponse(request, scene).also {
                        logger.info(
                            "Scenario ${scenario.name} [run:common_render] phase " +
                                "resulted with scene=[${selectedSceneArg.selectedCase}:${selectedSceneArg.selectedScene.sceneName}]"
                        )
                        logger.debug("MM_RESPONSE=\n{}", it)
                    }
                }
                SelectedCase.IRRELEVANTSCENE -> {
                    scenario.renderRunResponse(request, null).also {
                        logger.info("Scenario ${scenario.name} [run:common_render] phase resulted with scene=[${selectedSceneArg.selectedCase}]")
                        logger.debug("MM_RESPONSE=\n{}", it)
                    }
                }
                SelectedCase.ERRORSCENE -> {
                    val renderRunResponse =
                        TScenarioRunResponse.newBuilder().setVersion(versionProvider.version).setError(
                            ResponseProto.TScenarioError.newBuilder()
                                .setMessage(selectedSceneArg.errorScene.message)
                                .setType(selectedSceneArg.errorScene.type)
                        ).build()
                    logger.info("Scenario ${scenario.name} [run:common_render] phase resulted with scene=[${selectedSceneArg.selectedCase}]")
                    logger.debug("MM_RESPONSE=\n{}", renderRunResponse)
                    ScenarioCompositeResponse(renderRunResponse)
                }
                else -> throw RuntimeException("Can read apphost request key 'kronstadt_selected_scene_run'")

            }
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} [run:common_render] phase failed", ex)
            ScenarioCompositeResponse(
                TScenarioRunResponse.newBuilder()
                    .setVersion(versionProvider.version)
                    .setError(
                        ResponseProto.TScenarioError.newBuilder()
                            .setMessage(ex.message)
                            .setType(ex.javaClass.name)
                    )
                    .build()
            )
        }
        setApphostResponseItems(
            responseBuilder = apphostRequestContext,
            scenarioResponse = scenarioResponse,
            renderData = renderData
        )
    }

    @ScenarioHandler("commit/setup")
    fun prepareCommit(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioApplyRequest,
        apphostRequestContext: ApphostRequestContext,
    ) {
        var sceneName: String? = null
        try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val (request, chosenScene) = scenario.convertCommitRequest(requestProto, requestContext.currentUserId)
            sceneName = chosenScene.scene.name
            logger.debug(
                "Scenario {} [commit:prepare] phase started with sceneName={} AND megamind_request=\n{}",
                scenario.name,
                sceneName,
                request
            )

            chosenScene.prepareCommit(request, scenePrepareBuilder(apphostRequestContext, httpConverter))
            if (chosenScene.scene is CommittingSceneWithPrepare<State, *, *>) {
                addSelectedSceneFlag(apphostRequestContext, chosenScene.scene)
            }
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} scene $sceneName [commit:prepare] phase failed", ex)
            throw ex
        }
    }

    @ScenarioHandler("commit/common")
    @ApphostKey("mm_scenario_response")
    fun processCommit(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioApplyRequest,
        apphostRequest: ApphostRequest,
    ): TScenarioCommitResponse {
        try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val (request, chosenScene) = scenario.convertCommitRequest(
                requestProto,
                requestContext.currentUserId,
                additionalSources(apphostRequest)
            )

            logger.debug(
                "Scenario {} [commit:process] phase started with chosenScene={} AND megamind_request=\n{}",
                scenario.name,
                chosenScene,
                request
            )
            return scenario.renderCommitResponse(request, chosenScene as SelectedScene.Committing<State, *, *>)
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} commit phase failed", ex)
            return TScenarioCommitResponse.newBuilder()
                .setVersion(versionProvider.version)
                .setError(
                    ResponseProto.TScenarioError.newBuilder()
                        .setMessage(ex.message)
                        .setType(ex.javaClass.name)
                )
                .build()
        }
    }

    @ScenarioHandler("continue/setup")
    fun prepareContinue(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioApplyRequest,
        apphostRequestContext: ApphostRequestContext,
    ) {
        var sceneName: String? = null
        try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val (request, chosenScene) = scenario.convertContinueRequest(requestProto, requestContext.currentUserId)
            sceneName = chosenScene.scene.name

            logger.debug(
                "Scenario {} [continue:prepare] phase started with sceneName={} AND megamind_request=\n{}",
                scenario.name,
                sceneName,
                request
            )

            chosenScene.prepareContinue(request, scenePrepareBuilder(apphostRequestContext, httpConverter))
            if (chosenScene.scene is ContinuingSceneWithPrepare<State, *, *>) {
                addSelectedSceneFlag(apphostRequestContext, chosenScene.scene)
            }
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} scene ${sceneName} commit/prepare phase failed", ex)
            throw ex
        }
    }

    @ScenarioHandler("continue/common")
    fun processContinue(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioApplyRequest,
        apphostRequestContext: ApphostRequestContext,
    ) {
        var sceneName: String? = null
        val (scenarioResponse, renderData) = try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val (request, chosenScene) = scenario.convertContinueRequest(
                requestProto,
                requestContext.currentUserId,
                additionalSources(apphostRequestContext)
            )
            sceneName = chosenScene.name

            logger.debug(
                "Scenario {} [continue:process] phase started with chosenScene={} AND megamind_request=\n{}",
                scenario.name,
                chosenScene,
                request
            )

            scenario.renderContinueResponse(request, chosenScene as SelectedScene.Continuing<State, *, *>)
                .also {
                    logger.debug(
                        "Scenario {} [continue:process] phase with {} resulted with megamind_response=\n{}",
                        scenario.name,
                        chosenScene,
                        it
                    )
                }
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} scene $sceneName continue phase failed", ex)
            ScenarioCompositeResponse(
                TScenarioContinueResponse.newBuilder()
                    .setVersion(versionProvider.version)
                    .setError(
                        ResponseProto.TScenarioError.newBuilder()
                            .setMessage(ex.message)
                            .setType(ex.javaClass.name)
                    )
                    .build()
            )
        }
        setApphostResponseItems(
            responseBuilder = apphostRequestContext,
            scenarioResponse = scenarioResponse,
            renderData = renderData
        )
    }

    @ScenarioHandler("apply/setup")
    fun prepareApply(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioApplyRequest,
        apphostRequestContext: ApphostRequestContext,
    ) {
        var sceneName: String? = null
        try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val (request, chosenScene) = scenario.convertApplyRequest(requestProto, requestContext.currentUserId)
            sceneName = chosenScene.scene.name

            logger.debug(
                "Scenario {} [apply:prepare] phase started with sceneName={} AND megamind_request=\n{}",
                scenario.name,
                sceneName,
                request
            )

            chosenScene.prepareApply(request, scenePrepareBuilder(apphostRequestContext, httpConverter))
            if (chosenScene.scene is ApplyingSceneWithPrepare<State, *, *>) {
                addSelectedSceneFlag(apphostRequestContext, chosenScene.scene)
            }
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} scene $sceneName apply/prepare phase failed", ex)
            throw ex
        }
    }

    @ScenarioHandler("apply/common")
    fun processApply(
        @ApphostKey("mm_scenario_request_meta") requestMeta: TRequestMeta,
        @ApphostKey("mm_scenario_request") requestProto: TScenarioApplyRequest,
        apphostRequestContext: ApphostRequestContext,
    ) {
        var sceneName: String? = null
        val (scenarioResponse, renderData) = try {
            populateRequestContext(requestMeta, requestProto.baseRequest.options.userAgent.ifEmpty { null })

            val (request, chosenScene) = scenario.convertApplyRequest(
                requestProto,
                requestContext.currentUserId,
                additionalSources(apphostRequestContext)
            )
            sceneName = chosenScene.name

            logger.debug(
                "Scenario {} [apply:process] phase started with chosenScene={} AND megamind_request=\n{}",
                scenario.name,
                chosenScene,
                request
            )

            scenario.renderApplyResponse(request, chosenScene as SelectedScene.Applying<State, *, *>)
                .also {
                    logger.debug(
                        "Scenario {} [apply:process] phase with {} resulted with megamind_response=\n{}",
                        scenario.name,
                        chosenScene,
                        it
                    )
                }
        } catch (ex: Exception) {
            logger.error("Scenario ${scenario.name} scene $sceneName apply phase failed", ex)
            ScenarioCompositeResponse(
                TScenarioApplyResponse.newBuilder()
                    .setVersion(versionProvider.version)
                    .setError(
                        ResponseProto.TScenarioError.newBuilder()
                            .setMessage(ex.message)
                            .setType(ex.javaClass.name)
                    )
                    .build()
            )
        }
        setApphostResponseItems(
            responseBuilder = apphostRequestContext,
            scenarioResponse = scenarioResponse,
            renderData = renderData,
        )
    }

    private val apphostKeyToDataSourceIndex: Map<String, Int> = TDataSource.getDescriptor()
        .oneofs
        .first { it.name == "Type" }
        .fields
        .filter { it.options.hasExtension(RequestProto.dataSourceType) }
        .associate {
            val extension = it.options.getExtension(RequestProto.dataSourceType)
            ("datasource_" + extension.name) to extension.number
        }

    private val internalKeys = setOf(
        "mm_scenario_request_meta",
        "mm_scenario_request",
        "mm_scenario_response",
        "kronstadt_selected_scene_run",
    ) + apphostKeyToDataSourceIndex.keys

    private fun setApphostResponseItems(
        responseBuilder: ApphostResponseBuilder,
        scenarioResponse: Message,
        renderData: List<Api.TDivRenderData>
    ) {
        responseBuilder.addProtobufItem("mm_scenario_response", scenarioResponse)
        renderData.map { responseBuilder.addProtobufItem("render_data", it) }
    }

    private fun additionalSources(apphostRequest: ApphostRequest): AdditionalSources {
        val items: Map<String, List<(Message) -> Message>> =
            apphostRequest.requestItems
                .filter { !internalKeys.contains(it.type) }
                .groupBy(RequestItem::getType) {
                    return@groupBy { defaultInstance ->
                        it.getProtobufData(defaultInstance)
                    }
                }
        return AdditionalSources(items, httpConverter)
    }

    private fun getDataSourcesMap(apphostRequest: ApphostRequest): Map<Int, TDataSource> {
        return apphostKeyToDataSourceIndex.mapNotNull { (key, index) ->
            apphostRequest.getRequestItems(key)
                .firstOrNull()
                ?.let { value -> index to value.getProtobufData(TDataSource.getDefaultInstance()) }
        }.toMap()
    }

    private fun populateRequestContext(requestMeta: TRequestMeta, userAgent: String? = null) {
        contextPopulator.populateRequestAndLoggingContext(requestMeta, userAgent)
    }

    private val flagName = scenario.name.lowercase()
    private fun addSelectedSceneFlag(
        apphostContext: ApphostResponseBuilder,
        scene: Scene<State, *>
    ) {
        apphostContext.addFlag("kronstadt#${flagName}#${scene.name}")
    }

    companion object {
        private val logger = LogManager.getLogger(ApphostScenarioAdapter::class.java)
        private val BASE_SETRACE_MARKER = SETRACE_TAG_MARKER_PARENT
    }
}
