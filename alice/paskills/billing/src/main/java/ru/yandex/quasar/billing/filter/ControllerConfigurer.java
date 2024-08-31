package ru.yandex.quasar.billing.filter;

import org.springframework.stereotype.Component;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.SecretsConfig;

@Component
class ControllerConfigurer implements WebMvcConfigurer {

    private final TvmClient tvmClient;
    private final SecretsConfig secretsConfig;

    ControllerConfigurer(TvmClient tvmClient, SecretsConfig secretsConfig) {
        this.tvmClient = tvmClient;
        this.secretsConfig = secretsConfig;
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(new TvmServiceAuthorizationInterceptor(tvmClient, secretsConfig));
    }
}
