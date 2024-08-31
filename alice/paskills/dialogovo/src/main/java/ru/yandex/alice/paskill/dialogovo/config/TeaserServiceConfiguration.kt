package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.teasers.TeaserServiceImpl
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory

@Configuration
open class TeaserServiceConfiguration {

    @Bean
    open fun teaserService(
        skillProcessor: SkillRequestProcessor,
        @Qualifier("teaserServiceExecutor") executorService: DialogovoInstrumentedExecutorService,
        teasersConfig: TeasersConfig
    ) = TeaserServiceImpl(skillProcessor, executorService, teasersConfig)

    @Bean(value = ["teaserServiceExecutor"], destroyMethod = "shutdownNow")
    open fun teaserServiceExecutor(executorsFactory: ExecutorsFactory) =
        executorsFactory.cachedBoundedThreadPool(4, 100, 100, "teaser-service")
}
