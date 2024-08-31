package ru.yandex.quasar.billing.controller;


import java.text.ParseException;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.time.temporal.ChronoUnit;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.test.context.junit.jupiter.SpringExtension;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.Experiments;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.ProviderConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.controller.PromoInfoResponse.DeviceShortInfo;
import ru.yandex.quasar.billing.providers.TestContentProvider;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.laas.LaasClient;
import ru.yandex.quasar.billing.services.promo.BackendDeviceInfo;
import ru.yandex.quasar.billing.services.promo.DeviceId;
import ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult;
import ru.yandex.quasar.billing.services.promo.Platform;
import ru.yandex.quasar.billing.services.promo.PromoProvider;
import ru.yandex.quasar.billing.services.promo.QuasarBackendClient;
import ru.yandex.quasar.billing.services.promo.QuasarBackendException;
import ru.yandex.quasar.billing.services.promo.QuasarPromoService;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a6m_plus6m;
import static ru.yandex.quasar.billing.beans.PromoType.plus120;
import static ru.yandex.quasar.billing.beans.PromoType.plus360;
import static ru.yandex.quasar.billing.beans.PromoType.plus90;

/**
 * Test for {@link ProviderManagerController}
 */
@ExtendWith(SpringExtension.class)
@ExtendWith(EmbeddedPostgresExtension.class)
@AutoConfigureWebClient
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {TestConfigProvider.class}
)
class ProviderManagerControllerTest implements PricingOptionTestUtil {

    private static final String CARD_ID = "card-id";
    private static final String USER_EMAIL = "ua@ya.ru";
    private static final DateTimeFormatter FORMATTER = DateTimeFormatter.ISO_INSTANT;
    @Autowired
    private TestRestTemplate testRestTemplate;
    @Autowired
    private BillingConfig billingConfig;
    @SpyBean
    private AuthorizationService authorizationService;
    @SpyBean
    private QuasarPromoService quasarPromoService;
    @MockBean
    private QuasarBackendClient quasarBackendClient;
    @MockBean
    private TvmClient tvmClient;
    @Autowired
    private TestContentProvider testContentProvider;
    @SpyBean
    private LaasClient laasClient;
    @SpyBean
    private AuthorizationContext authorizationContext;

    @Autowired
    private JdbcTemplate jdbcTemplate;

    @LocalServerPort
    private int port;

    private String uid = "999";
    private ProviderConfig iviConfig;


    private UriComponentsBuilder url(String method) {
        return UriComponentsBuilder.fromUriString("http://localhost:" + port).pathSegment("billing", method);
    }

    @BeforeEach
    void setUp() {
        iviConfig = billingConfig.getProvidersConfig().get("ivi");
        when(tvmClient.getServiceTicketFor(eq("blackbox")))
                .thenReturn("ticket");
    }

    @Test
    void getProvidersList() {
        // given
        mockAuth(null);
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(new BackendDeviceInfo("device1", Platform.YANDEXSTATION,
                        Set.of("kinopoisk_a6m_plus6m", "ivi60"), "device name", null, Instant.EPOCH)));

        // when
        ProviderListResponse providerList = testRestTemplate.getForObject(url("provider")
                        .pathSegment("all")
                        .build()
                        .toUri(),
                ProviderListResponse.class);

        // then
        assertEquals(new ProviderListResponse(false, true), providerList);
    }

    @Test
    void getPromoInfoPersonalPlus() {
        // given
        mockAuth(null);
        BackendDeviceInfo device2 = new BackendDeviceInfo("device2", Platform.YANDEXMICRO,
                Set.of("plus90"), "device name 2", null, Instant.EPOCH);

        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(device2));

        // when
        PromoInfoResponse actual = testRestTemplate.getForObject(url("promo")
                        .build()
                        .toUri(),
                PromoInfoResponse.class);

        // then
        var expected = new PromoInfoResponse(
                Set.of(),
                Set.of(new DeviceShortInfo(device2.getId(), device2.getPlatform(), device2.getName(), plus90))
        );
        assertEquals(expected, actual);
    }

    @Test
    void getPromoInfoMultiPlus() {
        // given
        mockAuth(null);
        BackendDeviceInfo device1 = new BackendDeviceInfo("device1", Platform.YANDEXSTATION,
                Set.of("kinopoisk_a6m_plus6m", "ivi60"), "device name", null, Instant.EPOCH);

        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(device1));

        // when
        PromoInfoResponse actual = testRestTemplate.getForObject(url("promo")
                        .build()
                        .toUri(),
                PromoInfoResponse.class);

        // then
        var expected = new PromoInfoResponse(
                Set.of(new DeviceShortInfo(device1.getId(), device1.getPlatform(), device1.getName(),
                        kinopoisk_a6m_plus6m)),
                Set.of()
        );
        assertEquals(expected, actual);
    }

    @Test
    void getPromoInfoTwoDevices() {
        // given
        mockAuth(null);
        BackendDeviceInfo device1 = new BackendDeviceInfo("device1", Platform.YANDEXSTATION,
                Set.of("kinopoisk_a6m_plus6m", "ivi60"), "device name", null, Instant.EPOCH);
        BackendDeviceInfo device2 = new BackendDeviceInfo("device2", Platform.YANDEXMICRO,
                Set.of("plus90"), "device name 2", null, Instant.EPOCH);

        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(device1, device2));

        // when
        PromoInfoResponse actual = testRestTemplate.getForObject(url("promo")
                        .build()
                        .toUri(),
                PromoInfoResponse.class);

        // then
        var expected = new PromoInfoResponse(
                Set.of(new DeviceShortInfo(device1.getId(), device1.getPlatform(), device1.getName(),
                        kinopoisk_a6m_plus6m)),
                Set.of(new DeviceShortInfo(device2.getId(), device2.getPlatform(), device2.getName(), plus90))
        );
        assertEquals(expected, actual);
    }

    @Test
    void getProvidersListForIvi() {
        // given
        doReturn(225).when(laasClient).getCountryId();
        mockAuth(null);
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(new BackendDeviceInfo("device1", Platform.YANDEXSTATION,
                        Set.of("plus360", "ivi60", "amediateka90"), "device name", null, Instant.EPOCH)));

        jdbcTemplate.update("insert into UsedDevicePromo (id, uid, deviceId, platform, provider, codeid," +
                        "promoActivationTime) values (?, ?, ?, ?, ?, ?, ?)",
                -100, 999, "device1", Platform.YANDEXSTATION.toString(), "ivi", null, null);

        // when
        ProviderListResponse providerList;
        try {
            providerList = testRestTemplate.getForObject(url("provider")
                            .pathSegment("all")
                            .build()
                            .toUri(),
                    ProviderListResponse.class);
        } finally {
            jdbcTemplate.update("delete from UsedDevicePromo where id = -100");
        }

        // then
        assertEquals(new ProviderListResponse(true, true), providerList);
    }

    @Test
    void getProvidersListUserWithNewAndOldStation() {
        // given
        mockAuth(null);
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(
                                new BackendDeviceInfo("device1", Platform.YANDEXSTATION, Set.of(plus360.name()),
                                        "device name", null, Instant.EPOCH),
                                new BackendDeviceInfo("device2", Platform.YANDEXSTATION,
                                        Set.of(kinopoisk_a6m_plus6m.name()),
                                        "device name", null, Instant.EPOCH)
                        )
                );

        // when
        ProviderListResponse providerList = testRestTemplate.getForObject(url("provider")
                        .pathSegment("all")
                        .build()
                        .toUri(),
                ProviderListResponse.class);

        // then
        assertEquals(new ProviderListResponse(true, true), providerList);
    }

    @Test
    void getProvidersListUserWithOnlyNewStation() {
        // given
        mockAuth(null);
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(
                                new BackendDeviceInfo("device2", Platform.YANDEXSTATION,
                                        Set.of(kinopoisk_a6m_plus6m.name()),
                                        "device name", null, Instant.EPOCH)
                        )
                );

        // when
        ProviderListResponse providerList = testRestTemplate.getForObject(url("provider")
                        .pathSegment("all")
                        .build()
                        .toUri(),
                ProviderListResponse.class);

        // then
        assertEquals(new ProviderListResponse(false, true), providerList);
    }

    @Test
    void getProvidersListOnBackendFailure() {
        // given
        mockAuth(null);
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenThrow(new QuasarBackendException(HttpStatus.SERVICE_UNAVAILABLE));

        // when
        ProviderListResponse providerList = testRestTemplate.getForObject(url("provider")
                        .pathSegment("all")
                        .build()
                        .toUri(),
                ProviderListResponse.class);

        // then
        // as backend responded with 503
        assertFalse(providerList.plusPromoAvailable());
    }

    @Test
    void getYandexPlusProviderInfo() {
        mockAuth(iviConfig);
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(new BackendDeviceInfo("device1", Platform.YANDEXSTATION, Set.of(plus360.name()),
                        "device name", null, Instant.EPOCH)));
        ProviderInfoResponse providerInfo = testRestTemplate.getForObject(url("provider")
                        .pathSegment("yandexplus")
                        .build()
                        .toUri(),
                ProviderInfoResponse.class);

        assertEquals("yandexplus", providerInfo.getProviderName());
        assertNull(providerInfo.getSocialAPIServiceName());
        assertNull(providerInfo.getSocialAPIClientId());
        assertTrue(providerInfo.isAuthorized());
        assertEquals(Set.of(
                        new ProviderInfoResponse.DeviceShortInfo("device1", Platform.YANDEXSTATION, "device name",
                                plus360, null)),
                providerInfo.getDevicesWithPromo());
    }

    @Test
    void getYandexPlusProviderInfoWithExperiment() throws ParseException {
        mockAuth(iviConfig);
        Instant now = Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-09-10T00:00:00.000000Z"));
        // We add to 09-10 7 days, and get 09-17, after that we round it up to 09-18
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenReturn(List.of(new BackendDeviceInfo("device1", Platform.YANDEXMINI, Set.of(plus90.name()),
                        "device name", null, now.minus(8, ChronoUnit.DAYS))));
        when(authorizationContext.getRequestExperiments())
                .thenReturn(Set.of(Experiments.PLUS120_EXPERIMENT));
        when(authorizationContext.getRequestTimestamp())
                .thenReturn(now);
        ProviderInfoResponse providerInfo = testRestTemplate.getForObject(url("provider")
                        .pathSegment("yandexplus")
                        .build()
                        .toUri(),
                ProviderInfoResponse.class);

        assertEquals("yandexplus", providerInfo.getProviderName());
        assertNull(providerInfo.getSocialAPIServiceName());
        assertNull(providerInfo.getSocialAPIClientId());
        assertTrue(providerInfo.isAuthorized());
        assertEquals(Set.of(new ProviderInfoResponse.DeviceShortInfo("device1", Platform.YANDEXMINI, "device name",
                        plus120, FORMATTER.format(now.plus(7, ChronoUnit.DAYS)))),
                providerInfo.getDevicesWithPromo());
    }

    @Test
    void getYandexPlusProviderInfoOnBackendUnavailable() {
        // given
        mockAuth(iviConfig);
        when(quasarBackendClient.getUserDeviceList(uid))
                .thenThrow(new QuasarBackendException(HttpStatus.SERVICE_UNAVAILABLE));

        // when
        ResponseEntity<String> providerInfo = testRestTemplate.getForEntity(url("provider")
                        .pathSegment("yandexplus")
                        .build()
                        .toUri(),
                String.class);

        // then
        // if backend is unavailable we should fail here
        assertEquals(HttpStatus.INTERNAL_SERVER_ERROR, providerInfo.getStatusCode());
    }

    @Test
    void testActivatePromoOnDevice() {
        mockAuth(null);
        DeviceId deviceId = DeviceId.create("d1", Platform.YANDEXSTATION);
        doReturn(DevicePromoActivationResult.SUCCESS)
                .when(quasarPromoService).activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, "127.0.0.1",
                        deviceId,
                        "card-id");

        ResponseEntity<DevicePromoActivationResponse> responseEntity = executeCall(deviceId);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(DevicePromoActivationResult.SUCCESS, responseEntity.getBody().result());
    }

    @Test
    void testActivatePromoOnDeviceFailure() {
        mockAuth(null);
        DeviceId deviceId = DeviceId.create("d1", Platform.YANDEXSTATION);

        doReturn(DevicePromoActivationResult.ALREADY_ACTIVATED)
                .when(quasarPromoService).activatePromoPeriodFromPP(eq(PromoProvider.yandexplus), eq(uid), anyString(),
                        eq(deviceId), eq(CARD_ID));

        ResponseEntity<DevicePromoActivationResponse> responseEntity = executeCall(deviceId);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(DevicePromoActivationResult.ALREADY_ACTIVATED, responseEntity.getBody().result());
    }

    @Test
    void testActivatePromoOnDeviceWrongRegion() {
        mockAuth(null);
        DeviceId deviceId = DeviceId.create("d1", Platform.YANDEXSTATION);
        doReturn(DevicePromoActivationResult.NOT_ALLOWED_IN_CURRENT_REGION)
                .when(quasarPromoService).activatePromoPeriodFromPP(eq(PromoProvider.yandexplus), eq(uid), anyString(),
                        eq(deviceId), eq(CARD_ID));

        ResponseEntity<DevicePromoActivationResponse> responseEntity = executeCall(deviceId);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(DevicePromoActivationResult.NOT_ALLOWED_IN_CURRENT_REGION, responseEntity.getBody().result());
    }

    @Test
    void testActivatePromoOnDeviceCampaignRestrictions() {
        mockAuth(null);
        DeviceId deviceId = DeviceId.create("d1", Platform.YANDEXSTATION);
        doReturn(DevicePromoActivationResult.USER_HAS_TEMPORARY_CAMPAIGN_RESTRICTIONS)
                .when(quasarPromoService).activatePromoPeriodFromPP(
                        eq(PromoProvider.yandexplus),
                        eq(uid), anyString(),
                        eq(deviceId),
                        eq(CARD_ID)
                );

        ResponseEntity<DevicePromoActivationResponse> responseEntity = executeCall(deviceId);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(
                DevicePromoActivationResult.USER_HAS_TEMPORARY_CAMPAIGN_RESTRICTIONS,
                responseEntity.getBody().result()
        );
    }

    @Test
    void testActivatePromoOnConflict() throws InterruptedException {
        mockAuth(null);
        DeviceId deviceId = DeviceId.create("d1", Platform.YANDEXSTATION);
        CountDownLatch latch = new CountDownLatch(1);
        CountDownLatch latch2 = new CountDownLatch(1);

        doAnswer(answer -> {
            latch2.countDown();
            latch.await();
            return DevicePromoActivationResult.SUCCESS;
        })
                .when(quasarPromoService).activatePromoPeriodFromPP(eq(PromoProvider.yandexplus), eq(uid), anyString(),
                        eq(deviceId), eq(CARD_ID));


        CompletableFuture<ResponseEntity<DevicePromoActivationResponse>> future =
                CompletableFuture.supplyAsync(() -> executeCall(deviceId));

        // start when first thread locks
        latch2.await();
        ResponseEntity<DevicePromoActivationResponse> responseEntity2 = executeCall(deviceId);

        // let first request continue
        latch.countDown();

        ResponseEntity<DevicePromoActivationResponse> responseEntity = future.join();

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(DevicePromoActivationResult.SUCCESS, responseEntity.getBody().result());

        assertEquals(HttpStatus.CONFLICT, responseEntity2.getStatusCode());
    }

    @Test
    void testActivatePromoOnDeviceTemporaryBanned() {
        mockAuth(null);
        DeviceId deviceId = DeviceId.create("d1", Platform.YANDEXSTATION);
        doReturn(DevicePromoActivationResult.USER_TEMPORARY_BANNED)
                .when(quasarPromoService).activatePromoPeriodFromPP(eq(PromoProvider.yandexplus), eq(uid), anyString(),
                        eq(deviceId), eq(CARD_ID));

        ResponseEntity<DevicePromoActivationResponse> responseEntity = executeCall(deviceId);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals(DevicePromoActivationResult.USER_TEMPORARY_BANNED, responseEntity.getBody().result());
    }

    private ResponseEntity<DevicePromoActivationResponse> executeCall(DeviceId deviceId) {
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Authorization", "OAuth some_token");

        return testRestTemplate.exchange(url("provider").pathSegment("yandexplus", "activatePromoOnDevice")
                        .queryParam("deviceId", deviceId.getId())
                        .queryParam("platform", deviceId.getPlatform())
                        .queryParam("paymentCardId", CARD_ID)
                        .queryParam("userEmail", USER_EMAIL)
                        .build().toUri(),
                HttpMethod.POST,
                new HttpEntity<>(headers),
                DevicePromoActivationResponse.class);
    }

    private void mockAuth(@Nullable ProviderConfig providerConfig) {
        doReturn(uid).when(authorizationService).getSecretUid(any(HttpServletRequest.class));
        if (providerConfig != null) {
            doReturn(Optional.of("session")).when(authorizationService).getProviderTokenByUid(eq(uid),
                    eq(providerConfig.getSocialAPIServiceName()));
        }
    }

}
