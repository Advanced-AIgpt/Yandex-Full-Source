package ru.yandex.quasar.billing.services.processing.yapay;

import java.util.Optional;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.web.client.RestTemplateAutoConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.when;

@SpringJUnitConfig(classes = {TestConfigProvider.class, YaPayBillingService.class, AuthorizationContext.class,
        RestTemplateAutoConfiguration.class})
class YaPayBillingServiceTest {

    @MockBean
    private YandexPayClient yandexPayClient;
    @MockBean
    private TrustBillingService trustBillingService;
    @MockBean
    private AuthorizationContext authorizationContext;
    @Autowired
    private YaPayBillingService service;
    @Autowired
    private BillingConfig config;

    @Test
    void startPayment() {
        // given
        String yandexuid = "4354364335";
        when(authorizationContext.getYandexUid()).thenReturn(yandexuid);
        String trustURl = "http://trust-url";
        when(yandexPayClient.startOrder(
                eq(123L),
                eq(345L),
                eq(new StartOrderRequest(null, yandexuid))))
                .thenReturn(new StartOrderResponse(trustURl));

        // when
        Optional<String> trustUrlO = service.startPayment(null, null, "123|345");

        // then
        assertEquals(trustUrlO, Optional.of(trustURl));
    }
}
