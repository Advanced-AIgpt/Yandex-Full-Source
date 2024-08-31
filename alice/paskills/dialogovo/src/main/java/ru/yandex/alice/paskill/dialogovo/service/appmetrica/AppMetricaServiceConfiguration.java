package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.AppMetricaConfig;
import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig;
import ru.yandex.alice.paskill.dialogovo.config.YdbConfig;
import ru.yandex.alice.paskill.dialogovo.solomon.SolomonUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
class AppMetricaServiceConfiguration {

    @Value("${appmetrica.cache.size:2000}")
    private long cacheSize;

    @Bean
    AppMetricaService appMetricaService(
            AppMetricaConfig config,
            SecretsConfig secretsConfig,
            RestTemplateClientFactory restTemplateClientFactory,
            @Qualifier("appMetricaServiceExecutor") DialogovoInstrumentedExecutorService executor,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry,
            AppMetricaFirstUserEventDao appMetricaFirstUserEventDao
    ) {
        var appMetricaConfig = config;
        AppMetricaServiceImpl appMetricaService = new AppMetricaServiceImpl(
                appMetricaConfig, secretsConfig.getAppMetricaEncryptionSecret(), executor, restTemplateClientFactory
                .serviceWebClientWithRetryAndGzip(
                        "appmetrica",
                        Duration.ofMillis(1500),
                        3, true,
                        Duration.ofMillis(appMetricaConfig.getConnectTimeout()),
                        Duration.ofMillis(appMetricaConfig.getTimeout()),
                        30), appMetricaFirstUserEventDao, cacheSize
        );

        SolomonUtils.measureCacheStats(
                metricRegistry,
                "appmetrica_service_api_key_decrypt",
                appMetricaService::getApiKeyDecryptCacheStats);
        SolomonUtils.measureCacheStats(
                metricRegistry,
                "appmetrica_service_api_key_new_user_event",
                appMetricaService::getApiKeyNewUserEventCacheStats);

        return appMetricaService;
    }

    @Bean
    public AppMetricaFirstUserEventDao appMetricaFirstUserEventDao(
            YdbConfig ydbConfig,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry,
            YdbClient ydbClient
    ) {
        var appMetricaFirstUserEventDao = new AppMetricaFirstUserEventDaoImpl(metricRegistry, ydbClient);
        if (ydbConfig.isWarmUpOnStart()) {
            appMetricaFirstUserEventDao.prepareQueries();
        }
        return appMetricaFirstUserEventDao;
    }

    @Bean(value = "appMetricaServiceExecutor", destroyMethod = "shutdownNow")
    public DialogovoInstrumentedExecutorService appMetricaServiceExecutor(ExecutorsFactory executorsFactory) {
        return executorsFactory.cachedBoundedThreadPool(2, 50, 100, "appmetrica-service");
    }
}
