package ru.yandex.quasar.billing.services.tvm;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.alice.paskills.common.tvm.solomon.TvmClientWithSolomon;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.passport.tvmauth.NativeTvmClient;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.TvmToolSettings;
import ru.yandex.quasar.billing.config.BillingConfig;

@Configuration
class TvmConfiguration {
    @Value("${TVM_PORT:1}")
    private int tvmPort;

    TvmToolSettings tvmToolSettings(BillingConfig config) {
        return TvmToolSettings.create("quasar-billing")
                .setHostname("http://localhost")
                .setPort(tvmPort)
                .setAuthToken(config.getTvmConfig().getTvmToken());
    }

    @Profile("!ut")
    @Bean
    public TvmClient tvmClient(
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry,
            BillingConfig config
    ) {
        return new TvmClientWithSolomon(new NativeTvmClient(tvmToolSettings(config)), metricRegistry);
    }


}
