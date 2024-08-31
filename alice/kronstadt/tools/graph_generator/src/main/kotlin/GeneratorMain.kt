package ru.yandex.alice.kronstadt.generator

import NAppHostPbConfig.NNora.Graph
import NAppHostPbConfig.NNora.Graph.TNode
import NAppHostPbConfig.NNora.Monitoring
import NAppHostPbConfig.NNora.Shared
import com.google.protobuf.StringValue
import com.google.protobuf.TextFormat
import com.google.protobuf.UInt32Value
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.getBeansOfType
import org.springframework.boot.SpringApplication
import org.springframework.boot.WebApplicationType
import picocli.CommandLine
import picocli.CommandLine.Command
import picocli.CommandLine.Option
import ru.yandex.alice.kronstadt.core.scenario.AbstractScenario
import ru.yandex.alice.kronstadt.core.scenario.ApplyingScene
import ru.yandex.alice.kronstadt.core.scenario.ApplyingSceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.CommittingScene
import ru.yandex.alice.kronstadt.core.scenario.CommittingSceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.ContinuingScene
import ru.yandex.alice.kronstadt.core.scenario.ContinuingSceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.IScenario
import ru.yandex.alice.kronstadt.core.scenario.SceneWithPrepare
import ru.yandex.alice.kronstadt.runner.KronstadtApplication
import ru.yandex.alice.kronstadt.server.apphost.warmup.scenario.DUMMY_SCENARIO_META
import ru.yandex.alice.kronstadt.tools.generator.TGraphWrapper
import ru.yandex.alice.megamind.library.config.scenario_protos.ERequestType
import ru.yandex.alice.megamind.library.config.scenario_protos.TAbcService
import ru.yandex.alice.megamind.library.config.scenario_protos.TScenarioConfig
import java.io.File
import java.nio.file.Path
import java.util.concurrent.Callable
import kotlin.reflect.full.functions
import kotlin.reflect.jvm.javaMethod
import kotlin.system.exitProcess
import kotlin.time.Duration
import kotlin.time.Duration.Companion.milliseconds

@Command()
class Generator : Callable<Int> {

    @Option(names = ["-a", "--arcadia_root"])
    lateinit var arcadiaRoot: Path

    lateinit var apphostFolder: Path

    private val merger: Merger = Merger()
    private val printer: Printer = Printer()
    private val TRANSPARENT_NODE = TNode.newBuilder().setNodeType(Graph.TNodeType.TRANSPARENT).build()
    private val DEFAULT_MONITORING_SETTINGS = Graph.TDefaultNodeAlerts.newBuilder()
        .addAlerts(Monitoring.TAlert.newBuilder().apply {
            crit = 0.1
            operation = Monitoring.TOperationType.perc
            prior = 10
            type = Monitoring.TAlertType.failures
            warn = 0.09
        }.build())
        .build()

    override fun call(): Int {
        apphostFolder = arcadiaRoot.resolve("apphost/conf/verticals/ALICE")
        val app = SpringApplication(KronstadtApplication::class.java)
        app.webApplicationType = WebApplicationType.NONE
        app.setAdditionalProfiles("ut")
        app.setDefaultProperties(
            mapOf(
                "tvm.mode" to "disabled",
                "tests.it2" to true
            )
        )
        val context = app.run()
        val scenarios = context.getBeansOfType<IScenario<*>>()

        scenarios.values.filterNot { it.name == DUMMY_SCENARIO_META.name }.forEach { processScenario(it) }

        return 0
    }

    private fun processScenario(scenarioBean: IScenario<*>) {

        logger.info("Processing scenario ${scenarioBean.name}")

        val scenario = parseScenario(scenarioBean)
        val mmConfig: TScenarioConfig = parseMMConfig(scenario.mmName)

        if (mmConfig.handlers.requestType != ERequestType.AppHostPure &&
            !mmConfig.handlers.isTransferringToAppHostPure
        ) {
            logger.info("Scenario ${scenario.name} doesn't use apphost. Skipping...")
            return
        }

        val scenarioGraphPrefix = mmConfig.handlers.graphsPrefix.ifEmpty { scenario.name }

        val runFileName = "${scenarioGraphPrefix}_run.json"
        val runGraphFile = File(apphostFolder.toFile(), runFileName)

        logger.info("Generating run graph: $runFileName")
        val baseGraph = TGraphWrapper.newBuilder()
            .setMonitoring(DEFAULT_MONITORING_SETTINGS)
            .setSettings(createRunGraphNodes(mmConfig, scenario))
            .build()

        val targetGraph = merger.mergeGraph(runGraphFile, baseGraph)

        printer.printGraph(runGraphFile, targetGraph)

        if (scenario.scenes.any { it.applying }) {
            val fileName = "${scenarioGraphPrefix}_apply.json"
            logger.info("Generating apply graph: $fileName")
            val graphFile = File(apphostFolder.toFile(), fileName)
            val baseApplyGraph = TGraphWrapper.newBuilder()
                .setMonitoring(DEFAULT_MONITORING_SETTINGS)
                .setSettings(createApplyGraphNodes(mmConfig, scenario))
                .build()
            val targetApplyGraph = merger.mergeGraph(graphFile, baseApplyGraph)
            printer.printGraph(graphFile, targetApplyGraph)
        }

        if (scenario.scenes.any { it.continuing }) {
            val fileName = "${scenarioGraphPrefix}_continue.json"
            logger.info("Generating continue graph: $fileName")
            val graphFile = File(apphostFolder.toFile(), fileName)
            val baseApplyGraph = TGraphWrapper.newBuilder()
                .setMonitoring(DEFAULT_MONITORING_SETTINGS)
                .setSettings(createContinueGraphNodes(mmConfig, scenario))
                .build()
            val targetApplyGraph = merger.mergeGraph(graphFile, baseApplyGraph)
            printer.printGraph(graphFile, targetApplyGraph)
        }

        if (scenario.scenes.any { it.committing }) {
            val fileName = "${scenarioGraphPrefix}_commit.json"
            logger.info("Generating commit graph: $fileName")
            val graphFile = File(apphostFolder.toFile(), fileName)
            val baseApplyGraph = TGraphWrapper.newBuilder()
                .setMonitoring(DEFAULT_MONITORING_SETTINGS)
                .setSettings(createCommitGraphNodes(mmConfig, scenario))
                .build()
            val targetApplyGraph = merger.mergeGraph(graphFile, baseApplyGraph)
            printer.printGraph(graphFile, targetApplyGraph)
        }

        scenario.grpcHandlers.forEach { handler ->
            val fileName = "${scenarioGraphPrefix}_${handler.path}.json"
            logger.info("Generating ${handler.path} GRPC handler graph: ${fileName}")
            val graphFile = File(apphostFolder.toFile(), fileName)
            val baseApplyGraph = TGraphWrapper.newBuilder()
                .setMonitoring(DEFAULT_MONITORING_SETTINGS)
                .setSettings(createGrpcHandlerGraphNode(scenario, mmConfig, handler))
                .build()
            val targetApplyGraph = merger.mergeGraph(graphFile, baseApplyGraph)
            printer.printGraph(graphFile, targetApplyGraph)
        }
    }

    private fun parseMMConfig(name: String): TScenarioConfig {

        val scenarioConfigs = arcadiaRoot.resolve("alice/megamind/configs/dev/scenarios")
        val builder = TScenarioConfig.newBuilder()
        File(scenarioConfigs.toFile(), "${name}.pb.txt").reader().use {
            TextFormat.getParser().merge(it, builder)
        }
        return builder.build()
    }

    private fun createRunGraphNodes(
        mmConfig: TScenarioConfig,
        scenario: Scenario
    ): Graph.TGraph {
        val upperName = scenario.name.uppercase()
        return Graph.TGraph.newBuilder().apply {
            responsibles = createResponsibles(mmConfig)
            addAllInputDeps(
                listOfNotNull(
                    "DATASOURCES".takeIf { mmConfig.dataSourcesCount > 0 },
                    "WALKER_RUN_STAGE0"
                )
            )

            if (scenario.setupDispatch) {
                putNodes(
                    "${upperName}__PRE_SELECT_SCENE",
                    createDialogovoNode(
                        upperName,
                        path = "/kronstadt/scenario/${scenario.pathName}/run/pre_select"
                    )
                )
                putNodeDeps(
                    "${upperName}__PRE_SELECT_SCENE",
                    inputDeps("!WALKER_RUN_STAGE0",
                        "DATASOURCES".takeIf { mmConfig.dataSourcesCount > 0 })
                )
            }


            putNodeDeps(
                "${upperName}__SELECT_SCENE",
                inputDeps(
                    "DATASOURCES".takeIf { mmConfig.dataSourcesCount > 0 },
                    "!WALKER_RUN_STAGE0",
                    "${upperName}__PRE_SELECT_SCENE".takeIf { scenario.setupDispatch }
                )
            )

            putNodeDeps(
                "${upperName}__COMMON_RUN_RENDER",
                inputDeps(
                    "DATASOURCES".takeIf { mmConfig.dataSourcesCount > 0 },
                    "!WALKER_RUN_STAGE0",
                    "${upperName}__SELECT_SCENE@!kronstadt_selected_scene_run"
                )
            )

            putNodes(
                "${upperName}__SELECT_SCENE",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/run/select_scene"
                )
            )

            putNodes(
                "${upperName}__COMMON_RUN_RENDER",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/run/common"
                )
            )

            plugInDivRenderer(scenario, "${upperName}__COMMON_RUN_RENDER")

            addOutputDeps("RESPONSE")

        }.build()
    }

    private fun createApplyGraphNodes(mmConfig: TScenarioConfig, scenario: Scenario): Graph.TGraph {
        val upperName = scenario.name.uppercase()
        return Graph.TGraph.newBuilder().apply {
            responsibles = createResponsibles(mmConfig)
            addAllInputDeps(
                listOfNotNull(
                    "WALKER_APPLY_PREPARE"
                    // datasources will be supported in https://st.yandex-team.ru/MEGAMIND-3483
                )
            )
            putNodes(
                "${upperName}__APPLY_SETUP",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/apply/setup",
                    softTimeout = 50.milliseconds,
                    timeout = 250.milliseconds,
                )
            )
            putNodeDeps(
                "${upperName}__APPLY_SETUP",
                inputDeps(
                    "!WALKER_APPLY_PREPARE"
                )
            )

            putNodes(
                "${upperName}__APPLY_RENDER",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/apply/common",
                    softTimeout = 50.milliseconds,
                    timeout = 250.milliseconds,
                )
            )
            putNodeDeps(
                "${upperName}__APPLY_RENDER",
                inputDeps(
                    "!WALKER_APPLY_PREPARE",
                    "${upperName}__APPLY_SETUP"
                )
            )

            plugInDivRenderer(scenario, "${upperName}__APPLY_RENDER")

            addOutputDeps("RESPONSE")

        }.build()
    }

    private fun createContinueGraphNodes(mmConfig: TScenarioConfig, scenario: Scenario): Graph.TGraph {
        val upperName = scenario.name.uppercase()
        return Graph.TGraph.newBuilder().apply {
            responsibles = createResponsibles(mmConfig)
            addAllInputDeps(
                listOfNotNull(
                    "WALKER_POST_CLASSIFY"
                    // datasources will be supported in https://st.yandex-team.ru/MEGAMIND-3483
                )
            )
            putNodes(
                "${upperName}__CONTINUE_SETUP",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/continue/setup",
                    softTimeout = 50.milliseconds,
                    timeout = 250.milliseconds,
                )
            )
            putNodeDeps(
                "${upperName}__CONTINUE_SETUP",
                inputDeps(
                    "!WALKER_POST_CLASSIFY"
                )
            )

            putNodes(
                "${upperName}__CONTINUE_RENDER",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/continue/common",
                    softTimeout = 50.milliseconds,
                    timeout = 250.milliseconds,
                )
            )
            putNodeDeps(
                "${upperName}__CONTINUE_RENDER",
                inputDeps(
                    "!WALKER_POST_CLASSIFY",
                    "${upperName}__CONTINUE_SETUP"
                )
            )

            plugInDivRenderer(scenario, "${upperName}__CONTINUE_RENDER")

            addOutputDeps("RESPONSE")

        }.build()
    }

    private fun createCommitGraphNodes(mmConfig: TScenarioConfig, scenario: Scenario): Graph.TGraph {
        val upperName = scenario.name.uppercase()
        return Graph.TGraph.newBuilder().apply {
            responsibles = createResponsibles(mmConfig)
            addAllInputDeps(
                listOfNotNull(
                    "WALKER_APPLY_PREPARE"
                    // datasources will be supported in https://st.yandex-team.ru/MEGAMIND-3483
                )
            )
            putNodes(
                "${upperName}__COMMIT_SETUP",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/commit/setup",
                    softTimeout = 50.milliseconds,
                    timeout = 250.milliseconds,
                )
            )
            putNodeDeps(
                "${upperName}__COMMIT_SETUP",
                inputDeps(
                    "!WALKER_APPLY_PREPARE"
                )
            )

            putNodes(
                "${upperName}__COMMIT_RENDER",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/commit/common",
                    softTimeout = 50.milliseconds,
                    timeout = 250.milliseconds,
                )
            )
            putNodeDeps(
                "${upperName}__COMMIT_RENDER",
                inputDeps(
                    "!WALKER_APPLY_PREPARE",
                    "${upperName}__COMMIT_SETUP"
                )
            )

            plugInDivRenderer(scenario, "${upperName}__COMMIT_RENDER")

            addOutputDeps("RESPONSE")

        }.build()
    }

    private val DISPATCHER_DEPS =
        "DISPATCHER@!mm_scenario_request_meta,rpc_request,request,metadata,context_load_response"

    private fun createGrpcHandlerGraphNode(
        scenario: Scenario,
        mmConfig: TScenarioConfig,
        grpcHandler: GrpcHandler
    ): Graph.TGraph {
        val upperName = grpcHandler.path.uppercase()
        return Graph.TGraph.newBuilder().apply {
            responsibles = createResponsibles(mmConfig)
            addAllInputDeps(
                listOfNotNull(
                    //"DATASOURCES".takeIf { mmConfig.dataSourcesCount > 0 },
                    "DISPATCHER"
                )
            )

            putNodes(
                "SETUP",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/grpc/${grpcHandler.path}/setup",
                    additionalAliases = listOf(scenario.name.uppercase()),
                )
            )
            putNodeDeps(
                "SETUP",
                inputDeps(
                    DISPATCHER_DEPS,
                    //"DATASOURCES".takeIf { mmConfig.dataSourcesCount > 0 },
                )
            )

            grpcHandler.additionalPaths.forEach { additionalPath ->
                val nodeName = additionalPath.uppercase()
                putNodes(
                    nodeName,
                    createDialogovoNode(
                        upperName,
                        path = "/kronstadt/scenario/${scenario.pathName}/grpc/${grpcHandler.path}/${additionalPath}",
                        additionalAliases = listOf(scenario.name.uppercase()),
                    )
                )

                putNodeDeps(
                    nodeName,
                    inputDeps(
                        DISPATCHER_DEPS,
                        "!SETUP"
                    )
                )

                putEdgeExpressions("SETUP->${nodeName}", "!SETUP[error]")
            }

            putNodes(
                "PROCESS",
                createDialogovoNode(
                    upperName,
                    path = "/kronstadt/scenario/${scenario.pathName}/grpc/${grpcHandler.path}/process",
                    additionalAliases = listOf(scenario.name.uppercase()),
                )
            )
            putNodeDeps(
                "PROCESS",
                inputDeps(
                    DISPATCHER_DEPS,
                    //"DATASOURCES".takeIf { mmConfig.dataSourcesCount > 0 },
                    "!SETUP",
                    *grpcHandler.additionalPaths.map { it.uppercase() }.toTypedArray(),
                )
            )

            putEdgeExpressions("SETUP->PROCESS", "!SETUP[error]")

            addOutputDeps("RESPONSE")

            putNodeDeps(
                "RESPONSE",
                inputDeps(
                    "PROCESS@response,error",
                    "SETUP@error",
                )
            )

        }.build()
    }

    private fun sceneFlag(scenario: Scenario, scene: Scene) = "kronstadt#${scenario.flagName}#${scene.name}"

    private fun Graph.TGraph.Builder.plugInDivRenderer(
        scenario: Scenario,
        finalNodeName: String
    ) {
        if (scenario.useDivRenderer) {
            // plug in DIV_RENDERER

            putNodes("DIV_RENDER", createDivRenderNode())

            putNodeDeps(
                "DIV_RENDER",
                inputDeps("!${finalNodeName}->SCENARIO_RESPONSE@!render_data,!mm_scenario_response"),
            )
            putNodeDeps(
                "RESPONSE",
                inputDeps(
                    "DIV_RENDER@mm_scenario_response",
                    "${finalNodeName}@mm_scenario_response"
                )
            )

            putEdgeExpressions(
                "${finalNodeName}->DIV_RENDER",
                "${finalNodeName}[render_data]"
            )
            putEdgeExpressions(
                "${finalNodeName}->RESPONSE",
                "!${finalNodeName}[render_data]"
            )
        } else {
            putNodeDeps(
                "RESPONSE",
                inputDeps(
                    "${finalNodeName}@mm_scenario_response"
                )
            )
        }
    }

    private fun createDivRenderNode(): TNode {

        return TNode.newBuilder().apply {
            backendName = StringValue.of("SELF")
            nodeType = Graph.TNodeType.DEFAULT
            paramsBuilder.apply {
                attemptsBuilder.maxAttempts = UInt32Value.of(2)
                handler = StringValue.of("/_subhost/div_renderer")
                softTimeout = StringValue.of(50.milliseconds.toString())
                timeout = StringValue.of(100.milliseconds.toString())
            }

        }.build()
    }

    private fun createDialogovoNode(
        upperName: String,
        path: String,
        softTimeout: Duration = 30.milliseconds,
        timeout: Duration = 100.milliseconds,
        additionalAliases: List<String> = listOf(),
    ): TNode {
        val nodeBuilder = TNode.newBuilder().apply {
            aliasConfigBuilder.addAddrAlias(upperName)
            if (additionalAliases.isNotEmpty()) {
                aliasConfigBuilder.addAllAddrAlias(additionalAliases)
            }
            aliasConfigBuilder.addAddrAlias("KRONSTADT_ALL")

            backendName = StringValue.of("ALICE_DIALOGOVO")
            nodeType = Graph.TNodeType.DEFAULT

            paramsBuilder.apply {
                attemptsBuilder.maxAttempts = UInt32Value.of(2)

                handler = StringValue.of(path)

                setSoftTimeout(StringValue.of(softTimeout.toString()))
                setTimeout(StringValue.of(timeout.toString()))
            }
        }
        return nodeBuilder.build()
    }

    private fun createResponsibles(mmConfig: TScenarioConfig): Shared.TResponsibles {
        val builder = Shared.TResponsibles.newBuilder()
        mmConfig.responsibles.abcServicesList.forEach { service ->
            builder.addAbcService(createAbcResponsibles(service))
        }
        builder.addAllLogins(mmConfig.responsibles.loginsList)
        builder.addMessengerChatNames("MegamindDuty")
        return builder.build()
    }

    private fun createAbcResponsibles(service: TAbcService): Shared.TABCServiceInfo =
        Shared.TABCServiceInfo.newBuilder().apply {
            slug = service.name
            if (service.dutySlugsList.isNotEmpty()) {
                addAllDutySlugs(service.dutySlugsList)
            }
            if (service.scopesList.isNotEmpty()) {
                addAllRoleScopes(service.scopesList)
            }
        }.build()

    private fun inputDeps(vararg item: String?): Graph.TDependencies = inputDeps(item.filterNotNull().toList())

    private fun inputDeps(items: List<String>) = Graph.TDependencies.newBuilder()
        .addAllInputDeps(items)
        .build()

    private fun parseScenario(scenario: IScenario<*>): Scenario {
        val isPrepareSelectSceneDefined: Boolean =
            AbstractScenario::class.java != (scenario::class.functions
                .first { it.name == "prepareSelectScene" }
                .javaMethod!!
                .declaringClass)

        val scenes = scenario.scenes.map { scene ->
            Scene(
                name = scene.name,
                setupRun = scene is SceneWithPrepare<*, *>,
                applying = scene is ApplyingScene<*, *, *>,
                setupApply = scene is ApplyingSceneWithPrepare<*, *, *>,
                continuing = scene is ContinuingScene<*, *, *>,
                setupContinue = scene is ContinuingSceneWithPrepare<*, *, *>,
                committing = scene is CommittingScene<*, *, *>,
                setupCommit = scene is CommittingSceneWithPrepare<*, *, *>,
            )
        }

        val grpcHandlers =
            scenario.grpcHandlers.map { handler ->
                GrpcHandler(
                    path = handler.path,
                    additionalPaths = handler.additionalHandlers.keys
                )
            }
        val meta = scenario.scenarioMeta
        return Scenario(
            name = meta.name,
            mmName = meta.megamindName,
            pathName = meta.mmPath ?: meta.name,
            setupDispatch = isPrepareSelectSceneDefined,
            scenes = scenes,
            useDivRenderer = meta.useDivRenderer,
            grpcHandlers = grpcHandlers,
        )
    }

    companion object {
        private val logger = LogManager.getLogger(Generator::class.java)
    }
}

internal data class Scene(
    val name: String,
    val setupRun: Boolean,
    val applying: Boolean,
    val setupApply: Boolean,
    val continuing: Boolean,
    val setupContinue: Boolean,
    val committing: Boolean,
    val setupCommit: Boolean,
)

internal data class Scenario(
    val name: String,
    val mmName: String,
    val pathName: String,
    val setupDispatch: Boolean,
    val scenes: List<Scene>,
    val useDivRenderer: Boolean,
    val grpcHandlers: List<GrpcHandler>,
) {
    val flagName = name.lowercase()
}

internal data class GrpcHandler(
    val path: String,
    val additionalPaths: Set<String>
)

fun main(args: Array<String>): Unit = exitProcess(CommandLine(Generator()).execute(*args))
