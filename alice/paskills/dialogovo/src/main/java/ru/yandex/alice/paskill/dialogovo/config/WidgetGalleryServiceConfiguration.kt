package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.widgets.WidgetGalleryServiceImpl
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory

@Configuration
open class WidgetGalleryServiceConfiguration {

    @Bean
    open fun widgetGalleryService(
        skillProcessor: SkillRequestProcessor,
        @Qualifier("widgetGalleryServiceExecutor") executorService: DialogovoInstrumentedExecutorService,
        widgetsConfig: WidgetsConfig
    ) = WidgetGalleryServiceImpl(
        skillProcessor,
        executorService,
        widgetsConfig
    )

    @Bean(value = ["widgetGalleryServiceExecutor"], destroyMethod = "shutdownNow")
    open fun widgetGalleryServiceExecutor(executorsFactory: ExecutorsFactory) =
        executorsFactory.cachedBoundedThreadPool(4, 100, 100, "widget-gallery-service")
}
