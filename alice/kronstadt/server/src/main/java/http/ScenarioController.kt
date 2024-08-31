package ru.yandex.alice.kronstadt.server.http

import com.google.protobuf.InvalidProtocolBufferException
import com.google.protobuf.MessageOrBuilder
import com.google.protobuf.util.JsonFormat
import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpHeaders
import org.springframework.web.bind.annotation.PathVariable
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestAttribute
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RequestHeader
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.kronstadt.core.scenario.IScenario
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmRequired
import java.util.function.Function

@RestController
@TvmRequired("megamind")
open class ScenarioController(
    private val printer: JsonFormat.Printer,
    private val httpAdapter: ScenarioHttpAdapter,
    scenarios: List<IScenario<*>>,
) {
    private val logger = LogManager.getLogger()

    private val scenarioMap = scenarios
        .flatMap { scenario -> scenario.scenarioMeta.alternativePaths.map { path -> path to scenario } }
        .toMap() + scenarios.associateBy { it.scenarioMeta.mmPath ?: it.scenarioMeta.name }

    @PostMapping(value = ["/megamind/{scenario}/run", "/{scenario}/run"])
    fun run(
        @PathVariable("scenario") scenarioName: String,
        @RequestBody req: RequestProto.TScenarioRunRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioRunResponse {
        val scenario = findScenario(scenarioName)
        return withLogging("Run", contentType, accept, req) { httpAdapter.processRun(scenario, it, uid) }
    }

    @PostMapping(value = ["/megamind/{scenario}/apply", "/{scenario}/apply"])
    fun apply(
        @PathVariable("scenario") scenarioName: String,
        @RequestBody req: RequestProto.TScenarioApplyRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioApplyResponse {
        val scenario = findScenario(scenarioName)
        return withLogging("Apply", contentType, accept, req) { httpAdapter.processApply(scenario, it, uid) }
    }

    @PostMapping(value = ["/megamind/{scenario}/commit", "/{scenario}/commit"])
    fun commit(
        @PathVariable("scenario") scenarioName: String,
        @RequestBody req: RequestProto.TScenarioApplyRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioCommitResponse {
        val scenario = findScenario(scenarioName)
        return withLogging("Commit", contentType, accept, req) { httpAdapter.processCommit(scenario, it, uid) }
    }

    @PostMapping(value = ["/megamind/{scenario}/continue", "/{scenario}/continue"])
    fun continueRequest(
        @PathVariable("scenario") scenarioName: String,
        @RequestBody req: RequestProto.TScenarioApplyRequest,
        @RequestHeader(value = HttpHeaders.CONTENT_TYPE, required = false) contentType: String,
        @RequestHeader(value = HttpHeaders.ACCEPT, required = false) accept: String,
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: String?,
    ): ResponseProto.TScenarioContinueResponse {
        val scenario = findScenario(scenarioName)
        return withLogging("Continue", contentType, accept, req) { httpAdapter.processContinue(scenario, it, uid) }
    }

    private fun findScenario(scenarioName: String) =
        scenarioMap[scenarioName] ?: throw RuntimeException("Can't find scenario by path $scenarioName")

    private fun <TRequest : MessageOrBuilder, TResponse : MessageOrBuilder> withLogging(
        method: String,
        contentType: String,
        accept: String,
        req: TRequest,
        sup: Function<TRequest, TResponse>
    ): TResponse {
        logger.debug("$method Request ({} - {}): {}", { contentType }, { accept }, { toString(req) })

        val resp = sup.apply(req)

        logger.debug("$method Response: {}", { toString(resp) })
        return resp
    }

    private fun toString(msg: MessageOrBuilder): String {
        return try {
            printer.print(msg)
        } catch (e: InvalidProtocolBufferException) {
            logger.error(e)
            "ERROR"
        }
    }
}
