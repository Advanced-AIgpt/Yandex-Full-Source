package ru.yandex.alice.paskill.dialogovo.service.billing;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.config.BillingConfig;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
class BillingServiceConfiguration {
    private static final String BILLING_SERVICE_NAME = "billing";
    private static final int RETRY_COUNT = 2;

    @Bean
    public BillingService billingService(
            BillingConfig config,
            RestTemplateClientFactory restTemplateClientFactory,
            @Qualifier("billingServiceExecutor") DialogovoInstrumentedExecutorService executor,
            TvmClient tvmClient,
            RequestContext requestContext
    ) {

        return new BillingServiceImpl(
                config.getUrl(),
                Duration.ofMillis(config.getTimeout()),
                restTemplateClientFactory.serviceWebClientWithRetry(
                        BILLING_SERVICE_NAME,
                        Duration.ofMillis(250),
                        RETRY_COUNT,
                        true,
                        Duration.ofMillis(config.getConnectTimeout()),
                        Duration.ofMillis(config.getTimeout()),
                        10
                ),
                executor,
                tvmClient,
                requestContext
        );
    }


    @Bean(value = "billingServiceExecutor", destroyMethod = "shutdownNow")
    public DialogovoInstrumentedExecutorService socialExecutorService(ExecutorsFactory executorsFactory) {
        return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "billing-service");
    }
}
