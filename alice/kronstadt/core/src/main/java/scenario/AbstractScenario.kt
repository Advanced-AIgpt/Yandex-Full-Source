package ru.yandex.alice.kronstadt.core.scenario

import com.google.common.base.Stopwatch
import com.google.protobuf.GeneratedMessageV3
import com.google.protobuf.Message
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Autowired
import ru.yandex.alice.kronstadt.core.AdditionalSources
import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.kronstadt.core.AliceHandledException.ExceptionResponseBody
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.CommitNeededResponse
import ru.yandex.alice.kronstadt.core.ContinueNeededResponse
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse
import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import ru.yandex.alice.kronstadt.core.IrrelevantResponseException
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunErrorResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioCompositeResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.VersionProvider
import ru.yandex.alice.kronstadt.core.WithArgumentResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.convert.StateConverter
import ru.yandex.alice.kronstadt.core.convert.request.ContactsListConverter
import ru.yandex.alice.kronstadt.core.convert.request.InputConverter
import ru.yandex.alice.kronstadt.core.convert.request.MegamindRequestConverter
import ru.yandex.alice.kronstadt.core.convert.request.VideoCallCapabilityConverter
import ru.yandex.alice.kronstadt.core.convert.response.ActionConverter
import ru.yandex.alice.kronstadt.core.convert.response.ActionSpaceConverter
import ru.yandex.alice.kronstadt.core.convert.response.AnalyticsInfoConverter
import ru.yandex.alice.kronstadt.core.convert.response.CallbackDirectiveConverter
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverter
import ru.yandex.alice.kronstadt.core.convert.response.DivRenderDataConverter
import ru.yandex.alice.kronstadt.core.convert.response.FeaturesConverter
import ru.yandex.alice.kronstadt.core.convert.response.LayoutConverter
import ru.yandex.alice.kronstadt.core.convert.response.ScenarioResponseBodyConverter
import ru.yandex.alice.kronstadt.core.convert.response.ServerDirectiveConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextCard
import ru.yandex.alice.kronstadt.core.rpc.RpcHandler
import ru.yandex.alice.kronstadt.core.scenePrepareBuilder
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSceneArguments
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSceneArguments.SceneArgumentCase
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSelectedSceneForApply
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioBaseRequest
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioRunRequest
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioApplyResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioCommitResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioCommitResponse.TSuccess
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioContinueResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioError
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse.TCommitCandidate
import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Histogram
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder
import java.util.concurrent.TimeUnit
import java.util.function.Predicate
import kotlin.Any
import kotlin.contracts.ExperimentalContracts
import kotlin.contracts.InvocationKind
import kotlin.contracts.contract
import kotlin.reflect.KClass
import com.google.protobuf.Any as ProtobufAny

abstract class AbstractScenario<State>(
    final override val scenarioMeta: ScenarioMeta,
    protected val megamindRequestListeners: List<MegaMindRequestListener> = listOf()
) : IScenario<State> {

    @Autowired
    protected lateinit var metricRegistry: MetricRegistry

    @Autowired
    private lateinit var versionProvider: VersionProvider

    @Autowired
    private lateinit var protoUtil: ProtoUtil

    @Autowired
    private lateinit var inputConverter: InputConverter

    @Autowired
    private lateinit var layoutConverter: LayoutConverter

    @Autowired
    private lateinit var callbackDirectiveConverter: CallbackDirectiveConverter

    @Autowired
    private lateinit var serverDirectiveConverter: ServerDirectiveConverter

    @Autowired
    private lateinit var featureConverter: FeaturesConverter

    @Autowired
    private lateinit var directiveConverter: DirectiveConverter

    @Autowired
    private lateinit var divRenderDataConverter: DivRenderDataConverter

    @Autowired
    private lateinit var contactsListConverter: ContactsListConverter

    @Autowired
    private lateinit var actionSpaceConverter: ActionSpaceConverter

    @Autowired
    private lateinit var videoCallCapabilityConverter: VideoCallCapabilityConverter

    @Autowired
    private lateinit var httpConverter: HttpRequestConverter

    init {
        logger.info("Starting with scenario: ${scenarioMeta.name} ")
    }

    private val stateConverter = object : StateConverter<State> {
        override fun convert(src: TScenarioBaseRequest): State? = protoToState(src)
        override fun convert(src: State, ctx: ToProtoContext): Message = stateToProto(src, ctx)
    }

    protected open fun applyArgumentsToProto(applyArguments: ApplyArguments): Message {
        TODO("Apply argument conversion to proto not implemented for scenario ${name}")
    }

    protected open fun applyArgumentsFromProto(arg: ProtobufAny): ApplyArguments {
        TODO("Apply argument conversion from proto not implemented for scenario ${name}")
    }

    @Autowired(required = false)
    protected open val irrelevantResponseFactory: IrrelevantResponse.Factory<State> =
        DefaultIrrelevantResponse.Factory()

    private inner class ExceptionHandledScene :
        AbstractScene<State, ExceptionResponseBody>("ExceptionHandlingScene", ExceptionResponseBody::class),
        ApplyingScene<State, ExceptionResponseBody, GeneratedMessageV3>,
        ContinuingScene<State, ExceptionResponseBody, GeneratedMessageV3> {

        private fun baseResponse(
            request: MegaMindRequest<State>,
            args: ExceptionResponseBody
        ) = ScenarioResponseBody(
            layout = Layout(
                outputSpeech = args.aliceSpeech,
                shouldListen = args.expectRequest,
                cards = listOf(TextCard(text = args.aliceText))
            ),
            state = request.state,
            analyticsInfo = AnalyticsInfo("exception_handler", actions = listOf(args.action))
        )

        override val applyArgsClass = GeneratedMessageV3::class

        override fun render(
            request: MegaMindRequest<State>,
            args: ExceptionResponseBody
        ): RelevantResponse<State> {
            return RunOnlyResponse(baseResponse(request, args))
        }

        override fun processApply(
            request: MegaMindRequest<State>,
            args: ExceptionResponseBody,
            applyArg: GeneratedMessageV3,
        ): ScenarioResponseBody<State> = baseResponse(request, args)

        override fun processContinue(
            request: MegaMindRequest<State>,
            args: ExceptionResponseBody,
            applyArg: GeneratedMessageV3,
        ): ScenarioResponseBody<State> = baseResponse(request, args)
    }

    private val exceptionHandledScene = ExceptionHandledScene()

    override lateinit var scenes: List<Scene<State, *>>

    override var grpcHandlers: List<RpcHandler<*, *>> = listOf()

    private lateinit var scenesMapByClass: Map<KClass<out Scene<State, *>>, Scene<State, *>>

    protected lateinit var noargScenesMapByClass: Map<KClass<out NoargScene<State>>, NoargScene<State>>

    @Autowired(required = true)
    internal fun configureScenes(scenes: List<Scene<State, *>>) {
        val filteredScenes = scenes.filter { it.javaClass.packageName.startsWith(this.javaClass.packageName) }
        filteredScenes.groupBy { it.name }
            .entries.filter { (_, value) -> value.size > 1 }
            .forEach { (key, _) -> throw RuntimeException("Multiple scenes with name '${key}' found. Scene name must be unique") }
        this.scenes = filteredScenes

        // call lazy initialization
        this.scenesMapByClass = this.scenes.associateBy { it::class }
        this.noargScenesMapByClass = this.scenes.filterIsInstance<NoargScene<State>>().associateBy { it::class }
    }

    @Autowired(required = false)
    internal fun configureGrpcHandlers(handlers: List<RpcHandler<*, *>> = listOf()) {
        this.grpcHandlers = handlers.filter { it.javaClass.packageName.startsWith(this.javaClass.packageName) }
    }

    private val scenarioRegistry: MetricRegistry by lazy {
        metricRegistry.subRegistry(
            Labels.of(
                "scenario",
                scenarioMeta.name
            )
        )
    }

    private val scenesMap: Map<String, Scene<State, *>> by lazy { scenes.associateBy { it.name } }
    private val requestConverter: MegamindRequestConverter<State> by lazy {
        MegamindRequestConverter(
            scenarioMeta = scenarioMeta,
            inputConverter = inputConverter,
            protoUtil = protoUtil,
            stateConverter = stateConverter,
            contactsListConverter = contactsListConverter,
            videoCallCapabilityConverter = videoCallCapabilityConverter,
        )
    }
    private val analyticsInfoConverter = AnalyticsInfoConverter(scenarioMeta)
    private val actionConverter: ActionConverter by lazy {
        ActionConverter(
            scenarioMeta = scenarioMeta,
            directiveConverter = directiveConverter,
            callbackDirectiveConverter = callbackDirectiveConverter,
        )
    }
    private val responseConverter: ScenarioResponseBodyConverter<State> by lazy {
        ScenarioResponseBodyConverter(
            scenarioMeta = scenarioMeta,
            layoutConverter = layoutConverter,
            analyticsInfoConverter = analyticsInfoConverter,
            stateConverter = stateConverter,
            actionConverter = actionConverter,
            actionSpaceConverter = actionSpaceConverter,
            directiveConverter = callbackDirectiveConverter,
            serverDirectiveConverter = serverDirectiveConverter
        )
    }

    private val version: String by lazy { versionProvider.version }

    protected abstract fun stateToProto(state: State, ctx: ToProtoContext): Message
    protected abstract fun protoToState(request: TScenarioBaseRequest): State?

    protected inner class HandleProcess internal constructor(
        val request: MegaMindRequest<State>,
    ) {
        var found: SelectedScene.Running<State, *>? = null

        @OptIn(ExperimentalContracts::class)
        inline fun handle(body: () -> SelectedScene.Running<State, *>?): SelectedScene.Running<State, *>? {
            contract { callsInPlace(body, InvocationKind.AT_MOST_ONCE) }
            if (found == null) {
                val scene = body.invoke()
                // inside the invocation `found` may already be set. We shouldn't override it
                /*
                onFoo {
                    onClient(ClientInfo::isStationMini) {
                        scene(PlayScene::class, frame)
                    }
                    scene<StationDisclaimerScene>()
                }*/
                if (found == null) found = scene
            }
            return found
        }
    }

    protected open fun prepareSelectScene(request: MegaMindRequest<State>, responseBuilder: ApphostResponseBuilder) {
    }

    final override fun doPrepareSelectScene(request: MegaMindRequest<State>, responseBuilder: ApphostResponseBuilder) {
        scenarioRegistry.rate("kronstadt.prepare_select_scene.invocations").inc()

        megamindRequestListeners.forEach { it.onPrepareSelectScene(request) }

        try {
            prepareSelectScene(request, responseBuilder)
        } catch (e: Exception) {
            scenarioRegistry.rate("kronstadt.prepare_select_scene.failures").inc()
            logger.error("Failed to prepare select scene of scenario ${name}", e)
            throw e
        }
    }

    //protected abstract fun HandleProcess<State>.selectScene(request: MegaMindRequest<State>): ChosenScene<State, *>?
    protected abstract fun selectScene(request: MegaMindRequest<State>): SelectedScene.Running<State, *>?

    final override fun doSelectScene(request: MegaMindRequest<State>): SelectedScene.Running<State, *>? {

        megamindRequestListeners.forEach { it.onRun(request) }

        return try {
            monitorSceneSelection(request)
        } catch (e: AliceHandledException) {
            logger.error("Alice handled error on scenario ${scenarioMeta.name} select_scene stage: ${e.message}", e)
            SelectedScene.Running(exceptionHandledScene, e.responseBody)
        } catch (e: Exception) {
            logger.error("Unexpected exception on scenario ${scenarioMeta.name} select_scene stage: ${e.message}", e)
            throw e
        }
    }

    private val selectSceneRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate("kronstadt.select_scene.invocations", Labels.of("scene", it))
    }
    private val selectSceneHists = SimpleCache<String, Histogram> {
        scenarioRegistry.histogramRate("kronstadt.select_scene.times", Labels.of("scene", it),
            { Histograms.exponential(15, 1.5, 14.0) }
        )
    }

    private fun monitorSceneSelection(request: MegaMindRequest<State>): SelectedScene.Running<State, *>? {
        var sceneName: String? = null
        val sw = Stopwatch.createStarted()
        try {
            val scene = selectScene(request)
            sw.stop()
            sceneName = scene?.scene?.name ?: "irrelevant"
            return scene
        } catch (e: Exception) {
            sw.stop()
            scenarioRegistry.rate("kronstadt.select_scene.errors").inc()
            sceneName = "none"
            throw e
        } finally {
            selectSceneRates[sceneName!!].inc()
            selectSceneHists[sceneName].record(sw.elapsed(TimeUnit.MILLISECONDS))
        }
    }

    private val prepareRunRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "prepare_run", "scene", it)
        )
    }
    private val prepareRunFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "prepare_run", "scene", it)
        )
    }

    override fun prepareRun(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Running<State, *>
    ) {
        prepareRunRates[selectedScene.name].inc()

        megamindRequestListeners.forEach { it.onPrepareRun(request) }

        return try {
            selectedScene.prepareRun(request, scenePrepareBuilder(responseBuilder, httpConverter))
        } catch (e: Exception) {
            prepareRunFailureRates[selectedScene.name].inc()
            logger.error(
                "Failed to prepare apply for scene ${selectedScene.name} of scenario ${name}",
                e
            )
            throw e
        }
    }

    private val runRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "run", "scene", it)
        )
    }
    private val runFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "run", "scene", it)
        )
    }

    override fun renderRunResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Running<State, *>?
    ): ScenarioCompositeResponse<TScenarioRunResponse> {
        val sceneName = selectedScene?.scene?.name ?: "irrelevant"
        runRates[sceneName].inc()

        megamindRequestListeners.forEach { it.onRun(request) }

        val response: BaseRunResponse<State> = try {

            selectedScene?.render(request) ?: irrelevantResponseFactory.create(request)
        } catch (e: AliceHandledException) {
            runFailureRates[sceneName].inc()
            logger.error("Alice handled error on scenario ${scenarioMeta.name} run_render stage: ${e.message}", e)
            exceptionHandledScene.render(request, e.responseBody)
        } catch (e: IrrelevantResponseException) {
            runFailureRates[sceneName].inc()
            logger.info("Irrelevant exception raised on scenario ${scenarioMeta.name}")
            @Suppress("UNCHECKED_CAST")
            IrrelevantResponse(e.scenarioResponseBody as ScenarioResponseBody<State>)
        } catch (e: Exception) {
            runFailureRates[sceneName].inc()
            logger.error("Unexpected exception on scenario ${scenarioMeta.name} run_render stage: ${e.message}", e)
            throw e
        }

        try {
            return convertRunResponse(selectedScene, response)
        } catch (e: Exception) {
            runFailureRates[sceneName].inc()
            logger.error("Failed to convert run response from scene $sceneName", e)
            throw e
        }
    }

    override fun convertApplyRequest(
        req: RequestProto.TScenarioApplyRequest,
        uid: String?,
        additionalSources: AdditionalSources,
    ): Pair<MegaMindRequest<State>, SelectedScene.Applying<State, out Any, out GeneratedMessageV3>> {
        val request = convertMegaMindMRequest(uid, req.baseRequest, req.input, req.dataSourcesMap, additionalSources)

        val argsProto = req.arguments.unpack(TSelectedSceneForApply::class.java)
        val chosenScene = chooseApplyingScene(
            argsProto.selectedScene,
            argsProto.applyArguments
        )
        return Pair(request, chosenScene)
    }

    override fun convertCommitRequest(
        req: RequestProto.TScenarioApplyRequest,
        uid: String?,
        additionalSources: AdditionalSources,
    ): Pair<MegaMindRequest<State>, SelectedScene.Committing<State, out Any, out GeneratedMessageV3>> {
        val request = convertMegaMindMRequest(uid, req.baseRequest, req.input, req.dataSourcesMap, additionalSources)
        val argsProto = req.arguments.unpack(TSelectedSceneForApply::class.java)
        val chosenScene = chooseCommittingScene(
            argsProto.selectedScene,
            argsProto.applyArguments
        )
        return Pair(request, chosenScene)
    }

    override fun convertContinueRequest(
        req: RequestProto.TScenarioApplyRequest,
        uid: String?,
        additionalSources: AdditionalSources,
    ): Pair<MegaMindRequest<State>, SelectedScene.Continuing<State, out Any, out GeneratedMessageV3>> {
        val request = convertMegaMindMRequest(uid, req.baseRequest, req.input, req.dataSourcesMap, additionalSources)
        val argsProto = req.arguments.unpack(TSelectedSceneForApply::class.java)
        val chosenScene = chooseContinuingScene(
            argsProto.selectedScene,
            argsProto.applyArguments
        )
        return Pair(request, chosenScene)
    }

    override fun convertRunRequest(req: TScenarioRunRequest, uid: String?, additionalSources: AdditionalSources) =
        requestConverter.convertRunRequest(req, uid, additionalSources)

    override fun convertMegaMindMRequest(
        userId: String?,
        baseRequest: TScenarioBaseRequest,
        input: RequestProto.TInput,
        dataSourcesMap: Map<Int, RequestProto.TDataSource>,
        additionalSources: AdditionalSources
    ) = requestConverter.getMegaMindRequest(
        userId,
        baseRequest,
        input,
        dataSourcesMap,
        additionalSources,
    )

    @Suppress("UNCHECKED_CAST")
    private fun convertRunResponse(
        selectedScene: SelectedScene<State, *>?,
        response: BaseRunResponse<State>
    ): ScenarioCompositeResponse<TScenarioRunResponse> {
        val ctx = ToProtoContext()
        val runResponse = TScenarioRunResponse.newBuilder().apply {
            features = featureConverter.convert(response)
            version = versionProvider.version

            // make `when` an expression to force compilation error on missing sealed class check
            @Suppress("UNUSED_VARIABLE")
            val ignored: Unit = when (response) {
                is RunOnlyResponse<*> ->
                    responseBody = responseConverter.convert((response as RunOnlyResponse<State>).body, ctx)

                is ApplyNeededResponse<*> -> {
                    if (selectedScene == null) throw RuntimeException("Apply is not allowed for irrelevant response")
                    if (selectedScene.scene !is ApplyingScene<*, *, *>) throw RuntimeException("Apply returned from not applying scene")

                    applyArguments = convertArguments(selectedScene, response)
                }
                is CommitNeededResponse<*> -> {
                    if (selectedScene == null) throw RuntimeException("Commit is not allowed for irrelevant response")
                    if (selectedScene.scene !is CommittingScene<*, *, *>) throw RuntimeException("Commit returned from not committing scene")

                    commitCandidate = TCommitCandidate.newBuilder()
                        .setArguments(convertArguments(selectedScene, response))
                        .setResponseBody(responseConverter.convert((response as CommitNeededResponse<State>).body, ctx))
                        .build()
                }
                is ContinueNeededResponse<*> -> {
                    if (selectedScene == null) throw RuntimeException("Continue is not allowed for irrelevant response")
                    if (selectedScene.scene !is ContinuingScene<*, *, *>) throw RuntimeException("Continue returned from not continuing scene")

                    continueArguments = convertArguments(selectedScene, response)
                }
                is IrrelevantResponse<*> ->
                    responseBody = responseConverter.convert((response as IrrelevantResponse<State>).body, ctx)
                is RunErrorResponse ->
                    error = TScenarioError.newBuilder().setMessage(response.message).build()
            }
        }.build()
        val renderData =
            if (response is RunOnlyResponse<*> &&
                response.body is ScenarioResponseBody<*>
            ) {
                response.body.renderData.map { divRenderDataConverter.convert(it, ctx) }
            } else listOf()

        return ScenarioCompositeResponse(
            scenarioResponse = runResponse,
            renderData = renderData
        )
    }

    private fun convertArguments(selectedScene: SelectedScene<State, *>, response: WithArgumentResponse): ProtobufAny {

        return ProtobufAny.pack(
            TSelectedSceneForApply.newBuilder()
                .setSelectedScene(
                    packSceneArguments(selectedScene)
                )
                .setApplyArguments(ProtobufAny.pack(applyArgumentsToProto(response.arguments)))
                .build()
        )
    }

    override fun packSceneArguments(selectedScene: SelectedScene<State, *>): TSceneArguments {
        return TSceneArguments.newBuilder().apply {
            sceneName = selectedScene.name
            val sceneArgs = selectedScene.args
            if (sceneArgs is Message) {
                protoArgs = com.google.protobuf.Any.pack(sceneArgs)
            } else {
                args = protoUtil.objectToStruct(sceneArgs)
            }
        }.build()
    }

    @OptIn(ExperimentalContracts::class)
    protected fun MegaMindRequest<State>.handle(selectScene: HandleProcess.() -> SelectedScene.Running<State, *>?)
        : SelectedScene.Running<State, *>? {
        contract { callsInPlace(selectScene, InvocationKind.EXACTLY_ONCE) }
        return HandleProcess(this).selectScene()
    }

    private fun <State, Args : Any> Scene<State, Args>.chooseForRun(sceneArguments: TSceneArguments):
        SelectedScene.Running<State, Args> {
        val structToObject: Args = unpackSceneArguments(sceneArguments)
        return SelectedScene.Running(this, structToObject)
    }

    private fun <Args : Any, State> Scene<State, Args>.unpackSceneArguments(
        sceneArguments: TSceneArguments
    ): Args {
        return when (sceneArguments.sceneArgumentCase) {
            SceneArgumentCase.ARGS -> protoUtil.structToObject(sceneArguments.args, argsClass.java)
            else -> try {
                @Suppress("UNCHECKED_CAST")
                sceneArguments.protoArgs.unpack(this.argsClass.java as Class<Message>) as Args
            } catch (e: Exception) {
                logger.error(
                    "Failed to unpack arguments. expected: ${sceneArguments.protoArgs.typeUrl} but tried with ${this.argsClass.java.name}",
                    e
                )
                throw e
            }
        }
    }

    private fun <State, Args : Any, ApplyArg : GeneratedMessageV3>
        ApplyingScene<State, Args, ApplyArg>.chooseForApply(sceneArguments: TSceneArguments, applyArgs: ProtobufAny):
        SelectedScene.Applying<State, Args, ApplyArg> {
        val structToObject: Args = unpackSceneArguments(sceneArguments)
        return SelectedScene.Applying(this, structToObject, applyArgs.unpack(this.applyArgsClass.java))
    }

    private fun <State, Args : Any, ApplyArg : GeneratedMessageV3>
        CommittingScene<State, Args, ApplyArg>.chooseForCommit(sceneArguments: TSceneArguments, applyArgs: ProtobufAny):
        SelectedScene.Committing<State, Args, ApplyArg> {
        val structToObject: Args = unpackSceneArguments(sceneArguments)
        return SelectedScene.Committing(this, structToObject, applyArgs.unpack(this.applyArgsClass.java))
    }

    private fun <State, Args : Any, ApplyArg : GeneratedMessageV3>
        ContinuingScene<State, Args, ApplyArg>.chooseForContinue(
        sceneArguments: TSceneArguments,
        applyArgs: ProtobufAny
    ):
        SelectedScene.Continuing<State, Args, ApplyArg> {
        val structToObject: Args = unpackSceneArguments(sceneArguments)
        return SelectedScene.Continuing(this, structToObject, applyArgs.unpack(this.applyArgsClass.java))
    }

    override fun chooseRunningScene(sceneArguments: TSceneArguments): SelectedScene.Running<State, *> {
        val scene = scenesMap[sceneArguments.sceneName]
            ?: throw RuntimeException("Scene ${sceneArguments.sceneName} not found for scenario ${name}")
        return scene.chooseForRun(sceneArguments)
    }

    override fun chooseApplyingScene(
        sceneArguments: TSceneArguments,
        applyArgs: ProtobufAny
    ): SelectedScene.Applying<State, *, *> {
        val scene = scenesMap[sceneArguments.sceneName]
            ?: throw RuntimeException("Scene ${sceneArguments.sceneName} not found for scenario ${name}")
        val applyingScene = scene as? ApplyingScene<State, *, *>
            ?: throw RuntimeException("Scene ${scene.name} is not a applying scene")
        return applyingScene.chooseForApply(sceneArguments, applyArgs)
    }

    override fun chooseCommittingScene(
        sceneArguments: TSceneArguments,
        applyArgs: ProtobufAny
    ): SelectedScene.Committing<State, *, *> {
        val scene = scenesMap[sceneArguments.sceneName]
            ?: throw RuntimeException("Scene ${sceneArguments.sceneName} not found for scenario ${name}")
        val committingScene = scene as? CommittingScene<State, *, *>
            ?: throw RuntimeException("Scene ${scene.name} is not a committing scene")
        return committingScene.chooseForCommit(sceneArguments, applyArgs)
    }

    override fun chooseContinuingScene(
        sceneArguments: TSceneArguments,
        applyArgs: ProtobufAny
    ): SelectedScene.Continuing<State, *, *> {
        val scene = scenesMap[sceneArguments.sceneName]
            ?: throw RuntimeException("Scene ${sceneArguments.sceneName} not found for scenario ${name}")
        val continuingScene = scene as? ContinuingScene<State, *, *>
            ?: throw RuntimeException("Scene ${scene.name} is not a continuing scene")
        return continuingScene.chooseForContinue(sceneArguments, applyArgs)
    }

    private val prepareApplyRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "prepare_apply", "scene", it)
        )
    }
    private val prepareApplyFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "prepare_apply", "scene", it)
        )
    }

    override fun prepareApply(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Applying<State, *, *>
    ) {
        prepareApplyRates[selectedScene.name].inc()

        megamindRequestListeners.forEach { it.onPrepareCommit(request) }

        return try {
            selectedScene.prepareApply(request, scenePrepareBuilder(responseBuilder, httpConverter))
        } catch (e: Exception) {
            prepareApplyFailureRates[selectedScene.name].inc()
            logger.error(
                "Failed to prepare apply for scene ${selectedScene.name} of scenario ${name}",
                e
            )
            throw e
        }
    }

    private val applyRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "apply", "scene", it)
        )
    }

    private val applyFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "apply", "scene", it)
        )
    }

    override fun renderApplyResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Applying<State, *, *>
    ): ScenarioCompositeResponse<TScenarioApplyResponse> {
        val scene = selectedScene.name
        applyRates[scene].inc()

        megamindRequestListeners.forEach { it.onApply(request) }
        val response: ScenarioResponseBody<State> = try {

            selectedScene.renderApply(request)
        } catch (e: AliceHandledException) {
            applyFailureRates[name].inc()
            logger.error("Alice handled error on scenario ${name} scene ${scene} apply stage: ${e.message}", e)
            exceptionHandledScene.processApply(request, e.responseBody, selectedScene.applyArgs)
        } catch (e: Exception) {
            applyFailureRates[name].inc()
            logger.error("Unexpected exception on scenario ${name} scene ${scene} apply stage: ${e.message}", e)
            throw e
        }

        try {
            return convertApplyResponse(response)
        } catch (e: Exception) {
            applyFailureRates[name].inc()
            logger.error("Failed to convert apply response from scenario ${name} scene ${scene}", e)
            throw e
        }
    }

    private fun convertApplyResponse(
        response: ScenarioResponseBody<State>
    ): ScenarioCompositeResponse<TScenarioApplyResponse> {
        val ctx = ToProtoContext()
        val body = responseConverter.convert(response, ctx)
        val applyResponse = TScenarioApplyResponse.newBuilder()
            .setResponseBody(body)
            .setVersion(version)
            .build()
        val renderData = response.renderData.map { divRenderDataConverter.convert(it, ctx) }

        return ScenarioCompositeResponse(
            scenarioResponse = applyResponse,
            renderData = renderData
        )
    }

    private val prepareCommitRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "prepare_commit", "scene", it)
        )
    }
    private val prepareCommitFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "prepare_commit", "scene", it)
        )
    }

    override fun prepareCommit(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Committing<State, *, *>
    ) {
        prepareCommitRates[selectedScene.name].inc()

        megamindRequestListeners.forEach { it.onPrepareCommit(request) }

        return try {
            selectedScene.prepareCommit(request, scenePrepareBuilder(responseBuilder, httpConverter))
        } catch (e: Exception) {
            prepareCommitFailureRates[selectedScene.name].inc()
            logger.error(
                "Failed to prepare commit for scene ${selectedScene.name} of scenario ${name}",
                e
            )
            throw e
        }
    }

    private val commitRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "commit", "scene", it)
        )
    }
    private val commitFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "commit", "scene", it)
        )
    }

    override fun renderCommitResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Committing<State, *, *>
    ): TScenarioCommitResponse {

        commitRates[selectedScene.name].inc()

        megamindRequestListeners.forEach { it.onCommit(request) }

        val scene = selectedScene.name
        try {
            selectedScene.commit(request)

            return TScenarioCommitResponse.newBuilder()
                .setSuccess(TSuccess.getDefaultInstance())
                .setVersion(version)
                .build()
        } catch (e: AliceHandledException) {
            logger.error("Alice handled error on scenario ${name} scene ${scene} commit stage: ${e.message}", e)
            commitFailureRates[selectedScene.name].inc()
            return TScenarioCommitResponse.newBuilder()
                .setError(
                    TScenarioError.newBuilder()
                        .setMessage(e.message)
                        .setType(e::class.qualifiedName)
                )
                .setVersion(version)
                .build()
        } catch (e: Exception) {
            logger.error("Unexpected exception on scenario ${name} scene ${scene} commit stage: ${e.message}", e)
            commitFailureRates[selectedScene.name].inc()
            return TScenarioCommitResponse.newBuilder()
                .setError(
                    TScenarioError.newBuilder()
                        .setMessage(e.message)
                        .setType(e::class.qualifiedName)
                )
                .setVersion(version)
                .build()
        }
    }

    private val prepareContinueRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "prepare_continue", "scene", it)
        )
    }
    private val prepareContinueFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "prepare_continue", "scene", it)
        )
    }

    override fun prepareContinue(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Continuing<State, *, *>
    ) {
        prepareContinueRates[selectedScene.name].inc()

        megamindRequestListeners.forEach { it.onPrepareContinue(request) }

        return try {
            selectedScene.prepareContinue(request, scenePrepareBuilder(responseBuilder, httpConverter))
        } catch (e: Exception) {
            prepareContinueFailureRates[selectedScene.name].inc()
            logger.error(
                "Failed to prepare continue for scene ${selectedScene.name} of scenario ${name}",
                e
            )
            throw e
        }
    }

    private val continueRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_response",
            Labels.of("stage", "continue", "scene", it)
        )
    }
    private val continueFailureRates = SimpleCache<String, Rate> {
        scenarioRegistry.rate(
            "scenario_scene_failure",
            Labels.of("stage", "continue", "scene", it)
        )
    }

    override fun renderContinueResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Continuing<State, *, *>
    ): ScenarioCompositeResponse<ResponseProto.TScenarioContinueResponse> {

        megamindRequestListeners.forEach { it.onContinue(request) }

        continueRates[selectedScene.name].inc()

        val response: ScenarioResponseBody<State> = try {
            selectedScene.renderContinue(request)
        } catch (e: AliceHandledException) {
            continueFailureRates[selectedScene.name].inc()
            logger.error(
                "Alice handled error on scenario ${name} scene ${selectedScene.name} continue stage: ${e.message}",
                e
            )
            exceptionHandledScene.processApply(request, e.responseBody, selectedScene.applyArgs)
        } catch (e: Exception) {
            continueFailureRates[selectedScene.name].inc()
            logger.error(
                "Unexpected exception on scenario ${name} scene ${selectedScene.name} continue stage: ${e.message}",
                e
            )
            throw e
        }

        try {
            return convertContinueResponse(response)
        } catch (e: Exception) {
            continueFailureRates[selectedScene.name].inc()
            logger.error("Failed to convert continue response from scenario ${name} scene ${selectedScene.name}", e)
            throw e
        }
    }

    private fun convertContinueResponse(
        response: ScenarioResponseBody<State>
    ): ScenarioCompositeResponse<TScenarioContinueResponse> {
        val ctx = ToProtoContext()
        val body = responseConverter.convert(response, ctx)
        val continueResponse = TScenarioContinueResponse.newBuilder()
            .setResponseBody(body)
            .setVersion(version)
            .build()
        val renderData = response.renderData.map { divRenderDataConverter.convert(it, ctx) }

        return ScenarioCompositeResponse(
            scenarioResponse = continueResponse,
            renderData = renderData
        )
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onFrame(
        frameName: String,
        body: MegaMindRequest<State>.(SemanticFrame) -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract { callsInPlace(body, InvocationKind.AT_MOST_ONCE) }
        return handle { request.getSemanticFrame(frameName)?.let { frame -> body(request, frame) } }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onAnyFrame(
        vararg frameName: String,
        body: MegaMindRequest<State>.(SemanticFrame) -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract { callsInPlace(body, InvocationKind.AT_MOST_ONCE) }
        return handle { request.getAnySemanticFrame(frameName.asList())?.let { frame -> body(request, frame) } }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onFrameWithSlot(
        frameName: String, slotType: String,
        body: MegaMindRequest<State>.(SemanticFrame, SemanticFrameSlot) -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract { callsInPlace(body, InvocationKind.AT_MOST_ONCE) }
        return handle {
            request.getSemanticFrame(frameName)
                ?.takeIf { frame -> frame.hasValuedSlot(slotType) }
                ?.let { frame -> body(request, frame, frame.getFirstSlot(slotType)!!) }
        }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onClient(
        noinline check: (ClientInfo) -> Boolean,
        body: MegaMindRequest<State>.() -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract {
            callsInPlace(check, InvocationKind.AT_MOST_ONCE)
            callsInPlace(body, InvocationKind.AT_MOST_ONCE)
        }
        return handle { if (check(request.clientInfo)) body(request) else null }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onCondition(
        noinline check: MegaMindRequest<State>.() -> Boolean,
        body: MegaMindRequest<State>.() -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract {
            callsInPlace(check, InvocationKind.AT_MOST_ONCE)
            callsInPlace(body, InvocationKind.AT_MOST_ONCE)
        }
        return handle { if (check(request)) body(request) else null }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onCondition(
        check: Boolean,
        body: MegaMindRequest<State>.() -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract {
            callsInPlace(body, InvocationKind.AT_MOST_ONCE)
        }
        return handle { if (check) body(request) else null }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onCondition(
        check: Predicate<MegaMindRequest<State>>,
        body: MegaMindRequest<State>.() -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract {
            callsInPlace(body, InvocationKind.AT_MOST_ONCE)
        }
        return handle { if (check.test(request)) body(request) else null }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun <reified T : CallbackDirective> HandleProcess.onCallback(
        body: MegaMindRequest<State>.(T) -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract { callsInPlace(body, InvocationKind.AT_MOST_ONCE) }
        return handle {
            if (request.input.isCallback(T::class.java))
                body(request, request.input.getDirective(T::class.java))
            else null
        }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onExperiment(
        exp: String, body: MegaMindRequest<State>.() -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract { callsInPlace(body, InvocationKind.AT_MOST_ONCE) }
        return handle { if (request.hasExperiment(exp)) body(request) else null }
    }

    @OptIn(ExperimentalContracts::class)
    protected inline fun HandleProcess.onExperimentValue(
        exp: String, expValue: String, body: MegaMindRequest<State>.() -> SelectedScene.Running<State, *>?
    ): SelectedScene.Running<State, *>? {
        contract { callsInPlace(body, InvocationKind.AT_MOST_ONCE) }
        return handle { if (request.getExperimentWithValue(exp, "") == expValue) body(request) else null }
    }

    protected inline fun <reified T : NoargScene<State>> HandleProcess.scene():
        SelectedScene.Running<State, *>? = handle {
        val scene = noargScenesMapByClass[T::class]
            ?: throw RuntimeException("No NoargScene scene of class ${T::class.java.name} registered in scenario ${scenarioMeta.name}")

        return@handle SelectedScene.Running(scene as T, scene.arg)
    }

    protected fun HandleProcess.scene(sceneKClass: KClass<out NoargScene<State>>):
        SelectedScene.Running<State, *>? = handle {
        val scene = noargScenesMapByClass[sceneKClass]
            ?: throw RuntimeException("No NoargScene scene of class ${sceneKClass.java.name} registered in scenario ${scenarioMeta.name}")
        return@handle SelectedScene.Running(scene, scene.arg)
    }

    protected fun <Args : Any> HandleProcess.sceneWithArgs(sceneKClass: KClass<out Scene<State, Args>>, args: Args):
        SelectedScene.Running<State, *>? = handle {
        val scene = scenesMapByClass[sceneKClass]
            ?: throw RuntimeException("No Scene of class ${sceneKClass.java.name} registered in scenario ${scenarioMeta.name}")
        @Suppress("UNCHECKED_CAST")
        return@handle SelectedScene.Running(scene as Scene<State, Args>, args)
    }

    companion object {
        private val logger = LogManager.getLogger(AbstractScenario::class.java)
    }
}
