package ru.yandex.alice.social.sharing.document

import com.yandex.ydb.core.auth.AuthProvider
import com.yandex.ydb.core.auth.TokenAuthProvider
import com.yandex.ydb.core.grpc.GrpcTransport
import com.yandex.ydb.table.SessionRetryContext
import com.yandex.ydb.table.TableClient
import com.yandex.ydb.table.rpc.grpc.GrpcTableRpc
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskills.common.ydb.YdbClient
import ru.yandex.alice.paskills.common.ydb.YdbClientImpl
import ru.yandex.alice.paskills.common.ydb.listener.LoggingYDBQueryListener
import ru.yandex.alice.paskills.common.ydb.listener.MetricsYDBQueryListener
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.util.concurrent.Executor
import java.util.concurrent.Executors

@Configuration
open class YdbConfiguration {
    @Value("\${ydb.maxRetries}")
    private val maxRetries = 0

    @Value("\${ydb.endpoint}")
    private val endpoint: String? = null

    @Value("\${ydb.database}")
    private val database: String? = null

    @Value("\${ydb.sessionPoolSizeMin}")
    private val sessionPoolSizeMin = 0

    @Value("\${ydb.sessionPoolSizeMax}")
    private val sessionPoolSizeMax = 0

    @Value("\${ydb.queryCacheSize}")
    private val queryCacheSize = 0

    @Value("\${CORECOUNT:\${ydb.parallelism}}")
    private val parallelism: Int = 0

    @Value("\${ydb.token:}")
    private val token: String? = null

    @Value("\${ydb.sessionTimeout}")
    private val sessionTimeout: Long = 100

    @Value("\${ydb.clientTimeout}")
    private val clientTimeout: Long = 200

    // Cross-DC YDB cluster
    private val ydbTvmClientId = 2002490

    @Bean
    open fun transport(): GrpcTransport {
//        val provider: AuthProvider = if (!Strings.isNullOrEmpty(token)) {
//            logger.info("ydb config - use token auth provider")
//            TokenAuthProvider(token)
//        } else {
//            logger.info("ydb config - use TVM auth provider")
//            AuthProvider { tvmClient.getServiceTicketFor(ydbTvmClientId) }
//        }
        val provider: AuthProvider = TokenAuthProvider(token)
        return GrpcTransport.forEndpoint(endpoint, database)
            .withAuthProvider(provider)
            .build()
    }

    @Bean
    open fun tableClient(transport: GrpcTransport, registry: MetricRegistry): TableClient {
        val tableClient = TableClient.newClient(GrpcTableRpc.ownTransport(transport))
            .sessionPoolSize(sessionPoolSizeMin, sessionPoolSizeMax)
            .queryCacheSize(queryCacheSize)
            .build()
        val subRegistry = registry.subRegistry("bean", "table_client")
        subRegistry.lazyGaugeInt64("ydb.session_pool.idle_count") { tableClient.sessionPoolStats.idleCount.toLong() }
        subRegistry.lazyGaugeInt64("ydb.session_pool.disconnected_count") { tableClient.sessionPoolStats.disconnectedCount.toLong() }
        subRegistry.lazyGaugeInt64("ydb.session_pool.acquired_count") { tableClient.sessionPoolStats.acquiredCount.toLong() }
        subRegistry.lazyGaugeInt64("ydb.session_pool.pending_acquire_count") { tableClient.sessionPoolStats.pendingAcquireCount.toLong() }
        return tableClient
    }

    @Bean
    open fun sessionRetryContext(tableClient: TableClient): SessionRetryContext {
        return SessionRetryContext.create(tableClient)
            .maxRetries(maxRetries)
            .sessionSupplyTimeout(Duration.ofMillis(sessionTimeout))
            .executor(ydbExecutor())
            .build()
    }

    @Bean
    open fun ydbClient(
        sessionRetryContext: SessionRetryContext,
        metricRegistry: MetricRegistry
    ): YdbClient {
        return YdbClientImpl(
            ctx = sessionRetryContext,
            listeners = listOf(
                LoggingYDBQueryListener(Duration.ofMillis(300)),
                MetricsYDBQueryListener(metricRegistry),
            ),
            poolSize = sessionPoolSizeMin,
            clientTimeout = Duration.ofMillis(clientTimeout),
        )
    }

    @Bean("ydbExecutor")
    open fun ydbExecutor(): Executor {
        return if (parallelism > 0) Executors.newWorkStealingPool(parallelism) else Executors.newWorkStealingPool()
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
