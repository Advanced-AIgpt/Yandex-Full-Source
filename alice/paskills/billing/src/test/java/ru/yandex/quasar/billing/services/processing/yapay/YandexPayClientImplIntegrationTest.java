package ru.yandex.quasar.billing.services.processing.yapay;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.web.client.RestTemplateAutoConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.RemoteServiceProxyExtension;
import ru.yandex.quasar.billing.RemoteServiceProxyMode;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.when;

@SpringJUnitConfig(classes = {TestConfigProvider.class, YandexPayClientImpl.class, AuthorizationContext.class,
        RestTemplateAutoConfiguration.class})
@Disabled("no access to test env from local machine")
class YandexPayClientImplIntegrationTest {
    /*
    when recording put real tvm ticket here by executing
    curl --request GET    --url 'http://localhost:1/tvm/tickets?dsts=2002162' --header "Authorization: $QLOUD_TVM_TOKEN"
     */
    private static final String YA_PAY_TICKET = "";
    @RegisterExtension
    RemoteServiceProxyExtension wiremock = new RemoteServiceProxyExtension(
            "https://payments-test.mail.yandex.net/",
            RemoteServiceProxyMode.DISABLED
    );
    @MockBean
    private TvmClient tvmClient;
    @Autowired
    private YandexPayClientImpl client;
    @Autowired
    private BillingConfig config;
    @Autowired
    private ObjectMapper objectMapper;
    @Autowired
    private RestTemplateBuilder restTemplateBuilder;
    private String originalUrl;

    @BeforeEach
    void setUp() {
        originalUrl = config.getYaPayConfig().getApiBaseUrl();
        config.getYaPayConfig().setApiBaseUrl(wiremock.getUrl());

        client = new YandexPayClientImpl(config, restTemplateBuilder, tvmClient, objectMapper);

        when(tvmClient.getServiceTicketFor(any()))
                .thenReturn("ticket");
        when(tvmClient.getServiceTicketFor(eq(TvmClientName.ya_pay.getAlias())))
                .thenReturn(YA_PAY_TICKET);
    }

    @AfterEach
    void tearDown() {
        config.getYaPayConfig().setApiBaseUrl(originalUrl);
    }

    @Test
    void testMerchantInfo() {
        ServiceMerchantInfo serviceMerchantInfo = client.merchantInfo(54);
        System.out.println(serviceMerchantInfo);
    }

}
