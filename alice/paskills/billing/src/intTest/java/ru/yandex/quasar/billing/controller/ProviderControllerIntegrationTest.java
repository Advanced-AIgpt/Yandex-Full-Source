package ru.yandex.quasar.billing.controller;


import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.test.context.junit.jupiter.SpringExtension;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.PromoCodeBase;
import ru.yandex.quasar.billing.dao.PromoCodeBaseDao;
import ru.yandex.quasar.billing.services.TestAuthorizationService;
import ru.yandex.quasar.billing.services.mediabilling.TestMediaBillingClient;
import ru.yandex.quasar.billing.services.processing.trust.TestTrustClient;
import ru.yandex.quasar.billing.services.processing.trust.TestTrustClientConfig;
import ru.yandex.quasar.billing.services.promo.DeviceId;
import ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult;
import ru.yandex.quasar.billing.services.promo.QuasarBackendClientTestImpl;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.Mockito.doReturn;

@ExtendWith(SpringExtension.class)
@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {TestConfigProvider.class, TestTrustClientConfig.class}
)
@AutoConfigureWebClient(registerRestTemplate = true)
class ProviderControllerIntegrationTest {

    @Autowired
    private TestMediaBillingClient mediaBilling;
    @SpyBean
    private TestTrustClient trustClient;
    @Autowired
    private PromoCodeBaseDao promoCodeBaseDao;


    @LocalServerPort
    private int port;
    private RestTemplate restTemplate = new RestTemplate();

    private PromoCodeBase code1;

    @BeforeEach
    void setUp() throws Exception {
        mediaBilling.getActivations().clear();
        code1 = promoCodeBaseDao.save(PromoCodeBase.create("yandexplus", PromoType.plus360, "code1"));
    }

    @Test
    void testActivatePromoOnDevice() {
        ResponseEntity<DevicePromoActivationResponse> request1 =
                activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_YANDEXSTATION);
        assertEquals(HttpStatus.OK, request1.getStatusCode());
        assertEquals(DevicePromoActivationResult.SUCCESS, request1.getBody().result());

        assertEquals(Map.of(TestAuthorizationService.UID, code1.getCode()), mediaBilling.getActivations());
    }

    @Test
    void testActivatePromoOnUnknownDevice() {
        ResponseEntity<DevicePromoActivationResponse> request1 =
                activatePromoOnDevice(QuasarBackendClientTestImpl.UNKNOWN_DEVICE);
        assertEquals(HttpStatus.OK, request1.getStatusCode());
        assertEquals(DevicePromoActivationResult.ALREADY_ACTIVATED, request1.getBody().result());

    }

    @Test
    void testActivatePromoOnDeviceAlreadyActivated() {
        ResponseEntity<DevicePromoActivationResponse> request1 =
                activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_YANDEXSTATION);
        assertEquals(HttpStatus.OK, request1.getStatusCode());
        assertEquals(DevicePromoActivationResult.SUCCESS, request1.getBody().result());

        assertEquals(Map.of(TestAuthorizationService.UID, code1.getCode()), mediaBilling.getActivations());

        // DevicePromoAlreadyUsedException
        var request2 = activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_YANDEXSTATION);
        assertEquals(HttpStatus.OK, request2.getStatusCode());
        assertEquals(DevicePromoActivationResult.ALREADY_ACTIVATED, request2.getBody().result());
    }

    @Test
    void testActivatePromoOnDeviceNotOwned() {
        // DeviceNotOwnedByUserException
        HttpClientErrorException exception = assertThrows(HttpClientErrorException.class,
                () -> activatePromoOnDevice(QuasarBackendClientTestImpl.SOME_OTHER_DEVICE));
        assertEquals(HttpStatus.BAD_REQUEST, exception.getStatusCode());
    }

    @Test
    void testActivatePromoOnDeviceNoCardBound() {
        doReturn(List.of()).when(trustClient).getCardsList(TestAuthorizationService.UID, TestAuthorizationService.IP);
        // NoCardBoundException
        HttpClientErrorException exception = assertThrows(HttpClientErrorException.class,
                () -> activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_YANDEXSTATION));
        assertEquals(HttpStatus.BAD_REQUEST, exception.getStatusCode());
    }

    private UriComponentsBuilder url(String provider, @Nullable String method) {
        UriComponentsBuilder builder = UriComponentsBuilder.fromUriString("http://localhost:" + port).pathSegment(
                "billing", "provider", provider);
        if (method != null) {
            builder = builder.pathSegment(method);
        }
        return builder;
    }

    private ResponseEntity<DevicePromoActivationResponse> activatePromoOnDevice(DeviceId device) {
        MultiValueMap<String, String> authHeader = TestAuthorizationService.getSessionCookie();
        authHeader.add("Content-Type", "application/x-www-form-urlencoded");

        ResponseEntity<DevicePromoActivationResponse> responseEntity = restTemplate.exchange(
                url("yandexplus", "activatePromoOnDevice")
                        .queryParam("deviceId", device.getId())
                        .queryParam("platform", device.getPlatform().toString())
                        .queryParam("paymentCardId", TestTrustClient.PAYMETHOD_ID)
                        .queryParam("userEmail", "user@yandex.ru")
                        .build()
                        .encode()
                        .toUri(),
                HttpMethod.POST,
                new HttpEntity<>(authHeader),
                DevicePromoActivationResponse.class
        );
        return responseEntity;

    }
}
