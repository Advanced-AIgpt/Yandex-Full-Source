package ru.yandex.alice.memento.solomon

import org.apache.logging.log4j.LogManager
import org.eclipse.jetty.io.ConnectionStatistics
import org.eclipse.jetty.server.Server
import org.eclipse.jetty.server.ServerConnector
import org.eclipse.jetty.util.component.Container
import org.eclipse.jetty.util.thread.QueuedThreadPool
import org.eclipse.jetty.util.thread.ThreadPool
import org.eclipse.jetty.util.thread.ThreadPool.SizedThreadPool
import org.springframework.boot.web.embedded.jetty.JettyServerCustomizer
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry
import ru.yandex.monlib.metrics.JvmGc
import ru.yandex.monlib.metrics.JvmMemory
import ru.yandex.monlib.metrics.JvmRuntime
import ru.yandex.monlib.metrics.JvmThreads
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.Objects
import java.util.function.DoubleSupplier
import java.util.function.LongSupplier

@Configuration
internal open class SensorsRegistryConfiguration {

    private val logger = LogManager.getLogger()

    @Bean
    open fun metricRegistry(): MetricRegistry {
        val registry = MetricRegistry()
        JvmGc.addMetrics(registry)
        JvmMemory.addMetrics(registry)
        JvmRuntime.addMetrics(registry)
        JvmThreads.addMetrics(registry)
        return registry
    }

    @Bean
    open fun jettyThreadPoolMetricsCustomizer(registry: MetricRegistry): JettyServerCustomizer {
        logger.info("Runtime availableProcessors: ${Runtime.getRuntime().availableProcessors()}")
        return JettyServerCustomizer { server: Server ->
            addJettyExecutorMetrics(registry.subRegistry("threadPool", "jettyExecutor"), server.threadPool)
        }
    }

    private fun addJettyExecutorMetrics(registry: MetricRegistry, pool: ThreadPool) {

        if (pool is SizedThreadPool) {
            registry.lazyGaugeInt64("jvm.threadPool.minSize") { pool.minThreads.toLong() }
            registry.lazyGaugeInt64("jvm.threadPool.maxSize") { pool.maxThreads.toLong() }
            if (pool is QueuedThreadPool) {
                registry.lazyGaugeInt64("jvm.threadPool.activeThreads") { pool.busyThreads.toLong() }
                registry.lazyGaugeInt64("jvm.threadPool.queuedTasks") { pool.queueSize.toLong() }
            }
        }
        registry.lazyGaugeInt64("jvm.threadPool.size") { pool.threads.toLong() }
        registry.lazyGaugeInt64("jvm.threadPool.idle") { pool.idleThreads.toLong() }
    }

    @Bean
    open fun jettyConnectionMetricsCustomizer(registry: MetricRegistry): JettyServerCustomizer {
        return SolomonMonitoringJettyServerCustomizer(registry)
    }

    private inner class SolomonMonitoringJettyServerCustomizer(delegate: MetricRegistry) : JettyServerCustomizer {
        private val registry: NamedSensorsRegistry = NamedSensorsRegistry(delegate, "jetty.connections")

        override fun customize(server: Server) {
            // {@see ServerConnectionStatistics}
            for (connector in server.connectors) {
                logger.info("Connector: ${connector.name}")
                if (connector is ServerConnector) {
                    logger.info("Selectors count: ${connector.selectorManager.selectorCount}")
                    logger.info("Acceptors count: ${connector.acceptors}")
                }
                if (connector is Container) {
                    val statistics = ConnectionStatistics()
                    val subRegistry = registry.withLabels(
                        Labels.of("connector", Objects.requireNonNullElse(connector.name, "default"))
                    )
                    subRegistry.lazyCounter("received_bytes") { statistics.receivedBytes }
                    subRegistry.lazyCounter("sent_bytes") { statistics.sentBytes }
                    subRegistry.lazyGauge("connection_duration_max", LongSupplier { statistics.connectionDurationMax })
                    subRegistry.lazyGauge(
                        "connection_duration_mean",
                        DoubleSupplier { statistics.connectionDurationMean })
                    subRegistry.lazyGauge(
                        "connection_duration_std_dev",
                        DoubleSupplier { statistics.connectionDurationStdDev })
                    subRegistry.lazyGauge("open_connections_count", LongSupplier { statistics.connections })
                    subRegistry.lazyCounter("received_messages") { statistics.receivedMessages }
                    subRegistry.lazyCounter("sent_messages") { statistics.sentMessages }
                    connector.addBean(statistics)
                }
            }
        }
    }
}
