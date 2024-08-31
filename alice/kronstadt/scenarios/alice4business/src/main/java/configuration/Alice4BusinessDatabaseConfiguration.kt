package ru.yandex.alice.kronstadt.scenarios.alice4business.configuration

import com.zaxxer.hikari.HikariConfig
import com.zaxxer.hikari.HikariDataSource
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.Profile
import org.springframework.data.jdbc.repository.config.EnableJdbcRepositories
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcTemplate
import org.springframework.jdbc.datasource.DataSourceTransactionManager
import org.springframework.jdbc.datasource.lookup.AbstractRoutingDataSource
import org.springframework.transaction.annotation.EnableTransactionManagement
import org.springframework.transaction.support.TransactionSynchronizationManager
import javax.sql.DataSource

@Configuration
@EnableTransactionManagement
@EnableJdbcRepositories(
    basePackages = ["ru.yandex.alice.kronstadt.scenarios.alice4business.providers"],
    transactionManagerRef = "alice4BusinessTransactionManager",
    jdbcOperationsRef = "alice4businessJdbcOperations"
)
open class Alice4BusinessDatabaseConfiguration(
    @Value("\${alice4-business-config.pgReadWriteDatabaseUrl}")
    private val pgReadWriteDatabaseUrl: String,
    @Value("\${alice4-business-config.pgReadOnlyDatabaseUrl}")
    private val pgReadOnlyDatabaseUrl: String,
    @Value("\${alice4-business-config.pgUser}")
    private val pgUser: String,
    @Value("\${alice4-business-config.pgPassword}")
    private val pgPassword: String,
    @Value("\${alice4-business-config.poolSizeIdle}")
    private val poolSizeIdle: Int,
    @Value("\${alice4-business-config.poolSizeMax}")
    private val poolSizeMax: Int,
    @Value("\${alice4-business-config.poolConnectionMaxLifetime}")
    private val poolConnectionMaxLifetime: Long,
    @Value("\${alice4-business-config.poolConnectionIdleTimeout}")
    private val poolConnectionIdleTimeout: Long,
    @Value("\${alice4-business-config.poolConnectTimeout}")
    private val poolConnectTimeout: Long,
    @Value("\${alice4-business-config.pgSslEnable}")
    private val pgSslEnable: Boolean
) {

    @Bean
    @Profile("!ut")
    open fun alice4businessJdbcOperations(@Qualifier("alice4BusinessDataSource") alice4BusinessDataSource: DataSource) =
        NamedParameterJdbcTemplate(alice4BusinessDataSource)

    @Bean
    @Profile("!ut")
    open fun alice4BusinessTransactionManager(@Qualifier("alice4BusinessDataSource") alice4BusinessDataSource: DataSource) =
        DataSourceTransactionManager(alice4BusinessDataSource)

    @Bean
    @Profile("!ut")
    open fun alice4BusinessDataSource(): DataSource =
        TransactionRoutingDataSource().apply {
            setTargetDataSources(
                mapOf(
                    DataSourceType.READ_WRITE to basicDataSource(true),
                    DataSourceType.READ_ONLY to basicDataSource(false)
                )
            )
        }

    private fun basicDataSource(
        readWrite: Boolean
    ): HikariDataSource {
        val config = HikariConfig().apply {
            driverClassName = "org.postgresql.Driver"
            minimumIdle = poolSizeIdle
            maximumPoolSize = poolSizeMax
            maxLifetime = poolConnectionMaxLifetime
            idleTimeout = poolConnectionIdleTimeout
            connectionTimeout = poolConnectTimeout

            username = pgUser
            password = pgPassword
            jdbcUrl =
                if (readWrite) pgReadWriteDatabaseUrl else pgReadOnlyDatabaseUrl

            addDataSourceProperty("prepareThreshold", "0")
            addDataSourceProperty("preparedStatementCacheQueries", "0")
            addDataSourceProperty("targetServerType", if (readWrite) "master" else "preferSecondary")
            addDataSourceProperty("ssl", pgSslEnable)
        }

        return HikariDataSource(config)
    }

    private class TransactionRoutingDataSource : AbstractRoutingDataSource() {
        override fun determineCurrentLookupKey(): Any {
            return if (TransactionSynchronizationManager.isCurrentTransactionReadOnly())
                DataSourceType.READ_ONLY else DataSourceType.READ_WRITE
        }
    }

    private enum class DataSourceType {
        READ_ONLY, READ_WRITE
    }
}

