package ru.yandex.quasar.billing.controller;


import java.util.List;
import java.util.Map;

import org.json.JSONObject;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.test.context.junit.jupiter.SpringExtension;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.PromoDuration;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.controller.BillingController.PromoActivationRequestBody;
import ru.yandex.quasar.billing.dao.PromocodePrototypeDao;
import ru.yandex.quasar.billing.dao.PromocodePrototypeDb;
import ru.yandex.quasar.billing.services.TestAuthorizationService;
import ru.yandex.quasar.billing.services.mediabilling.TestMediaBillingClient;
import ru.yandex.quasar.billing.services.processing.trust.TestTrustClient;
import ru.yandex.quasar.billing.services.processing.trust.TestTrustClientConfig;
import ru.yandex.quasar.billing.services.promo.BackendDeviceInfo;
import ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult;
import ru.yandex.quasar.billing.services.promo.DroidekaClient;
import ru.yandex.quasar.billing.services.promo.QuasarBackendClientTestImpl;
import ru.yandex.quasar.billing.services.promo.QuasarPromoServiceImpl;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;

@ExtendWith(SpringExtension.class)
@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {TestConfigProvider.class, TestTrustClientConfig.class}
)
@AutoConfigureWebClient(registerRestTemplate = true)
class TvPromoIntegrationTest {

    @Autowired
    private TestMediaBillingClient mediaBilling;
    @SpyBean
    private TestTrustClient trustClient;
    @Autowired
    private PromocodePrototypeDao promocodePrototypeDao;
    @MockBean
    private DroidekaClient droidekaClient;
    @Autowired
    private QuasarPromoServiceImpl quasarPromoService;


    @LocalServerPort
    private int port;
    private RestTemplate restTemplate = new RestTemplate();

    private PromocodePrototypeDb prototype1;

    @BeforeEach
    void setUp() {
        mediaBilling.getActivations().clear();
        prototype1 = promocodePrototypeDao.save(
                new PromocodePrototypeDb(null,
                        QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL.getPlatform().getName(),
                        "plus90", "prototype1", "test-task1")
        );
        when(droidekaClient.getGiftState(
                eq(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL.getSerial()),
                eq(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL.getWifiMac()),
                eq(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL.getEthernetMac())
        )).thenReturn(DroidekaClient.GiftState.AVAILABLE);
        quasarPromoService.loadPrototypes();
    }

    @Test
    void testActivatePromoOnDevice() {

        ResponseEntity<BillingController.PromoPeriodInfo> promoAvailableResult =
                promoPeriodAvailable(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL);
        assertTrue(promoAvailableResult.getBody().available());
        assertNotNull(promoAvailableResult.getBody().promoDetails());
        assertEquals(PromoDuration.P3M.getValue(),
                promoAvailableResult.getBody().promoDetails().logicalDuration());

        ResponseEntity<DevicePromoActivationResponse> request1 =
                activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL);


        assertEquals(HttpStatus.OK, request1.getStatusCode());
        assertEquals(DevicePromoActivationResult.SUCCESS, request1.getBody().result());

        assertEquals(Map.of(TestAuthorizationService.UID, mediaBilling.clonePrototype(prototype1.getCode())),
                mediaBilling.getActivations());
    }

    @Test
    void testPromoExpirationDate() {
        JSONObject promoAvailableResult =
                promoPeriodAvailableText(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL);

        var expDate = promoAvailableResult.optJSONObject("promo_details").optString("promo_expiration_date", "");
        assertEquals("2022-12-31", expDate);
    }

    @Test
    void testPromoExpirationDateAnonymous() {
        JSONObject promoAvailableResult =
                promoPeriodAvailableText(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL, true);

        var expDate = promoAvailableResult.optJSONObject("promo_details").optString("promo_expiration_date", "");
        assertEquals("2022-12-31", expDate);
    }

    @Test
    void testActivatePromoOnDeviceAlreadyActivated() {

        ResponseEntity<BillingController.PromoPeriodInfo> promoAvailableResult =
                promoPeriodAvailable(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL);
        assertTrue(promoAvailableResult.getBody().available());
        assertNotNull(promoAvailableResult.getBody().promoDetails());
        assertNotNull(promoAvailableResult.getBody().promoDetails().promoExpirationDate());

        ResponseEntity<DevicePromoActivationResponse> request1 =
                activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL);
        assertEquals(HttpStatus.OK, request1.getStatusCode());
        assertEquals(DevicePromoActivationResult.SUCCESS, request1.getBody().result());

        assertEquals(Map.of(TestAuthorizationService.UID, mediaBilling.clonePrototype(prototype1.getCode())),
                mediaBilling.getActivations());

        // DevicePromoAlreadyUsedException
        var request2 = activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL);
        assertEquals(HttpStatus.OK, request2.getStatusCode());
        assertEquals(DevicePromoActivationResult.ALREADY_ACTIVATED, request2.getBody().result());
    }

    @Test
    void testActivatePromoOnDeviceNoCardBound() {
        doReturn(List.of()).when(trustClient).getCardsList(TestAuthorizationService.UID, TestAuthorizationService.IP);
        // NoCardBoundException
        HttpClientErrorException exception = assertThrows(HttpClientErrorException.class,
                () -> activatePromoOnDevice(QuasarBackendClientTestImpl.USER_NEW_NOT_ACTIVATED_TV_FULL));
        assertEquals(HttpStatus.BAD_REQUEST, exception.getStatusCode());
    }


    private ResponseEntity<DevicePromoActivationResponse> activatePromoOnDevice(BackendDeviceInfo device) {
        HttpHeaders authHeaders = new HttpHeaders();
        authHeaders.add(TvmHeaders.SERVICE_TICKET_HEADER, TestAuthorizationService.SERVICE_TICKET);
        authHeaders.add(TvmHeaders.USER_TICKET_HEADER, TestAuthorizationService.USER_TICKET);

        authHeaders.add("Content-Type", "application/json");

        ResponseEntity<DevicePromoActivationResponse> responseEntity = restTemplate.exchange(
                UriComponentsBuilder.fromUriString("http://localhost:" + port)
                        .path("billing/promo/tv/activate_promo_period")
                        .queryParam("device_id", device.getId())
                        .queryParam("platform", device.getPlatform().toString())
                        .queryParam("activation_region", device.getActivationRegion())
                        .queryParam("serial", device.getSerial())
                        .queryParam("wifi_mac", device.getWifiMac())
                        .queryParam("ethernet_mac", device.getEthernetMac())
                        .queryParam("tag", device.getTags().stream().toArray(String[]::new))
                        .build()
                        .encode()
                        .toUri(),
                HttpMethod.POST,
                new HttpEntity<>(new PromoActivationRequestBody(TestTrustClient.PAYMETHOD_ID), authHeaders),
                DevicePromoActivationResponse.class
        );
        if (responseEntity.getStatusCode().isError()) {
            throw new RuntimeException("(activate_promo_period) Http request failed: " +
                    responseEntity.getStatusCode().name());
        }
        return responseEntity;

    }

    private ResponseEntity<BillingController.PromoPeriodInfo> promoPeriodAvailable(BackendDeviceInfo device) {
        HttpHeaders authHeaders = new HttpHeaders();
        authHeaders.add(TvmHeaders.SERVICE_TICKET_HEADER, TestAuthorizationService.SERVICE_TICKET);
        authHeaders.add(TvmHeaders.USER_TICKET_HEADER, TestAuthorizationService.USER_TICKET);
        authHeaders.add("Content-Type", "application/x-www-form-urlencoded");

        ResponseEntity<BillingController.PromoPeriodInfo> responseEntity = restTemplate.exchange(
                UriComponentsBuilder.fromUriString("http://localhost:" + port)
                        .path("billing/promo/tv/promo_period_available")
                        .queryParam("device_id", device.getId())
                        .queryParam("platform", device.getPlatform().toString())
                        .queryParam("activation_region", device.getActivationRegion())
                        .queryParam("serial", device.getSerial())
                        .queryParam("wifi_mac", device.getWifiMac())
                        .queryParam("ethernet_mac", device.getEthernetMac())
                        .queryParam("tag", device.getTags().stream().toArray(String[]::new))
                        .build()
                        .encode()
                        .toUri(),
                HttpMethod.GET,
                new HttpEntity<>(authHeaders),
                BillingController.PromoPeriodInfo.class
        );
        if (responseEntity.getStatusCode().isError()) {
            throw new RuntimeException("(promo_period_available) Http request failed: " +
                    responseEntity.getStatusCode().name());
        }
        return responseEntity;

    }

    private JSONObject promoPeriodAvailableText(BackendDeviceInfo device) {
        return promoPeriodAvailableText(device, false);
    }

    private JSONObject promoPeriodAvailableText(BackendDeviceInfo device, boolean anonymous) {
        HttpHeaders authHeaders = new HttpHeaders();
        authHeaders.add(TvmHeaders.SERVICE_TICKET_HEADER, TestAuthorizationService.SERVICE_TICKET);
        if (!anonymous) {
            authHeaders.add(TvmHeaders.USER_TICKET_HEADER, TestAuthorizationService.USER_TICKET);
        }
        authHeaders.add("Content-Type", "application/x-www-form-urlencoded");

        ResponseEntity<String> responseEntity = restTemplate.exchange(
                UriComponentsBuilder.fromUriString("http://localhost:" + port)
                        .path("billing/promo/tv/promo_period_available")
                        .queryParam("device_id", device.getId())
                        .queryParam("platform", device.getPlatform().toString())
                        .queryParam("activation_region", device.getActivationRegion())
                        .queryParam("serial", device.getSerial())
                        .queryParam("wifi_mac", device.getWifiMac())
                        .queryParam("ethernet_mac", device.getEthernetMac())
                        .queryParam("tag", device.getTags().stream().toArray(String[]::new))
                        .build()
                        .encode()
                        .toUri(),
                HttpMethod.GET,
                new HttpEntity<>(authHeaders),
                String.class
        );
        if (responseEntity.getStatusCode().isError()) {
            throw new RuntimeException("(promo_period_available) Http request failed: " +
                    responseEntity.getStatusCode().name());
        }
        return new JSONObject(responseEntity.getBody());

    }
}
