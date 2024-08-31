package ru.yandex.alice.paskills.common.executor

import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.concurrent.BlockingQueue
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.ThreadFactory
import java.util.concurrent.ThreadPoolExecutor
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

private val DEFAULT_HANDLER = ThreadPoolExecutor.AbortPolicy()

object InstrumentedExecutorFactory {
    fun fixedThreadPool(
        name: String,
        threads: Int,
        queueCapacity: Int?,
        threadFactory: ThreadFactory = prefixedThreadFactory("${name}-"),
        metricRegistry: MetricRegistry,
    ): InstrumentedExecutorService {
        val namedSensorsRegistry = NamedSensorsRegistry(
            metricRegistry.subRegistry("pool", name),
            "thread"
        )
        val queue: BlockingQueue<Runnable> = if (queueCapacity != null)
            LinkedBlockingQueue(queueCapacity) else LinkedBlockingQueue()
        return InstrumentedExecutorService(
            namedSensorsRegistry,
            PoolConfig(threads, threads, 0L, TimeUnit.MILLISECONDS),
            queue,
            threadFactory,
            DEFAULT_HANDLER,
            name,
        )
    }

    fun prefixedThreadFactory(name: String): ThreadFactory = ThreadNameIndexThreadFactory(name)

    private class ThreadNameIndexThreadFactory(private val base: String) : ThreadFactory {
        private val nextSuffix: AtomicInteger = AtomicInteger(0)

        override fun newThread(r: Runnable): Thread = Thread(r, base + nextSuffix.getAndIncrement())
    }
}
