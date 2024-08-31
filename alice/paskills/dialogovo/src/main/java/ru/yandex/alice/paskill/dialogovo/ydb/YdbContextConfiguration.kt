package ru.yandex.alice.paskill.dialogovo.ydb

import com.yandex.ydb.core.grpc.GrpcTransport
import com.yandex.ydb.table.SessionRetryContext
import com.yandex.ydb.table.TableClient
import com.yandex.ydb.table.rpc.grpc.GrpcTableRpc
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig
import ru.yandex.alice.paskill.dialogovo.config.YdbConfig
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory
import ru.yandex.alice.paskill.dialogovo.ydb.listener.YDBQueryListener
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.passport.tvmauth.TvmClient
import java.time.Duration
import java.util.concurrent.ExecutorService

@Configuration
internal open class YdbContextConfiguration(
    secretsConfig: SecretsConfig,
    val tvmClient: TvmClient,
    private val ydbConfig: YdbConfig,
) {
    private val endpoint: String = secretsConfig.ydbEndpoint
    private val database: String = secretsConfig.ydbDatabase

    private val minPoolSize = ydbConfig.sessionPoolSizeMin

    // Cross-DC YDB cluster
    private val ydbTvmClientId = 2002490

    @Bean
    open fun transport(): GrpcTransport {
        return GrpcTransport.forEndpoint(endpoint, database)
            .withAuthProvider { tvmClient.getServiceTicketFor(ydbTvmClientId) }
            .build()
    }

    @Bean
    open fun tableClient(transport: GrpcTransport, registry: MetricRegistry): TableClient {

        val tableClient = TableClient.newClient(GrpcTableRpc.ownTransport(transport))
            .sessionPoolSize(ydbConfig.sessionPoolSizeMin, ydbConfig.sessionPoolSizeMax)
            .queryCacheSize(ydbConfig.queryCacheSize)
            .build()

        val subRegistry = registry.subRegistry("bean", "table_client")
        subRegistry.lazyGaugeInt64("ydb.session_pool.idle_count") {
            tableClient.sessionPoolStats.idleCount.toLong()
        }
        subRegistry.lazyGaugeInt64("ydb.session_pool.disconnected_count") {
            tableClient.sessionPoolStats.disconnectedCount.toLong()
        }
        subRegistry.lazyGaugeInt64("ydb.session_pool.acquired_count") {
            tableClient.sessionPoolStats.acquiredCount.toLong()
        }
        subRegistry.lazyGaugeInt64("ydb.session_pool.pending_acquire_count") {
            tableClient.sessionPoolStats.pendingAcquireCount.toLong()
        }
        return tableClient
    }

    @Bean
    open fun sessionRetryContext(
        tableClient: TableClient,
        @Qualifier("ydbExecutorService") executor: ExecutorService
    ): SessionRetryContext {
        return SessionRetryContext.create(tableClient)
            .executor(executor)
            .maxRetries(ydbConfig.maxRetries)
            .sessionSupplyTimeout(Duration.ofSeconds(1))
            .build()
    }

    @Bean(name = ["ydbExecutorService"], destroyMethod = "shutdownNow")
    open fun ydbExecutorService(executorsFactory: ExecutorsFactory): DialogovoInstrumentedExecutorService {
        return executorsFactory.cachedBoundedThreadPool(20, 100, 1000, "ydb-client")
    }

    @Bean
    open fun ydbClient(
        sessionRetryContext: SessionRetryContext,
        listeners: List<YDBQueryListener>,
    ) = YdbClient(sessionRetryContext, listeners, minPoolSize)
}
