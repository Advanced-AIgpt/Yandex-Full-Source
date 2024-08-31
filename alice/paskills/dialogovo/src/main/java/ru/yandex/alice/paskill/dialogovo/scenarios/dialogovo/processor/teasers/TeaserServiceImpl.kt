package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.teasers

import org.slf4j.LoggerFactory
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.config.TeasersConfig
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.external.v1.request.TeasersRequest
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import java.time.Duration
import java.util.Optional
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit

class TeaserServiceImpl(
    private val skillProcessor: SkillRequestProcessor,
    private val executorService: DialogovoInstrumentedExecutorService,
    private val teasersConfig: TeasersConfig
) : TeaserService {
    override fun process(
        request: MegaMindRequest<DialogovoState>,
        skills: List<SkillInfo>,
        context: Context?,
    ): List<SkillProcessResult> {
        val futureResponses = skills.map { skillInfo ->
            executorService.supplyAsyncInstrumented(
                { skillProcessor.process(context, buildSkillProcessRequest(request, skillInfo)) },
                Duration.ofMillis(teasersConfig.skillRequestTimeout),
                {
                    logger.error("Timeout on request for teaser to skill ${skillInfo.id}")
                    null
                }
            ).exceptionally {
                logger.error("Request for teasers to skill ${skillInfo.id} failed with exception ${it.message}", it)
                null
            }
        }.toTypedArray()
        val result = CompletableFuture.allOf(*futureResponses)
            .orTimeout(teasersConfig.skillRequestTimeout, TimeUnit.MILLISECONDS)
        try {
            result.join()
        } catch (ex: Exception) {
            logger.error("Request for teasers for skills failed with exception ${ex.message}", ex)
            return listOf()
        }
        return futureResponses.mapNotNull {
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
            .teasersRequest(TeasersRequest())
            .build()
    }

    companion object {
        private val logger = LoggerFactory.getLogger(TeaserServiceImpl::class.java)
    }
}
