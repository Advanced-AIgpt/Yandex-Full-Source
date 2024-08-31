package ru.yandex.alice.kronstadt.server.http

import com.google.common.collect.Maps
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.VersionProvider
import ru.yandex.alice.kronstadt.core.scenario.IScenario
import ru.yandex.alice.kronstadt.server.SELECTED_SCENE_SETRACE_MARKER
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioApplyResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioCommitResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioContinueResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry

@Component
class ScenarioHttpAdapter(
    private val metricRegistry: MetricRegistry,
    private val versionProvider: VersionProvider,
) {

    private val logger = LogManager.getLogger()

    fun <State> processRun(
        scenario: IScenario<State>,
        req: RequestProto.TScenarioRunRequest,
        uid: String?
    ): TScenarioRunResponse = monitor(scenario.name, stage = "run").measure {

        try {
            val request: MegaMindRequest<State> = scenario.convertRunRequest(req, uid)
            logger.debug("Got run request: {}", request)

            val selectedScene = scenario.doSelectScene(request)
            logger.info(
                SELECTED_SCENE_SETRACE_MARKER,
                "Scenario {} [run:select_scene] phase resulted with selected_scene={}",
                scenario.name,
                selectedScene?.name
            )

            return@measure scenario.renderRunResponse(request, selectedScene).scenarioResponse
        } catch (ex: Exception) {
            return@measure TScenarioRunResponse.newBuilder()
                .setVersion(versionProvider.version)
                .setError(
                    ResponseProto.TScenarioError.newBuilder()
                        .setMessage(ex.message)
                        .setType(ex.javaClass.name)
                )
                .build()
        }

    }

    fun <State> processApply(
        scenario: IScenario<State>,
        req: RequestProto.TScenarioApplyRequest,
        uid: String?
    ): TScenarioApplyResponse = monitor(scenario.name, stage = "apply").measure {
        try {
            val (request, chosenScene) = scenario.convertApplyRequest(req, uid)
            logger.debug("Got apply request: {}", request)

            return@measure scenario.renderApplyResponse(request, chosenScene).scenarioResponse
        } catch (ex: Exception) {
            return@measure TScenarioApplyResponse.newBuilder()
                .setVersion(versionProvider.version)
                .setError(
                    ResponseProto.TScenarioError.newBuilder()
                        .setMessage(ex.message)
                        .setType(ex.javaClass.name)
                )
                .build()
        }
    }

    fun <State> processCommit(
        scenario: IScenario<State>,
        req: RequestProto.TScenarioApplyRequest,
        uid: String?
    ): TScenarioCommitResponse = monitor(scenario.name, stage = "commit").measure {
        try {
            val (request, chosenScene) = scenario.convertCommitRequest(req, uid)
            logger.debug("Got commit request: {}", request)

            return@measure scenario.renderCommitResponse(request, chosenScene)
        } catch (ex: Exception) {
            return@measure TScenarioCommitResponse.newBuilder()
                .setVersion(versionProvider.version)
                .setError(
                    ResponseProto.TScenarioError.newBuilder()
                        .setMessage(ex.message)
                        .setType(ex.javaClass.name)
                )
                .build()
        }
    }

    fun <State> processContinue(
        scenario: IScenario<State>,
        req: RequestProto.TScenarioApplyRequest,
        uid: String?
    ): TScenarioContinueResponse = monitor(scenario.name, stage = "continue").measure {
        try {
            val (request, chosenScene) = scenario.convertContinueRequest(req, uid)
            logger.debug("Got continue request: {}", request)

            return@measure scenario.renderContinueResponse(request, chosenScene).scenarioResponse
        } catch (ex: Exception) {
            return@measure TScenarioContinueResponse.newBuilder()
                .setVersion(versionProvider.version)
                .setError(
                    ResponseProto.TScenarioError.newBuilder()
                        .setMessage(ex.message)
                        .setType(ex.javaClass.name)
                )
                .build()
        }
    }

    private data class Phase(val scenarioName: String, val stage: String)

    private val monitors = Maps.newConcurrentMap<Phase, PhaseMonitor>()
    private fun monitor(scenarioName: String, stage: String): PhaseMonitor {
        val phase = Phase(scenarioName, stage)
        return monitors.computeIfAbsent(phase) { createPhaseMonitor(it) }
    }

    private class PhaseMonitor(val requestRate: Rate, val failureRate: Rate) {
        inline fun <T> measure(body: () -> T): T {
            requestRate.inc()
            try {
                return body.invoke()
            } catch (e: Exception) {
                failureRate.inc()
                throw e
            }
        }
    }

    private fun createPhaseMonitor(phase: Phase): PhaseMonitor {
        val registry = metricRegistry.subRegistry(Labels.of("scenario", phase.scenarioName))
        return PhaseMonitor(
            requestRate = registry.rate("scenario_request", Labels.of("stage", phase.stage)),
            failureRate = registry.rate("scenario_request_failure", Labels.of("stage", phase.stage))
        )
    }
}
