package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import com.zaxxer.hikari.HikariConfig
import com.zaxxer.hikari.HikariDataSource
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.Primary
import org.springframework.context.annotation.Profile
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcTemplate
import org.springframework.jdbc.datasource.DataSourceTransactionManager
import org.springframework.transaction.PlatformTransactionManager
import org.springframework.transaction.annotation.EnableTransactionManagement
import ru.yandex.alice.kronstadt.core.db.reorderHosts
import java.util.concurrent.TimeUnit
import javax.sql.DataSource

@Configuration
@EnableTransactionManagement
open class PaskillsDatabaseConfig {

    private val logger = LogManager.getLogger()

    @Value("\${PORTO_HOST:null}")
    private var portoHost: String? = null

    @Value("\${paskillsConfig.database.multihost}")
    private lateinit var multihost: String

    @Value("\${paskillsConfig.database.databaseName}")
    private lateinit var databaseName: String

    @Value("\${paskillsConfig.database.user}")
    private lateinit var user: String

    @Value("\${paskillsConfig.database.pass}")
    private lateinit var password: String

    @Value("\${paskillsConfig.database.poolSizeIdle}")
    private val poolSizeIdle: Int = 1

    @Value("\${paskillsConfig.database.poolSizeMax}")
    private val poolSizeMax: Int = 10

    @Value("\${paskillsConfig.database.supportSsl:true}")
    private val supportsSsl: Boolean = true

    @Value("\${paskillsConfig.database.connectionTimeout:300}")
    private val connectionTimeout: Long = 300

    private val poolConnectionIdleTime = 3
    private val polConnectionMaxLifetime = 5

    @Bean
    @Profile("!ut")
    open fun dataSource(): HikariDataSource {
        val hosts = reorderHosts(portoHost, multihost)

        logger.info("reordered hosts for pg: {}", hosts)

        val config = HikariConfig()
        config.driverClassName = "org.postgresql.Driver"
        config.jdbcUrl = "jdbc:postgresql://${hosts}/${databaseName}"
        config.username = user
        config.password = password
        config.addDataSourceProperty("prepareThreshold", "0")
        config.addDataSourceProperty("preparedStatementCacheQueries", "0")
        config.addDataSourceProperty("targetServerType", "any")
        config.addDataSourceProperty("ssl", supportsSsl.toString())

        logger.info("POOL_IDLE_SIZE: $poolSizeIdle")
        config.minimumIdle = poolSizeIdle

        logger.info("POOL_MAX_SIZE: $poolSizeMax")
        config.maximumPoolSize = poolSizeMax

        logger.info("POOL_CONNECTION_MAX_LIFETIME: $polConnectionMaxLifetime min")
        config.maxLifetime = TimeUnit.MINUTES.toMillis(polConnectionMaxLifetime.toLong())

        logger.info("POOL_CONNECTION_IDLE_TIMEOUT: $poolConnectionIdleTime min")
        config.idleTimeout = TimeUnit.MINUTES.toMillis(poolConnectionIdleTime.toLong())
        config.connectionTimeout = connectionTimeout //ms
        return HikariDataSource(config)
    }

    @Bean
    @Primary
    open fun namedParameterJdbcOperation(dataSource: DataSource) =
        NamedParameterJdbcTemplate(dataSource)

    @Bean
    @Primary
    open fun transactionManager(dataSource: DataSource): PlatformTransactionManager =
        DataSourceTransactionManager(dataSource)
}
