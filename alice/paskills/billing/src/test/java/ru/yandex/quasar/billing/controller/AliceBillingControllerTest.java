package ru.yandex.quasar.billing.controller;

import java.net.URI;
import java.util.Optional;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;

@ExtendWith(EmbeddedPostgresExtension.class)
@AutoConfigureWebClient
@SpringBootTest(
        properties = "spring.main.allow-bean-definition-overriding=true",
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {SyncExecutorServicesConfig.class, TestConfigProvider.class}
)
class AliceBillingControllerTest {

    public static final String UID = "999";
    @Autowired
    private TestRestTemplate testRestTemplate;
    @SpyBean
    private BillingService billingService;
    @SpyBean
    private AuthorizationService authorizationService;
    @LocalServerPort
    private int port;

    private void mockAuth() {
        doReturn(UID).when(authorizationService).getSecretUid(any());
        doReturn("127.0.0.1").when(authorizationService).getUserIp(any());
        doReturn(Optional.of("222")).when(authorizationService).getProviderTokenByUid(any(), any());
    }

    private UriComponentsBuilder url(String method) {
        return UriComponentsBuilder.fromUriString("http://localhost:" + port).pathSegment("billing", method);
    }


    @Test
    void startBinding() {
        mockAuth();
        doReturn(new NewBindingInfo("http://ya.ru", "qweqweqwe"))
                .when(billingService).startBinding(eq(UID), eq("127.0.0.1"), eq(PaymentProcessor.TRUST),
                        eq(TrustCurrency.RUB), anyString(), eq(TemplateTag.MOBILE));

        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Authorization", "OAuth some_token");
        ResponseEntity<StartCardBindingResponse> responseEntity = testRestTemplate.postForEntity(url("startBinding")
                        .queryParam("processor", "TRUST")
                        .queryParam("return_url", "http://return.url")
                        .build()
                        .toUri(),
                new HttpEntity<>(headers),
                StartCardBindingResponse.class);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(new StartCardBindingResponse("http://ya.ru", "qweqweqwe"), responseEntity.getBody());
    }

    @Test
    void startBindingTv() {
        mockAuth();
        doReturn(new NewBindingInfo("http://ya.ru", "qweqweqwe"))
                .when(billingService).startBinding(eq(UID), eq("127.0.0.1"), eq(PaymentProcessor.MEDIABILLING),
                        eq(TrustCurrency.RUB), anyString(), eq(TemplateTag.STARTTV));

        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Authorization", "OAuth some_token");
        ResponseEntity<StartCardBindingResponse> responseEntity =
                testRestTemplate.postForEntity(url("promo/tv/bind_card")
                                .queryParam("return_url", "http://return.url")
                                .queryParam("template", TemplateTag.STARTTV)
                                .build()
                                .toUri(),
                        new HttpEntity<>(headers),
                        StartCardBindingResponse.class);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(new StartCardBindingResponse("http://ya.ru", "qweqweqwe",
                URI.create("https://yandex.ru/")), responseEntity.getBody());
    }

    @Test
    void getBindingInfo() {
        mockAuth();
        doReturn(new BindingInfo(BindingInfo.Status.success, "card-0000"))
                .when(billingService).getBindingInfo(eq(UID), eq("127.0.0.1"), eq(PaymentProcessor.TRUST), eq(
                        "qweqweqwe"));

        HttpHeaders headers = new HttpHeaders();
        headers.add("Authorization", "OAuth some_token");
        ResponseEntity<BindingInfoResponse> responseEntity = testRestTemplate.exchange(url("getBindingInfo")
                        .queryParam("processor", "TRUST")
                        .queryParam("purchase_token", "qweqweqwe")
                        .build()
                        .toUri(),
                HttpMethod.GET,
                new HttpEntity<>(headers),
                BindingInfoResponse.class);

        assertEquals(new BindingInfoResponse("success", "card-0000"), responseEntity.getBody());
    }

    @Test
    void getBindingInfoTvNotEnoughFunds() {
        mockAuth();
        doReturn(new BindingInfo(BindingInfo.Status.not_enough_funds, null))
                .when(billingService).getBindingInfo(eq(UID), eq("127.0.0.1"), any(), eq("qweqweqwe"));

        HttpHeaders headers = new HttpHeaders();
        headers.add("Authorization", "OAuth some_token");
        ResponseEntity<BindingInfoResponse> responseEntity = testRestTemplate.exchange(
                UriComponentsBuilder.fromUriString("http://localhost:" + port)
                        .pathSegment("billing/promo/tv/binding_info")
                        .queryParam("purchase_token", "qweqweqwe")
                        .build()
                        .toUri(),
                HttpMethod.GET,
                new HttpEntity<>(headers),
                BindingInfoResponse.class);

        assertEquals(new BindingInfoResponse("not_enough_funds", null), responseEntity.getBody());
    }

}
