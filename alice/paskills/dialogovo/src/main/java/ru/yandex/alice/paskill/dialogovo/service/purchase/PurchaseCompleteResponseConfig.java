package ru.yandex.alice.paskill.dialogovo.service.purchase;

import java.time.Duration;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.YdbConfig;
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
public class PurchaseCompleteResponseConfig {
    private static final Duration TIMEOUT = Duration.ofMillis(1000);

    @Bean
    public PurchaseCompleteResponseDao purchaseCompleteSkillResponseDao(
            YdbClient ydbClient,
            ObjectMapper objectMapper,
            YdbConfig ydbConfig,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry
    ) {
        var responseDao = new PurchaseCompleteResponseDaoImpl(ydbClient, objectMapper, metricRegistry, TIMEOUT);
        if (ydbConfig.isWarmUpOnStart()) {
            responseDao.prepareQueries();
        }
        return responseDao;
    }
}
