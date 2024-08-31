package ru.yandex.quasar.billing.monitoring;

import java.sql.SQLException;

import javax.sql.DataSource;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.boot.actuate.endpoint.annotation.Endpoint;
import org.springframework.boot.actuate.endpoint.annotation.ReadOperation;
import org.springframework.boot.actuate.health.Health;
import org.springframework.boot.actuate.health.HealthIndicator;
import org.springframework.boot.actuate.health.PingHealthIndicator;
import org.springframework.stereotype.Component;

@Component
@Endpoint(id = "healthcheck")
class BillingHealthCheckEndpoint {

    private final HealthIndicator healthIndicator;
    private final DataSource ds;

    private static final Logger logger = LogManager.getLogger();


    BillingHealthCheckEndpoint(PingHealthIndicator healthIndicator, DataSource ds) {
        this.healthIndicator = healthIndicator;
        this.ds = ds;
    }

    @ReadOperation
    public Health healthcheck() {
        try (var connection = ds.getConnection()) {
            // ok
            return this.healthIndicator.health();
        } catch (SQLException e) {
            logger.error("Failed to obtain connection", e);
            throw new RuntimeException("service not ready");
        }
    }

}
