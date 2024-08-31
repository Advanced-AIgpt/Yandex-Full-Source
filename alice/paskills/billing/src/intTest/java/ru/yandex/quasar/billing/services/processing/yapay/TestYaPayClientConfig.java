package ru.yandex.quasar.billing.services.processing.yapay;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.context.annotation.Bean;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;

@TestConfiguration("yaPayClientConfig")
public class TestYaPayClientConfig extends YaPayClientConfig {

    @Bean("yandexPayClient")
    @Override
    TestYandexPayClient yandexPayClient(BillingConfig config, RestTemplateBuilder restTemplateBuilder,
                                        TvmClient tvmClient, ObjectMapper objectMapper) {
        return new TestYandexPayClient(restTemplateBuilder);
    }
}
