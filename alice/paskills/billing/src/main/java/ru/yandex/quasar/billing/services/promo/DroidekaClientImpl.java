package ru.yandex.quasar.billing.services.promo;

import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.DroidekaConfig;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;
import ru.yandex.quasar.billing.util.ParallelHelper;

@Component
class DroidekaClientImpl implements DroidekaClient {

    private final RestTemplate client;
    private final AuthorizationContext authorizationContext;
    private final TvmClient tvmClient;
    private final DroidekaConfig config;
    private final ParallelHelper parallelHelper;

    private static final Logger logger = LogManager.getLogger();

    DroidekaClientImpl(RestTemplateBuilder builder, AuthorizationContext authorizationContext,
                       TvmClient tvmClient, BillingConfig config) {
        this.authorizationContext = authorizationContext;
        this.tvmClient = tvmClient;
        this.config = config.getDroidekaConfig();
        this.client = builder
                .setConnectTimeout(Duration.ofMillis(this.config.getConnectionTimeout()))
                .setReadTimeout(Duration.ofMillis(this.config.getReadTimeout()))
                .build();
        this.parallelHelper = new ParallelHelper(Executors.newCachedThreadPool(), authorizationContext);
    }

    @Override
    public CompletableFuture<GiftState> getGiftStateAsync(String serial, String wifiMac, String ethernetMan) {

        return parallelHelper.async(() -> getGiftState(serial, wifiMac, ethernetMan));
    }

    @Override
    public GiftState getGiftState(String serial, String wifiMac, String ethernetMan) {

        var headers = new HttpHeaders();
        headers.add("serial", serial);
        headers.add("X-Wifi-Mac", wifiMac);
        headers.add("X-Ethernet-Mac", ethernetMan);
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, tvmClient.getServiceTicketFor(TvmClientName.droideka.getAlias()));
        headers.add(TvmHeaders.USER_TICKET_HEADER, authorizationContext.getCurrentUserTicket());

        try {
            return client.exchange(config.getUrl(), HttpMethod.GET, new HttpEntity<>(headers), GiftState.class)
                    .getBody();
        } catch (HttpClientErrorException.NotFound e) {
            // device may not present in droideka DB if in was produced after migration to billing.
            // there is no chance promo was obtained via droideka and we know it's valid device by calling quasar
            return GiftState.AVAILABLE;
        } catch (Exception e) {
            logger.error("Failed to call droideka", e);
            // if droideka fails with timeout or any 5xx just treat promo as available
            return GiftState.AVAILABLE;
        }
    }
}
