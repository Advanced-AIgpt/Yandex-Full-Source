package ru.yandex.alice.paskill.dialogovo.service.xiva;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig;
import ru.yandex.alice.paskill.dialogovo.config.XivaConfig;
import ru.yandex.alice.paskill.dialogovo.processor.DirectiveToDialogUriConverter;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;

@Configuration
public class XivaServiceConfiguration {

    @Bean
    public XivaService xivaService(
            XivaConfig config,
            SecretsConfig secretsConfig,
            RestTemplateClientFactory restTemplateClientFactory,
            DirectiveToDialogUriConverter converter,
            @Qualifier("xivaServiceExecutor") DialogovoInstrumentedExecutorService executor
    ) {

        var xivaConfig = config;

        var restTemplate = restTemplateClientFactory
                .serviceWebClientWithRetry(
                        "xiva",
                        Duration.ofMillis(500),
                        2, false,
                        Duration.ofMillis(xivaConfig.getConnectTimeout()),
                        Duration.ofMillis(xivaConfig.getTimeout()),
                        100);

        return new XivaServiceImpl(xivaConfig, secretsConfig, restTemplate, executor, converter);
    }

    @Bean(value = "xivaServiceExecutor", destroyMethod = "shutdownNow")
    public DialogovoInstrumentedExecutorService xivaServiceExecutor(ExecutorsFactory executorsFactory) {
        return executorsFactory.cachedBoundedThreadPool(2, 50, 50, "xiva-service");
    }

}
