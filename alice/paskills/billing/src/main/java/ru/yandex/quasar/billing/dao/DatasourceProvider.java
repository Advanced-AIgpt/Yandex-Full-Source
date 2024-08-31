package ru.yandex.quasar.billing.dao;

import com.zaxxer.hikari.HikariConfig;
import com.zaxxer.hikari.HikariDataSource;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.library.routingdatasource.RoutingDataSource;
import ru.yandex.quasar.billing.config.SecretsConfig;

import static java.util.concurrent.TimeUnit.MINUTES;

@Configuration
public class DatasourceProvider {

    private static final Logger logger = LogManager.getLogger();

    private static final int POOL_CONNECTION_IDLE_TIMEOUT = 3;
    private static final int POOL_CONNECTION_MAX_LIFETIME = 5;

    private final int poolSizeIdle;
    private final int poolSizeMax;
    private final boolean pgSslEnable;
    private final RoutingDataSource dataSource;

    public DatasourceProvider(
            @Value("${db.pool_idle_size:0}") int poolSizeIdle,
            @Value("${db.pool_max_size:3}") int poolSizeMax,
            @Value("${db.ssl_enable:false}") boolean pgSslEnable,
            SecretsConfig secretsConfig
    ) {
        this.poolSizeIdle = poolSizeIdle;
        this.poolSizeMax = poolSizeMax;
        this.pgSslEnable = pgSslEnable;
        this.dataSource = new RoutingDataSource(
                basicDataSource(secretsConfig, true),
                basicDataSource(secretsConfig, false)
        );

    }

    private HikariDataSource basicDataSource(SecretsConfig secretsConfig, boolean readWrite) {
        HikariConfig config = new HikariConfig();

        config.setDriverClassName("org.postgresql.Driver");

        var url = readWrite ?
                secretsConfig.getPgReadWriteDatabaseUrl() :
                secretsConfig.getPgReadOnlyDatabaseUrl();
        config.setJdbcUrl(url);
        config.setUsername(secretsConfig.getPgUser());
        config.setPassword(secretsConfig.getPgPassword());
        config.addDataSourceProperty("prepareThreshold", "0");
        config.addDataSourceProperty("preparedStatementCacheQueries", "0");
        config.addDataSourceProperty("targetServerType", readWrite ? "master" : "preferSecondary");
        if (pgSslEnable) {
            config.addDataSourceProperty("ssl", "true");
        }
        logger.info("POOL_IDLE_SIZE: " + poolSizeIdle);
        config.setMinimumIdle(poolSizeIdle);
        logger.info("POOL_MAX_SIZE: " + poolSizeMax);
        config.setMaximumPoolSize(poolSizeMax);
        logger.info("POOL_CONNECTION_MAX_LIFETIME: " + POOL_CONNECTION_MAX_LIFETIME + " min");
        config.setMaxLifetime(MINUTES.toMillis(POOL_CONNECTION_MAX_LIFETIME));
        logger.info("POOL_CONNECTION_IDLE_TIMEOUT: " + POOL_CONNECTION_IDLE_TIMEOUT + " min");
        config.setIdleTimeout(MINUTES.toMillis(POOL_CONNECTION_IDLE_TIMEOUT));

        config.setConnectionTimeout(500);

        return new HikariDataSource(config);
    }

    @Bean
    public RoutingDataSource dataSource() {
        return dataSource;
    }

}
