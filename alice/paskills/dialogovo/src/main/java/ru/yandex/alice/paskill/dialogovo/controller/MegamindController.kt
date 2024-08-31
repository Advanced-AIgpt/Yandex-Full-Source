package ru.yandex.alice.paskill.dialogovo.controller

import org.springframework.http.HttpHeaders
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestAttribute
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RequestHeader
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.kronstadt.server.http.ScenarioController
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmRequired
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry

private const val SIGNAL = "megamind_controller_calls"
private const val DEFAULT_SCENARIO = "dialogovo"

@RestController
@TvmRequired("megamind")
@Deprecated("Controller left for migration purpose. Remove after elimination of \"megamind_controller_calls\" signal")
class MegamindController(
    private val scenarioController: ScenarioController,
    private val metricRegistry: MetricRegistry
) {

    @PostMapping(value = ["/megamind/run", "/run"])
    fun run(
        @RequestBody req: RequestProto.TScenarioRunRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioRunResponse {
        metricRegistry.rate(SIGNAL, Labels.of("method", "run"))
        return scenarioController.run(scenarioName = DEFAULT_SCENARIO, req, contentType, accept, uid)
    }

    @PostMapping(value = ["/megamind/apply", "/apply"])
    fun apply(
        @RequestBody req: RequestProto.TScenarioApplyRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioApplyResponse {
        metricRegistry.rate(SIGNAL, Labels.of("method", "apply"))
        return scenarioController.apply(DEFAULT_SCENARIO, req, contentType, accept, uid)
    }

    @PostMapping(value = ["/megamind/commit", "/commit"])
    fun commit(
        @RequestBody req: RequestProto.TScenarioApplyRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioCommitResponse {
        metricRegistry.rate(SIGNAL, Labels.of("method", "commit"))
        return scenarioController.commit(DEFAULT_SCENARIO, req, contentType, accept, uid)
    }

    @PostMapping(value = ["/megamind/continue", "/continue"])
    fun continueRequest(
        @RequestBody req: RequestProto.TScenarioApplyRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioContinueResponse {
        metricRegistry.rate(SIGNAL, Labels.of("method", "continue"))
        return scenarioController.continueRequest(DEFAULT_SCENARIO, req, contentType, accept, uid)
    }
}
