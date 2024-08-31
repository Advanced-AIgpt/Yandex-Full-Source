package ru.yandex.quasar.billing.services.processing.yapay;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;

/**
 * Separate configuration is used to overwrite it in tests
 */
@Configuration
public class YaPayClientConfig {

    @Bean("yandexPayClient")
    YandexPayClient yandexPayClient(
            BillingConfig config,
            RestTemplateBuilder restTemplateBuilder,
            TvmClient tvmClient,
            ObjectMapper objectMapper
    ) {
        return new YandexPayClientImpl(config, restTemplateBuilder, tvmClient, objectMapper);
    }
}
