package ru.yandex.alice.paskill.dialogovo.controller;

import org.apache.logging.log4j.LogManager
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.megamind.AliceHandledWithDevConsoleMessageException
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor
import ru.yandex.alice.paskill.dialogovo.vins.ExternalDevError
import ru.yandex.alice.paskill.dialogovo.vins.RequestVinsSlot
import ru.yandex.alice.paskill.dialogovo.vins.VinsConverter
import ru.yandex.alice.paskill.dialogovo.vins.VinsRequest
import ru.yandex.alice.paskill.dialogovo.vins.VinsResponse
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookException
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmRequired

@RestController
@TvmRequired
class VinsController(
    private val vinsConverter: VinsConverter,
    private val requestProcessor: SkillRequestProcessor,
) {
    private val logger = LogManager.getLogger();

    @PostMapping("/vins")
    fun vins(@RequestBody request: VinsRequest): VinsResponse {
        logger.debug("Request: {}", request)

        val processReq: SkillProcessRequest = vinsConverter.convert(request)
        val ctx = Context(getSourceType(request))
        var result: SkillProcessResult
        var errors: List<ExternalDevError> = listOf()

        try {
            result = requestProcessor.process(ctx, processReq)
        } catch (ex: WebhookException) {
            result = SkillProcessResult.builder(ex.response, processReq.skill, ex.request.body).build()

            errors = ex.response.errors
                .map { ExternalDevError(it.code().code, it.code().description, it.path(), it.message()) }
        } catch (ex: AliceHandledException) {
            // тут мы не можем понять ходили мы в вебхук или нет
            // возможно стоит добавить ответ в AliceHandledException
            // или отказаться от исключений в этом месте
            result = SkillProcessResult.builder(null, processReq.skill, null).build()

            if (ex is AliceHandledWithDevConsoleMessageException) {
                errors = listOf(ExternalDevError("error", ex.externalDebugMessage))
            }
        } catch (ex: Exception) {
            logger.error("Vins controller detected not AliceHandledException. ", ex)
            throw ex
        }

        val response: VinsResponse
        try {
            response = vinsConverter.convert(request, result, errors)
        } catch (ex: Exception) {
            logger.error("VinsConverter Error. ", ex)
            throw ex
        }
        logger.debug("Response: {}", response);

        return response;
    }

    /**
     * Vins style source determination - remove after ping initialization migration to dialogovo
     */
    fun getSourceType(vinsRequest: VinsRequest): SourceType {
        val isPing = vinsRequest.form.slots.filterIsInstance<RequestVinsSlot>()
            .firstOrNull()
            ?.let { requestSlot -> requestSlot.name == "request" && requestSlot.value.asText() == "ping" } == true

        return if (isPing) SourceType.PING else SourceType.CONSOLE
    }
}
