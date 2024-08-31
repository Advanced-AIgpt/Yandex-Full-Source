package ru.yandex.alice.paskills.my_alice.geobase;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
public class GeoBaseConfiguration {

    @Value("${GEOBASE_PATH:${geobase.path}}")
    private String geoBasePath;

    @Bean(destroyMethod = "close")
    public GeoBase geoBase(Laas laas, MetricRegistry metricRegistry) {
        return new GeoBase(laas, geoBasePath, metricRegistry);
    }

}
