package ru.yandex.alice.paskill.dialogovo.service.penguinary;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.PenguinaryConfig;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;

@Configuration
class PenguinaryClientConfiguration {

    @Bean
    PenguinaryServiceImpl penguinaryClient(
            PenguinaryConfig penguinaryConfig,
            RestTemplateClientFactory restTemplateClientFactory,
            @Qualifier("penguinaryClientExecutor") DialogovoInstrumentedExecutorService executor
    ) {

        final RestTemplate restTemplate = restTemplateClientFactory
                .serviceWebClientWithRetry(
                        "penguinary",
                        Duration.ofMillis(50),
                        2, true,
                        Duration.ofMillis(penguinaryConfig.getConnectTimeout()),
                        Duration.ofMillis(penguinaryConfig.getTimeout()),
                        100);
        return new PenguinaryServiceImpl(penguinaryConfig, restTemplate, executor);
    }


    @Bean(value = "penguinaryClientExecutor")
    public DialogovoInstrumentedExecutorService penguinaryClientService(ExecutorsFactory executorsFactory) {
        return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "penguinary");
    }
}
