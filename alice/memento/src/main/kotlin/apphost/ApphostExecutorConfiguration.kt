package ru.yandex.alice.memento.apphost

import com.google.common.util.concurrent.ThreadFactoryBuilder
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.monlib.metrics.JvmThreads
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.concurrent.ExecutorService
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.ThreadPoolExecutor
import java.util.concurrent.TimeUnit

@Configuration
open class ApphostExecutorConfiguration(private val metricRegistry: MetricRegistry) {

    @Value("\${apphost.handlerThreads.min}")
    private val minThreads: Int = 30

    @Value("\${apphost.handlerThreads.max}")
    private val maxThreads: Int = 180

    @Value("\${apphost.handlerThreads.max-queue-capacity}")
    private val queueCapacity: Int = 128

    @Bean
    open fun apphostHandlerExecutor(): ExecutorService {
        val threadFactory = ThreadFactoryBuilder()
            .setDaemon(true)
            .setNameFormat("apphost-grpc-request-handler-%d")
            .build()
        val executor = ThreadPoolExecutor(
            minThreads, maxThreads,
            60L, TimeUnit.SECONDS,
            LinkedBlockingQueue(queueCapacity),
            threadFactory
        )

        JvmThreads.addExecutorMetrics("apphostHandlerExecutor", executor, metricRegistry)

        return executor
    }
}
