package ru.yandex.alice.paskills.common.resttemplate.factory.client;

import java.time.Duration;
import java.time.Instant;
import java.util.Optional;

import org.apache.http.conn.routing.HttpRoute;
import org.apache.http.pool.ConnPoolControl;
import org.apache.http.pool.PoolStats;

public class HttpClientPoolMetrics {

    private final ConnPoolControl<HttpRoute> connPoolControl;
    private Optional<Instant> lastUpdate = Optional.empty();
    private Optional<PoolStats> cachedStats = Optional.empty();

    HttpClientPoolMetrics(ConnPoolControl<HttpRoute> connPoolControl) {
        this.connPoolControl = connPoolControl;
    }

    int available() {
        getStats();
        return cachedStats.get().getAvailable();
    }

    public int leased() {
        getStats();
        return cachedStats.get().getLeased();
    }

    public int max() {
        getStats();
        return cachedStats.get().getMax();
    }

    public int pending() {
        getStats();
        return cachedStats.get().getPending();
    }

    // updating once a 30 sec cause pool stats is synchronized
    private void getStats() {
        Instant now = Instant.now();
        boolean shouldUpdate =
                cachedStats.isEmpty()
                        || lastUpdate
                        .map(lastUpdate -> Duration.between(lastUpdate, now).compareTo(Duration.ofSeconds(30)) > 0)
                        .orElse(true);

        if (shouldUpdate) {
            // no need to do synchronization due to synchronization in pool
            cachedStats = Optional.of(connPoolControl.getTotalStats());
            lastUpdate = Optional.of(now);
        }
    }
}
