package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.widgets

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.config.WidgetsConfig
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WidgetGalleryRequest
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import java.time.Duration
import java.util.Optional
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit

class WidgetGalleryServiceImpl(
    private val skillProcessor: SkillRequestProcessor,
    private val executorService: DialogovoInstrumentedExecutorService,
    private val widgetsConfig: WidgetsConfig
) : WidgetGalleryService {
    override fun process(
        request: MegaMindRequest<DialogovoState>,
        context: Context?,
        skills: List<SkillInfo>
    ): List<SkillProcessResult> {

        val futureList = skills.map { skillInfo ->
            val skillProcessRequest = buildSkillProcessRequest(request, skillInfo)
            executorService.supplyAsyncInstrumented(
                { skillProcessor.process(context, skillProcessRequest) },
                Duration.ofMillis(widgetsConfig.skillRequestTimeout),
                {
                    logger.error("Request for widget data for skill ${skillInfo.id} failed with timeout")
                    SkillProcessResult.builder(null, skillInfo, null).build()
                }
            ).exceptionally {
                logger.error(
                    "Request for widget data for skill ${skillInfo.id} failed with exception ${it.message}",
                    it
                )
                SkillProcessResult.builder(null, skillInfo, null).build()
            }
        }.toTypedArray()

        val future = CompletableFuture.allOf(*futureList)
            .orTimeout(widgetsConfig.skillRequestTimeout, TimeUnit.MILLISECONDS)
        try {
            future.join()
        } catch (ex: Exception) {
            logger.error("Request for widget data for skills failed with exception ${ex.message}", ex)
            return skills.map { SkillProcessResult.builder(null, it, null).build() }
        }

        return futureList.map {
            it.get()
        }
    }

    private fun buildSkillProcessRequest(
        request: MegaMindRequest<DialogovoState>,
        skillInfo: SkillInfo
    ): SkillProcessRequest {
        return SkillProcessRequest.builder()
            .clientInfo(request.clientInfo)
            .locationInfo(request.getLocationInfoO())
            .experiments(request.experiments)
            .skill(skillInfo)
            .testRequest(request.isTest())
            .mementoData(request.mementoData)
            .requestTime(request.serverTime)
            .viewState(Optional.empty())
            .widgetGalleryRequest(WidgetGalleryRequest())
            .build()
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
