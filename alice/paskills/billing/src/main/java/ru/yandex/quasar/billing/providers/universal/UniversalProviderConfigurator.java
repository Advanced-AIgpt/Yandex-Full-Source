package ru.yandex.quasar.billing.providers.universal;

import java.time.Duration;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.function.BiConsumer;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpHeaders;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.UniversalProviderConfig;
import ru.yandex.quasar.billing.providers.XDeviceHeaders;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

@Configuration
public class UniversalProviderConfigurator {

    public static final String KINOPOISK_PROVIDER_NAME = "kinopoisk";

    @Bean("kinopoiskContentProvider")
    public UniversalProvider kinopoiskContentProvider(
            BillingConfig billingConfig,
            AuthorizationContext context,
            TvmClient tvmClient,
            RestTemplateBuilder restTemplateBuilder,
            @Qualifier("kinopoiskExecutorService") ExecutorService executorService,
            UnistatService unistatService
    ) {

        BiConsumer<HttpHeaders, String> fillAuthHeaders = (headers, token) -> {
            String serviceTicket = tvmClient.getServiceTicketFor("kp-universal");
            headers.add(TvmHeaders.SERVICE_TICKET_HEADER, serviceTicket);

            String userTicket = context.getCurrentUserTicket();
            if (userTicket != null) {
                headers.add(TvmHeaders.USER_TICKET_HEADER, userTicket);
            }
            if (context.getDeviceVideoCodecs() != null) {
                headers.add(XDeviceHeaders.X_DEVICE_VIDEO_CODECS, context.getDeviceVideoCodecs());
            }
            if (context.getDeviceAudioCodecs() != null) {
                headers.add(XDeviceHeaders.X_DEVICE_AUDIO_CODECS, context.getDeviceAudioCodecs());
            }
            if (context.getSupportsCurrentHDCPLevel() != null) {
                headers.add(XDeviceHeaders.SUPPORTS_CURRENT_HDCP_LEVEL, context.getSupportsCurrentHDCPLevel());
            }
            if (context.getDeviceDynamicRanges() != null) {
                headers.add(XDeviceHeaders.X_DEVICE_DYNAMIC_RANGES, context.getDeviceDynamicRanges());
            }
            if (context.getDeviceVideoFormats() != null) {
                headers.add(XDeviceHeaders.X_DEVICE_VIDEO_FORMATS, context.getDeviceVideoFormats());
            }

        };

        UniversalProviderConfig kinopoiskConfig = billingConfig.getUniversalProviders().get(KINOPOISK_PROVIDER_NAME);

        var builder = restTemplateBuilder
                .setConnectTimeout(Duration.ofMillis(kinopoiskConfig.getConnectionTimeoutMs()))
                .setReadTimeout(Duration.ofMillis(kinopoiskConfig.getReadTimeoutMs()));
        return new UniversalProvider(KINOPOISK_PROVIDER_NAME,
                billingConfig.getProvidersConfig().get(KINOPOISK_PROVIDER_NAME),
                kinopoiskConfig.getBaseUrl(),
                builder,
                context,
                fillAuthHeaders,
                executorService,
                unistatService);
    }

    @Bean(value = "kinopoiskExecutorService", destroyMethod = "shutdownNow")
    public ExecutorService kinopoiskExecutorService() {
        return Executors.newCachedThreadPool(
                new ThreadFactoryBuilder()
                        .setNameFormat("kinopoisk-%d")
                        .build()
        );
    }

}
