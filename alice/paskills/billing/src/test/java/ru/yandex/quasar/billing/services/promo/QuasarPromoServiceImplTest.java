package ru.yandex.quasar.billing.services.promo;

import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.time.temporal.ChronoUnit;
import java.util.Collection;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;

import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.Experiments;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.PromoCodeBase;
import ru.yandex.quasar.billing.dao.PromoCodeBaseDao;
import ru.yandex.quasar.billing.dao.PromocodePrototypeDao;
import ru.yandex.quasar.billing.dao.PromocodePrototypeDb;
import ru.yandex.quasar.billing.dao.QuasarPromoBlacklistDao;
import ru.yandex.quasar.billing.dao.UsedDevicePromo;
import ru.yandex.quasar.billing.dao.UsedDevicePromoDao;
import ru.yandex.quasar.billing.dao.UserSubscriptionsDAO;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.TestContentProvider;
import ru.yandex.quasar.billing.providers.amediateka.AmediatekaContentProvider;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.OfferCardData;
import ru.yandex.quasar.billing.services.laas.LaasClient;
import ru.yandex.quasar.billing.services.mediabilling.MediaBillingClient;
import ru.yandex.quasar.billing.services.mediabilling.MediaBillingClient.MusicPromoActivationResult;
import ru.yandex.quasar.billing.services.mediabilling.PromoCodeActivationResult;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region;
import ru.yandex.quasar.billing.services.sup.MobilePushService;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static java.util.Collections.emptyMap;
import static java.util.Collections.emptySet;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.hamcrest.Matchers.emptyIterable;
import static org.hamcrest.Matchers.hasItem;
import static org.hamcrest.Matchers.not;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.beans.PromoType.amediateka90;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a3m;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a3m_multi;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a3m_to_kpa;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a6m;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a6m_plus6m;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a6m_to_kpa;
import static ru.yandex.quasar.billing.beans.PromoType.plus120;
import static ru.yandex.quasar.billing.beans.PromoType.plus180;
import static ru.yandex.quasar.billing.beans.PromoType.plus180_il;
import static ru.yandex.quasar.billing.beans.PromoType.plus30_multi;
import static ru.yandex.quasar.billing.beans.PromoType.plus360;
import static ru.yandex.quasar.billing.beans.PromoType.plus360_by;
import static ru.yandex.quasar.billing.beans.PromoType.plus360_kz;
import static ru.yandex.quasar.billing.beans.PromoType.plus90;
import static ru.yandex.quasar.billing.beans.PromoType.plus90_149rub;
import static ru.yandex.quasar.billing.beans.PromoType.plus90_multi;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.BY;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.KZ;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.RU;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.ALREADY_ACTIVATED;
import static ru.yandex.quasar.billing.services.promo.Platform.ELARI_A98;
import static ru.yandex.quasar.billing.services.promo.Platform.LINKPLAY_A98;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXMICRO;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXMINI;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXMINI_2;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXMODULE;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXSTATION;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXSTATION_2;
import static ru.yandex.quasar.billing.services.promo.Platform.create;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.amediateka;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.kinopoisk;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.yandexplus;
import static ru.yandex.quasar.billing.services.promo.QuasarPromoServiceImpl.DROIDEKA_PROMO;
import static ru.yandex.quasar.billing.services.promo.QuasarPromoServiceImpl.RUS_ID;
import static ru.yandex.quasar.billing.services.promo.QuasarPromoServiceImpl.WAIT_BETWEEN_CODES_MS;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = TestConfigProvider.class)
@MockBean(TvmHeaders.class)
class QuasarPromoServiceImplTest implements PricingOptionTestUtil {
    private final String userEmail = "ya@ya.ru";
    private final String uid = "9999";
    private final String uid2 = "99992";
    private final String userIp = "0.0.0.0";
    private final String deviceId = "devide1";
    private final String device2Id = "devide2";
    private final String irbisDeviceId = "irbisDeviceId";
    private final String wrongDeviceId = "devide2_WRONG";
    // device for new promo offer (6+6)
    private final String newStationDeviceId = "stationNew";
    private final String newActivatedStationDeviceId = "newActivatedStationDeviceId";
    private final String moduleDeviceId = "module1";
    private final String myBelarusStationId = "balorus_station";
    private final String myBelarusStationId2 = "balorus_station2";
    private final String myKazakhstanStationId = "kaza_station";
    private final String myKazakhstanStationId2 = "kaza_station2";
    private final String myIzraelStationId = "izrael_station";
    private final String stationMiniRusId = "station_mini_rus";
    private final String newStationMiniRusId = "new_station_mini";
    private final String newStationMicroRusId = "new_station_micro";
    private final String deviceName = "name";
    private final String iviSession = "ivi_session";
    private final String myTvId = "621c16c5af8f78b6f46d";
    private final DeviceId myDevice = DeviceId.create(deviceId, YANDEXSTATION);
    private final DeviceId myIrbisDevice = DeviceId.create(irbisDeviceId, LINKPLAY_A98);
    private final DeviceId myElariDevice = DeviceId.create(irbisDeviceId, ELARI_A98);
    private final DeviceId myActivatedDevice = DeviceId.create(device2Id, YANDEXSTATION);
    private final DeviceId wrongDevice = DeviceId.create(wrongDeviceId, YANDEXSTATION);
    private final DeviceId newStation = DeviceId.create(newStationDeviceId, YANDEXSTATION);
    private final DeviceId newActivatedStation = DeviceId.create(newActivatedStationDeviceId, YANDEXSTATION);
    private final DeviceId myModuleDevice = DeviceId.create(moduleDeviceId, YANDEXMODULE);
    private final DeviceId myBelarusStation = DeviceId.create(myBelarusStationId, YANDEXSTATION);
    private final DeviceId myBelarusStation2 = DeviceId.create(myBelarusStationId2, YANDEXSTATION);
    private final DeviceId myKazakhstanStation = DeviceId.create(myKazakhstanStationId, YANDEXSTATION);
    private final DeviceId myKazakhstanStation2 = DeviceId.create(myKazakhstanStationId2, YANDEXSTATION);
    private final DeviceId myIzraelStation = DeviceId.create(myIzraelStationId, YANDEXSTATION);
    private final DeviceId myStationMiniRus = DeviceId.create(stationMiniRusId, YANDEXMINI);
    private final DeviceId myNewStationMini = DeviceId.create(newStationMiniRusId, YANDEXMINI);
    private final DeviceId myNewStationMicro = DeviceId.create(newStationMicroRusId, YANDEXMICRO);
    private final DeviceId myTv = DeviceId.create(myTvId, create("yandex_tv_rt2871_hikeen"));
    private final PaymentMethod validCard = PaymentMethod.builder("card-1", "card")
            .account("123123123")
            .expired(false)
            .system("MasterCard")
            .expired(false)
            .build();
    @Autowired
    private QuasarPromoServiceImpl quasarPromoService;
    @MockBean
    private QuasarBackendClient quasarBackendClient;
    @MockBean
    private MediaBillingClient mediaBillingClient;
    @MockBean
    private BillingService billingService;
    @MockBean
    private MobilePushService mobilePushService;
    @MockBean
    private QuasarPromoBlacklistDao quasarPromoBlacklistDao;
    @Autowired
    private PromoCodeBaseDao promoCodeBaseDao;
    @Autowired
    private PromocodePrototypeDao promocodePrototypeDao;
    @MockBean
    private DroidekaClient droidekaClient;
    @SpyBean
    private TestContentProvider testContentProvider;
    private final IContentProvider iviContentProvider = mock(IContentProvider.class);
    @SpyBean
    private AmediatekaContentProvider amediatekaContentProvider;
    @MockBean
    private AuthorizationService authorizationService;
    @MockBean
    private UserSubscriptionsDAO userSubscriptionsDAO;
    @Autowired
    private UsedDevicePromoDao usedDevicePromoDao;
    @MockBean
    private LaasClient laasClient;
    @SpyBean
    private AuthorizationContext authorizationContext;
    private BackendDeviceInfo myDeviceBackendRecord;
    private BackendDeviceInfo myIrbisDeviceBackendRecord;
    private BackendDeviceInfo myActivatedDeviceBackendRecord;
    private BackendDeviceInfo myElariDeviceBackendRecord;
    private BackendDeviceInfo myModuleDeviceBackendRecord;
    private BackendDeviceInfo newStationBackendRecord;
    private BackendDeviceInfo newActivatedStationBackendRecord;
    private BackendDeviceInfo myBelarusStationDeviceBackendRecord;
    private BackendDeviceInfo myBelarusStationDeviceBackendRecord2;
    private BackendDeviceInfo myKazakhstanStationDeviceBackendRecord;
    private BackendDeviceInfo myKazakhstanStationDeviceBackendRecord2;
    private BackendDeviceInfo myIzraelStationDeviceBackendRecord;
    private BackendDeviceInfo stationMiniRusDeviceBackendRecord;
    private BackendDeviceInfo myTvRecord;
    private BackendDeviceInfo myTvEthernetRecord;
    private BackendDeviceInfo stationMiniActivatedOnSomeFixedWeekAgo;
    private BackendDeviceInfo stationMiniEmptyActivationDate;
    private BackendDeviceInfo stationMicroActivatedOnSomeFixedWeekAgo;
    private final PaymentProcessor trust = PaymentProcessor.TRUST;
    private PromoCodeBase amediatekaCode;
    private PromoCodeBase miniSpecialCode;
    private final PromoProvider droidekaProvider = DROIDEKA_PROMO.getProvider();
    private Instant someFixedWeekAgoInstant;
    private Instant someFixedInstant;

    @BeforeEach
    void setUp() {
        myDeviceBackendRecord = new BackendDeviceInfo(myDevice.getId(), myDevice.getPlatform(),
                Set.of("plus360", "tag1", "ivi60", "amediateka90"), deviceName, null);
        myIrbisDeviceBackendRecord = new BackendDeviceInfo(myIrbisDevice.getId(), myIrbisDevice.getPlatform(),
                Set.of("plus180", "tag2"), deviceName, null);
        myElariDeviceBackendRecord = new BackendDeviceInfo(myElariDevice.getId(), myElariDevice.getPlatform(),
                Set.of("plus90", "tag2"), deviceName, null);
        myActivatedDeviceBackendRecord = new BackendDeviceInfo(myActivatedDevice.getId(),
                myActivatedDevice.getPlatform(), Set.of(), deviceName, null);
        myModuleDeviceBackendRecord = new BackendDeviceInfo(myModuleDevice.getId(), myModuleDevice.getPlatform(),
                Set.of("kinopoisk_a3m"), deviceName, null);
        newStationBackendRecord = new BackendDeviceInfo(newStation.getId(), newStation.getPlatform(),
                Set.of("kinopoisk_a6m_plus6m"), deviceName, null);
        newActivatedStationBackendRecord = new BackendDeviceInfo(newActivatedStation.getId(),
                newActivatedStation.getPlatform(), Set.of(), deviceName, null);
        myBelarusStationDeviceBackendRecord = new BackendDeviceInfo(myBelarusStation.getId(),
                myBelarusStation.getPlatform(), Set.of("plus360_by"), deviceName, null);
        myBelarusStationDeviceBackendRecord2 = createDevice(myBelarusStation2, Set.of("kinopoisk_a6m_plus6m"), BY);
        myKazakhstanStationDeviceBackendRecord = new BackendDeviceInfo(myKazakhstanStation.getId(),
                myKazakhstanStation.getPlatform(), Set.of("plus360_kz"), deviceName, null);
        myKazakhstanStationDeviceBackendRecord2 = createDevice(myKazakhstanStation2, Set.of("kinopoisk_a6m_plus6m"),
                KZ);
        stationMiniRusDeviceBackendRecord = new BackendDeviceInfo(myStationMiniRus.getId(),
                myStationMiniRus.getPlatform(), Set.of("plus90"), deviceName, RU);
        myIzraelStationDeviceBackendRecord = new BackendDeviceInfo(myIzraelStation.getId(),
                myIzraelStation.getPlatform(), Set.of("kinopoisk_a6m_plus6m"), deviceName, "IL");
        myTvRecord = BackendDeviceInfo.create(myTvId, myTv.getPlatform(), Set.of(DROIDEKA_PROMO.name()),
                null, "serial", "40:AA:56:3B:28:48", "B0:D5:68:C8:C7:7B");
        myTvEthernetRecord = BackendDeviceInfo.create(myTvId, myTv.getPlatform(), Set.of(DROIDEKA_PROMO.name()),
                null, "serial", "02:00:00:00:00:00", "B0:D5:68:C8:C7:7B");
        someFixedInstant = Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-09-10T01:02:03.000000Z"));
        someFixedWeekAgoInstant = someFixedInstant.minus(7, ChronoUnit.DAYS);
        stationMiniActivatedOnSomeFixedWeekAgo = new BackendDeviceInfo(myNewStationMini.getId(),
                myNewStationMini.getPlatform(), Set.of("plus90"), deviceName, RU, someFixedWeekAgoInstant);
        stationMiniEmptyActivationDate = new BackendDeviceInfo(myNewStationMini.getId(),
                myNewStationMini.getPlatform(), Set.of("plus90"), deviceName, RU, null);
        stationMicroActivatedOnSomeFixedWeekAgo = new BackendDeviceInfo(myNewStationMicro.getId(),
                myNewStationMicro.getPlatform(), Set.of("plus90"), deviceName, RU, someFixedWeekAgoInstant);

        when(quasarBackendClient.getUserDeviceList(anyString()))
                .thenThrow(new HttpClientErrorException(HttpStatus.FORBIDDEN));
        when(quasarBackendClient.getUserDeviceList(eq(uid)))
                .then(invocation -> List.of(
                        myDeviceBackendRecord,
                        myActivatedDeviceBackendRecord,
                        myIrbisDeviceBackendRecord,
                        myElariDeviceBackendRecord,
                        myModuleDeviceBackendRecord,
                        newStationBackendRecord,
                        newActivatedStationBackendRecord,
                        myBelarusStationDeviceBackendRecord,
                        myBelarusStationDeviceBackendRecord2,
                        myKazakhstanStationDeviceBackendRecord,
                        myKazakhstanStationDeviceBackendRecord2,
                        stationMiniRusDeviceBackendRecord,
                        myIzraelStationDeviceBackendRecord
                ));

        when(billingService.getCardsList(anyString(), eq(trust), any())).thenReturn(List.of());
        when(billingService.getCardsList(anyString(), eq(PaymentProcessor.MEDIABILLING), any())).thenReturn(List.of());
        promoCodeBaseDao.save(PromoCodeBase.create(yandexplus.name(), plus360, "code1"));
        promoCodeBaseDao.save(PromoCodeBase.create(yandexplus.name(), plus180, "code2"));
        promoCodeBaseDao.save(PromoCodeBase.create(yandexplus.name(), plus90, "plus90code2"));
        //promoCodeBaseDao.save(PromoCodeBase.create(ivi.name(), PromoType.ivi60, "code3"));
        amediatekaCode = promoCodeBaseDao.save(PromoCodeBase.create(kinopoisk.name(), amediateka90, "code4"));
        promoCodeBaseDao.save(PromoCodeBase.create(kinopoisk.name(), kinopoisk_a3m, "code-amedia-3m"));
        promoCodeBaseDao.save(PromoCodeBase.create(kinopoisk.name(), kinopoisk_a6m_plus6m, "code_a6m" +
                "|code_plus6m"));
        promoCodeBaseDao.save(PromoCodeBase.create(yandexplus.name(), plus360_by, "code_plus360_by"));
        promoCodeBaseDao.save(PromoCodeBase.create(yandexplus.name(), plus360_kz, "code_plus360_kz"));

        promocodePrototypeDao.save(new PromocodePrototypeDb(null, myIzraelStation.getPlatform().toString(),
                plus180_il.name(), "IL_CODE_PROTOTYPE", "TASK_ID"));

        promocodePrototypeDao.save(new PromocodePrototypeDb(null, myTv.getPlatform().toString(),
                plus90_149rub.name(), "PLUS90_149RUB_CODE", "TASK_ID"));
        promocodePrototypeDao.save(new PromocodePrototypeDb(null, myTv.getPlatform().toString(),
                plus90.name(), "PLUS90_TV_CODE", "TASK_ID"));

        miniSpecialCode = promoCodeBaseDao.save(PromoCodeBase.create(yandexplus.name(), plus90, "plus90code2ForMini",
                YANDEXMINI));

        when(quasarPromoBlacklistDao.userInBlacklist(anyString())).thenReturn(false);

        when(authorizationService.getProviderTokenByUid(uid, iviContentProvider.getSocialAPIServiceName()))
                .thenReturn(Optional.of(iviSession));
        doReturn(emptyMap())
                .when(iviContentProvider).getActiveSubscriptions(iviSession);

        when(billingService.getCardsList(eq(uid), eq(PaymentProcessor.MEDIABILLING), any()))
                .thenReturn(List.of(validCard));
        when(billingService.getCardsList(eq(uid), eq(PaymentProcessor.TRUST), any())).thenReturn(List.of(validCard));
        when(mediaBillingClient.activatePromoCode(eq(Long.valueOf(uid)), anyString(), eq(validCard.getId()),
                anyString(), any()))
                .thenReturn(new PromoCodeActivationResult(MusicPromoActivationResult.SUCCESS));
        when(laasClient.getCountryId()).thenReturn(RUS_ID);
        when(mediaBillingClient.clonePrototype(anyString()))
                .then(inv -> inv.getArgument(0).toString() + "_clone");

        when(droidekaClient.getGiftState(any(), anyString(), anyString()))
                .thenReturn(DroidekaClient.GiftState.AVAILABLE);
        when(droidekaClient.getGiftStateAsync(any(), anyString(), anyString()))
                .thenReturn(CompletableFuture.completedFuture(DroidekaClient.GiftState.AVAILABLE));

        quasarPromoService.loadPrototypes();
    }

    private BackendDeviceInfo createDevice(DeviceId deviceId, Set<String> tags) {
        return createDevice(deviceId, tags, null, Instant.EPOCH);
    }

    private BackendDeviceInfo createDevice(DeviceId deviceId, Set<String> tags, String region) {
        return createDevice(deviceId, tags, region, Instant.EPOCH);
    }

    private BackendDeviceInfo createDevice(DeviceId deviceId, Set<String> tags,
                                           @Nullable String region, Instant firstActivation) {
        return new BackendDeviceInfo(deviceId.getId(), deviceId.getPlatform(), tags, deviceName, region,
                firstActivation);
    }

    @Test
    void testDevicePromoAvailableOnWrongDevice() {

        try {
            quasarPromoService.getUserDevice(uid, DeviceId.create(wrongDeviceId, YANDEXSTATION));
            fail("DeviceNotOwnedByUserException not thrown");
        } catch (DeviceNotOwnedByUserException e) {
            assertNotNull(e);
        }
    }

    @Test
    void testDevicePromoAvailableOnActivatedByBackendDevice() throws DeviceNotOwnedByUserException {
        Collection<DeviceInfo> deviceInfos = quasarPromoService.getUserDevices(uid);

        assertThat(deviceInfos, containsInAnyOrder(List.of(
                deviceInfo(myDevice, Map.of(yandexplus, plus360, kinopoisk, amediateka90)),
                deviceInfo(myActivatedDevice, Map.of()),
                deviceInfo(myIrbisDevice, Map.of(yandexplus, plus180)),
                deviceInfo(myElariDevice, Map.of(yandexplus, plus90)),
                deviceInfo(myModuleDevice, Map.of(kinopoisk, kinopoisk_a3m)),
                deviceInfo(newStation, Map.of(kinopoisk, kinopoisk_a6m_plus6m)),
                deviceInfo(newActivatedStation, Map.of()),
                deviceInfo(myBelarusStation, Map.of(yandexplus, plus360_by)),
                deviceInfo(myBelarusStation2, Map.of(yandexplus, plus360_by)),
                deviceInfo(myKazakhstanStation, Map.of(yandexplus, plus360_kz)),
                deviceInfo(myKazakhstanStation2, Map.of(yandexplus, plus360_kz)),
                deviceInfo(myStationMiniRus, Map.of(yandexplus, plus90)),
                deviceInfo(myIzraelStation, Map.of(yandexplus, plus180_il))
        ).toArray()));
    }

    private DeviceInfo deviceInfo(DeviceId newActivatedStation, Map<PromoProvider, PromoType> promos) {
        return new DeviceInfo(newActivatedStation.getId(), newActivatedStation.getPlatform(), deviceName, promos,
                Instant.EPOCH);
    }


    @Test
    void testDevicePlusPromoAvailable() throws DeviceNotOwnedByUserException {
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);

        when(billingService.getCardsList(eq(uid), eq(trust), any())).thenReturn(List.of(validCard));

        assertThat(deviceInfo.getAvailablePromos(), hasItem(yandexplus));

        quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());

        assertEquals(Set.of(kinopoisk), quasarPromoService.getUserDevice(uid, myDevice).getAvailablePromos());


        var activationResult = quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("code1");

    }

    @Test
    void testDeviceKinopoiskPromoActivation() throws DeviceNotOwnedByUserException {
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myModuleDevice);


        assertThat(deviceInfo.getAvailablePromos(), hasItem(kinopoisk));

        quasarPromoService.activatePromoPeriodFromPP(PromoProvider.kinopoisk, uid, userIp,
                deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        assertEquals(Set.of(), quasarPromoService.getUserDevice(uid, myModuleDevice).getAvailablePromos());

        var activationResult = quasarPromoService.activatePromoPeriodFromPP(kinopoisk, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("code-amedia-3m");
    }

    @Test
    @DisplayName("Получение промопериода Станции в Беларуссии")
    void testDeviceBelarusPlusActivation() throws DeviceNotOwnedByUserException {

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myBelarusStation);

        assertThat(deviceInfo.getAvailablePromos(), hasItem(yandexplus));

        quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp, deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        assertEquals(Set.of(), quasarPromoService.getUserDevice(uid, myBelarusStation).getAvailablePromos());

        var activationResult = quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("code_plus360_by");
    }

    @Test
    @DisplayName("Получение промопериода Станции в Казахстане")
    void testDeviceKazakhstanPlusActivation() throws DeviceNotOwnedByUserException {

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myKazakhstanStation);

        assertThat(deviceInfo.getAvailablePromos(), hasItem(yandexplus));

        quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp, deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        assertEquals(Set.of(), quasarPromoService.getUserDevice(uid, myKazakhstanStation).getAvailablePromos());

        var activationResult = quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("code_plus360_kz");
    }

    @Test
    @DisplayName("Получение промопериода СтанцииМини в РФ")
    void testMiniPlusActivation() throws DeviceNotOwnedByUserException {

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myStationMiniRus);

        assertThat(deviceInfo.getAvailablePromos(), hasItem(yandexplus));

        quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp, deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        assertEquals(Set.of(), quasarPromoService.getUserDevice(uid, myStationMiniRus).getAvailablePromos());

        var activationResult = quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("plus90code2ForMini");
    }

    @Test
    void testDevicePromoAvailableButNoCardBound() throws DeviceNotOwnedByUserException, NoCardBoundException {
        when(billingService.getCardsList(eq(uid), eq(PaymentProcessor.MEDIABILLING), any())).thenReturn(List.of());
        when(billingService.getCardsList(eq(uid), eq(PaymentProcessor.TRUST), any())).thenReturn(List.of());

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);

        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());

        try {
            quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                    deviceInfo.getDeviceIdentifier(), validCard.getId());
            fail("NoCardBoundException not thrown");
        } catch (NoCardBoundException e) {
            assertNotNull(e);
        }
    }

    @Test
    void testDevicePromoAvailableButBoundCardExpired() throws DeviceNotOwnedByUserException {
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);

        PaymentMethod expiredCard = PaymentMethod.builder("card-1", "card")
                .account("123123123")
                .expired(false)
                .system("MasterCard")
                .expired(true)
                .build();

        when(billingService.getCardsList(eq(uid), eq(PaymentProcessor.MEDIABILLING), any()))
                .thenReturn(List.of(expiredCard));

        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());

        try {
            quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                    deviceInfo.getDeviceIdentifier(), validCard.getId());
            fail("NoCardBoundException not thrown");
        } catch (NoCardBoundException e) {
            assertNotNull(e);
        }
    }

    @Test
    void testDevicePromoNotAvailableForAnotherUserAfterActivation() throws DeviceNotOwnedByUserException {
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);

        when(billingService.getCardsList(eq(uid), eq(trust), any())).thenReturn(List.of(validCard));
        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());

        quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());

        assertEquals(Set.of(kinopoisk), quasarPromoService.getUserDevice(uid, myDevice).getAvailablePromos());

        // imagine myDivice was now connected to uid2
        when(quasarBackendClient.getUserDeviceList(eq(uid2))).thenReturn(List.of(myDeviceBackendRecord));
        DeviceInfo deviceInfo2 = quasarPromoService.getUserDevice(uid2, myDevice);

        assertEquals(Set.of(kinopoisk), deviceInfo2.getAvailablePromos());
    }

    @Test
    void testPromoActivationOnAnotherAccountAfterFailureOnFirst() throws DeviceNotOwnedByUserException {
        // fail on activating on first account nad ok on second
        when(mediaBillingClient.activatePromoCode(eq(Long.valueOf(uid)), anyString(), eq(validCard.getId()),
                anyString(), any()))
                .thenReturn(new PromoCodeActivationResult(MusicPromoActivationResult.UNKNOWN));

        when(mediaBillingClient.activatePromoCode(eq(Long.valueOf(uid2)), anyString(), eq(validCard.getId()),
                anyString(), any()))
                .thenReturn(new PromoCodeActivationResult(MediaBillingClient.MusicPromoActivationResult.SUCCESS));

        when(billingService.getCardsList(eq(uid), any(PaymentProcessor.class), any())).thenReturn(List.of(validCard));
        when(billingService.getCardsList(eq(uid2), any(PaymentProcessor.class), any())).thenReturn(List.of(validCard));

        // simulate changing device-account association to uid2
        when(quasarBackendClient.getUserDeviceList(eq(uid2))).thenReturn(List.of(myDeviceBackendRecord));

        // then
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);
        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());

        assertThrows(UnknownPromoActivationResultException.class,
                () -> quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                        deviceInfo.getDeviceIdentifier(), validCard.getId()));
        assertEquals(Set.of(yandexplus, kinopoisk),
                quasarPromoService.getUserDevice(uid, myDevice).getAvailablePromos());

        // imagine myDevice was now connected to uid2
        DeviceInfo deviceInfo2 = quasarPromoService.getUserDevice(uid2, myDevice);
        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo2.getAvailablePromos());

        quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid2, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(Set.of(kinopoisk), quasarPromoService.getUserDevice(uid2, myDevice).getAvailablePromos());
    }

    @Test
    void testRequestPlusWithoutAvailablePromos() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(myActivatedDeviceBackendRecord));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);

        // then
        assertEquals(PromoState.PAYMENT_REQUIRED, requestPlusResult.getPromoState());
        assertNull(requestPlusResult.getOfferCardData());
    }

    @Test
    void testRequestPlusWithPlus120ExperimentOnMini() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(
                List.of(stationMiniActivatedOnSomeFixedWeekAgo));
        doReturn(Set.of(Experiments.PLUS120_EXPERIMENT)).when(authorizationContext).getRequestExperiments();
        doReturn(someFixedInstant).when(authorizationContext).getRequestTimestamp();
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);

        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertEquals(requestPlusResult.getExpirationTime(),
                Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-09-18T00:00:00.000000Z")));
        assertNull(requestPlusResult.getOfferCardData());
        assertEquals(requestPlusResult.getExperiment(), plus120.name());
    }

    @Test
    void testRequestPlusWithPlus120ExperimentOnMiniEmptyActivationDate() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(
                List.of(stationMiniEmptyActivationDate));
        doReturn(Set.of(Experiments.PLUS120_EXPERIMENT)).when(authorizationContext).getRequestExperiments();
        doReturn(someFixedInstant).when(authorizationContext).getRequestTimestamp();
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);

        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertNull(requestPlusResult.getExpirationTime());
        assertNull(requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExperiment());
    }

    @Test
    void testRequestPlusWithPlus120ExperimentOnMicro() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(
                List.of(stationMicroActivatedOnSomeFixedWeekAgo));
        doReturn(Set.of(Experiments.PLUS120_EXPERIMENT)).when(authorizationContext).getRequestExperiments();
        doReturn(someFixedInstant).when(authorizationContext).getRequestTimestamp();
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);

        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertEquals(requestPlusResult.getExpirationTime(),
                Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-09-18T00:00:00.000000Z")));
        assertNull(requestPlusResult.getOfferCardData());
        assertEquals(requestPlusResult.getExperiment(), plus120.name());
    }

    @Test
    void testRequestPlusWithAvailablePromos() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(myElariDeviceBackendRecord));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);
        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertNull(requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    void testRequestPlusWithAvailablePromosAndDontSendCardAndPush() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(myElariDeviceBackendRecord));
        when(mobilePushService.generateCardId()).thenReturn("card_id");
        when(mobilePushService.getActivatePromoPeriodUrl(yandexplus.name())).thenReturn("http").thenReturn("http_2");
        when(mobilePushService.getWrappedLinkUrl(any())).thenAnswer(i -> "http_wraped$" + i.getArgument(0));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid, false);
        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        var expectedCard = new OfferCardData("card_id", "http", "Активировать Яндекс.Плюс", 1, 1, emptyMap());
        assertEquals(expectedCard, requestPlusResult.getOfferCardData());
        assertEquals("http_wraped$http_2", requestPlusResult.getActivatePromoUri());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    void testRequestPlusWithKinopoiskAvailablePromosForModule() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(myModuleDeviceBackendRecord));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);
        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertNull(requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    void testRequestPlusWithKinopoiskAvailablePromosForNewStation() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(newStationBackendRecord));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);
        // then
        assertNull(requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    void testRequestPlusWithKinopoiskAvailablePromosForNewStationAndDontSendCard() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(newStationBackendRecord));
        when(mobilePushService.generateCardId()).thenReturn("card_id");
        when(mobilePushService.getActivatePromoPeriodUrl(kinopoisk.name())).thenReturn("http");
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid, false);
        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        var expected = new OfferCardData("card_id", "http", "Активировать Яндекс.Плюс", 1, 1, emptyMap());
        assertEquals(expected, requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    @DisplayName("Запрос Плюса по пользователю со старой станцией, новой станций (6+6) и Модулем")
    void testRequestPlusWithKinopoiskAvailablePromosForOldStationAndNewStationAndModule() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(newStationBackendRecord,
                myModuleDeviceBackendRecord, myDeviceBackendRecord));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);
        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertNull(requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    @DisplayName("Запрос Плюса от пользователя с Белорусской станцией")
    void testRequestPlusAvailablePromosForBelarusNewStation() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid))).thenReturn(List.of(myBelarusStationDeviceBackendRecord));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);
        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertNull(requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    @DisplayName("Запрос Плюса от пользователя с Казахской станцией станцией")
    void testRequestPlusAvailablePromosForKazakhstanNewStation() {
        // given
        when(quasarBackendClient.getUserDeviceList(eq(uid)))
                .thenReturn(List.of(myKazakhstanStationDeviceBackendRecord));
        // when
        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid);
        // then
        assertEquals(PromoState.PROMO_AVAILABLE, requestPlusResult.getPromoState());
        assertNull(requestPlusResult.getOfferCardData());
        assertNull(requestPlusResult.getExpirationTime());
    }

    @Test
    void testPromoActivationOnWrongRegion() throws DeviceNotOwnedByUserException {
        // fail on activating on first account nad ok on second
        when(mediaBillingClient.activatePromoCode(eq(Long.valueOf(uid)), anyString(), eq(validCard.getId()),
                anyString(), any()))
                .thenReturn(
                        new PromoCodeActivationResult(MusicPromoActivationResult.CODE_NOT_ALLOWED_IN_CURRENT_REGION)
                );

        when(billingService.getCardsList(eq(uid), eq(trust), any())).thenReturn(List.of(validCard));

        // then
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);
        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());

        var result = quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(),
                validCard.getId());
        assertEquals(DevicePromoActivationResult.NOT_ALLOWED_IN_CURRENT_REGION, result);
    }


    @Test
    void testPromoActivationOnUserBanned() throws DeviceNotOwnedByUserException {
        // fail on activating on first account nad ok on second
        when(mediaBillingClient.activatePromoCode(eq(Long.valueOf(uid)), anyString(), eq(validCard.getId()),
                anyString(), any()))
                .thenReturn(new PromoCodeActivationResult(MusicPromoActivationResult.USER_TEMPORARY_BANNED));

        when(billingService.getCardsList(eq(uid), eq(trust), any())).thenReturn(List.of(validCard));

        // then
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);
        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());

        var result = quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(),
                validCard.getId());
        assertEquals(DevicePromoActivationResult.USER_TEMPORARY_BANNED, result);
    }

    @Test
    void testDevicePromoAvailableWithIrbis() throws DeviceNotOwnedByUserException {

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);
        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());
        DeviceInfo irbisDeviceInfo = quasarPromoService.getUserDevice(uid, myIrbisDevice);
        assertEquals(EnumSet.of(yandexplus), irbisDeviceInfo.getAvailablePromos());
    }

    @Test
    void testDevicePromoAvailableWithElari() throws DeviceNotOwnedByUserException {

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);
        assertEquals(EnumSet.of(yandexplus, kinopoisk), deviceInfo.getAvailablePromos());
        DeviceInfo elariDeviceInfo = quasarPromoService.getUserDevice(uid, myElariDevice);
        assertEquals(EnumSet.of(yandexplus), elariDeviceInfo.getAvailablePromos());
    }

    @Test
    void testElariDevicePlusPromoActivation() throws DeviceNotOwnedByUserException {
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myElariDevice);

        when(billingService.getCardsList(eq(uid), eq(trust), any())).thenReturn(List.of(validCard));

        assertThat(deviceInfo.getAvailablePromos(), hasItem(yandexplus));

        quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());

        assertEquals(Set.of(), quasarPromoService.getUserDevice(uid, myElariDevice).getAvailablePromos());


        var activationResult = quasarPromoService.activatePromoPeriodFromPP(PromoProvider.yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("plus90code2");
    }

    @Test
    void testDevicePromoAvailableButUserInBlacklist() throws DeviceNotOwnedByUserException {
        when(quasarPromoBlacklistDao.userInBlacklist(eq(uid))).thenReturn(true);
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);

        assertEquals(emptySet(), deviceInfo.getAvailablePromos());
    }

    @Test
    @DisplayName("Активация 6+6 промо для нового пользователя")
    void testActivate66PromoForNewUser() throws InterruptedException {
        // given
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, newStation);


        assertThat(deviceInfo.getAvailablePromos(), hasItem(kinopoisk));

        // when
        quasarPromoService.activatePromoPeriodFromPP(kinopoisk, uid, userIp, deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        //then
        assertEquals(Set.of(), quasarPromoService.getUserDevice(uid, newStation).getAvailablePromos());

        var activationResult = quasarPromoService.activatePromoPeriodFromPP(kinopoisk, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("code_a6m");
        Thread.sleep(WAIT_BETWEEN_CODES_MS + 500);
        verifyCodeActivated("code_plus6m");
    }

    @Test
    @DisplayName("Активация 6+6 промо для пользователя активировавшего плюс")
    void testActivate66PromoNotAvailableForUserActivatedPlus() {
        // given
        usedDevicePromoDao.save(UsedDevicePromo.builder()
                .deviceId(newStation.getId())
                .platform(newStation.getPlatform())
                .provider(yandexplus)
                .uid(Long.valueOf(uid))
                .promoActivationTime(Instant.now())
                .build());

        when(mediaBillingClient.activatePromoCode(eq(Long.valueOf(uid)), anyString(), eq(validCard.getId()),
                anyString(), any()))
                .thenReturn(new PromoCodeActivationResult(MusicPromoActivationResult.SUCCESS));
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, newStation);


        assertThat(deviceInfo.getAvailablePromos(), not(hasItem(kinopoisk)));
    }

    @Test
    @DisplayName("Активация 3м КП промо для пользователя активировавшего Амедиатеку")
    void testKPAmediaPromoNotAvailableForUserActivatedAmediateka() {
        // given
        usedDevicePromoDao.save(UsedDevicePromo.builder()
                .deviceId(myDevice.getId())
                .platform(myDevice.getPlatform())
                .provider(amediateka)
                .uid(Long.valueOf(uid))
                .promoActivationTime(Instant.now())
                .build());

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);


        assertThat(deviceInfo.getAvailablePromos(), not(hasItem(kinopoisk)));
    }

    @Test
    @DisplayName("Активация нового промо КП+А вместо старой амедиатеки")
    void testActivateKinopoiskAmediatekaPromoForOldUser() {
        // given
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myDevice);


        assertThat(deviceInfo.getAvailablePromos(), hasItem(kinopoisk));

        // when
        quasarPromoService.activatePromoPeriodFromPP(kinopoisk, uid, userIp, deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        //then
        assertEquals(Set.of(), quasarPromoService.getUserDevice(uid, myDevice).getAvailablePromos());

        var activationResult = quasarPromoService.activatePromoPeriodFromPP(kinopoisk, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("code4");
    }

    @Test
    @DisplayName("Активация 6+6 на станции, активированной в Белоруссии")
    void testActivateKinopoiskOnByDevice() {
        // given
        // it has kinopoisk_a6m_plus6m tag and BY activation region
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myBelarusStation2);

        assertThat(deviceInfo.getAvailablePromos(), hasItem(yandexplus));

        // when
        quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp, deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        //then
        assertEquals(Set.of(),
                quasarPromoService.getUserDevice(uid, deviceInfo.getDeviceIdentifier()).getAvailablePromos());

        var activationResult = quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());
        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("code_plus360_by");

        // check one cant activate in another country
        myBelarusStationDeviceBackendRecord2 =
                createDevice(myBelarusStationDeviceBackendRecord2.getDeviceIdentifier(),
                        myBelarusStationDeviceBackendRecord2.getTags(), KZ);
        assertThat(quasarPromoService.getUserDevice(uid, myBelarusStation2).getAvailablePromos(), emptyIterable());

        myBelarusStationDeviceBackendRecord2 =
                createDevice(myBelarusStationDeviceBackendRecord2.getDeviceIdentifier(),
                        myBelarusStationDeviceBackendRecord2.getTags(), RU);
        assertThat(quasarPromoService.getUserDevice(uid, myBelarusStation2).getAvailablePromos(), emptyIterable());

        myBelarusStationDeviceBackendRecord2 =
                createDevice(myBelarusStationDeviceBackendRecord2.getDeviceIdentifier(),
                        myBelarusStationDeviceBackendRecord2.getTags(), null);
        assertThat(quasarPromoService.getUserDevice(uid, myBelarusStation2).getAvailablePromos(), emptyIterable());
    }

    @Test
    void testActivatePromoViaPrototype() {
        //given
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myIzraelStation);

        assertThat(deviceInfo.getAvailablePromos(), hasItem(yandexplus));

        // when
        quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp, deviceInfo.getDeviceIdentifier(),
                validCard.getId());

        //then
        assertEquals(Set.of(),
                quasarPromoService.getUserDevice(uid, deviceInfo.getDeviceIdentifier()).getAvailablePromos());
        var activationResult = quasarPromoService.activatePromoPeriodFromPP(yandexplus, uid, userIp,
                deviceInfo.getDeviceIdentifier(), validCard.getId());

        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("IL_CODE_PROTOTYPE" + "_clone");

    }

    @Test
    void testActivateTV() {
        //given
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myTvRecord);

        assertThat(deviceInfo.getAvailablePromos(), hasItem(droidekaProvider));
        assertTrue(myTvRecord.isDroidekaMaintainedDevice());

        // when
        quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvRecord,
                validCard.getId());

        //then
        assertEquals(Set.of(),
                quasarPromoService.getUserDevice(uid, myTvRecord).getAvailablePromos());
        var activationResult =
                quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvRecord,
                        validCard.getId());

        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("PLUS90_TV_CODE" + "_clone");

    }

    @Test
    void testActivateTVWithEthernetConnection() {
        //given
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myTvEthernetRecord);

        assertThat(deviceInfo.getAvailablePromos(), hasItem(droidekaProvider));
        assertTrue(myTvEthernetRecord.isDroidekaMaintainedDevice());

        // when
        quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvEthernetRecord,
                validCard.getId());

        //then
        assertEquals(Set.of(),
                quasarPromoService.getUserDevice(uid, myTvEthernetRecord).getAvailablePromos());
        var activationResult =
                quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvEthernetRecord,
                        validCard.getId());

        // for wifi_mac 02:00:00:00:00:00 droideka doesn't give gift
        verify(droidekaClient, Mockito.never()).getGiftState(any(), any(), any());

        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("PLUS90_TV_CODE" + "_clone");

    }

    @Test
    void testActivateTVWithoutCardAndEmailNoAvailable() {
        // given
        when(droidekaClient.getGiftState(any(), anyString(), anyString()))
                .thenReturn(DroidekaClient.GiftState.NOT_AVAILABLE);
        when(droidekaClient.getGiftStateAsync(any(), anyString(), anyString()))
                .thenReturn(CompletableFuture.completedFuture(DroidekaClient.GiftState.NOT_AVAILABLE));

        assertTrue(myTvRecord.isDroidekaMaintainedDevice());
        // when
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myTvRecord);

        // then
        assertThat(deviceInfo.getAvailablePromos(), not(hasItem(droidekaProvider)));

    }

    @Test
    void testActivateTVWithPlus90() {

        //given
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myTvRecord);


        assertThat(deviceInfo.getAvailablePromos(), hasItem(droidekaProvider));
        assertEquals(plus90, deviceInfo.getAvailablePromoTypes().get(yandexplus));
        assertTrue(myTvRecord.isDroidekaMaintainedDevice());

        // when
        quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvRecord, validCard.getId());

        //then
        assertEquals(Set.of(),
                quasarPromoService.getUserDevice(uid, myTvRecord).getAvailablePromos());
        var activationResult =
                quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvRecord,
                        validCard.getId());

        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("PLUS90_TV_CODE" + "_clone");

    }

    @Test
    void testActivateTVWithPlus90_149ExperimentOnTv() {

        //given
        doReturn(Set.of(Experiments.PLUS90_TV_149RUB)).when(authorizationContext).getRequestExperiments();
        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, myTvRecord);


        assertThat(deviceInfo.getAvailablePromos(), hasItem(droidekaProvider));
        assertEquals(plus90_149rub, deviceInfo.getAvailablePromoTypes().get(yandexplus));
        assertTrue(myTvRecord.isDroidekaMaintainedDevice());

        // when
        quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvRecord, validCard.getId());

        //then
        assertEquals(Set.of(),
                quasarPromoService.getUserDevice(uid, myTvRecord).getAvailablePromos());
        var activationResult =
                quasarPromoService.activatePromoPeriodFromTv(droidekaProvider, uid, userIp, myTvRecord,
                        validCard.getId());

        assertEquals(ALREADY_ACTIVATED, activationResult, "DevicePromoAlreadyUsedException not thrown");

        verifyCodeActivated("PLUS90_149RUB_CODE" + "_clone");

    }

    @ParameterizedTest
    @MethodSource("promoByProviderSource")
    void promoByProviderTest(Platform platform, PromoType tag, String exp, PromoType expectedPromo) {
        when(authorizationContext.getRequestExperiments()).thenReturn(Set.of(exp));
        BackendDeviceInfo deviceInfo = BackendDeviceInfo.create("q", platform, Set.of(tag.name()), Region.RU);

        PromoType actual = quasarPromoService.promosByProvider(deviceInfo, Set.of())
                .values()
                .stream()
                .findFirst()
                .orElse(null);

        assertEquals(expectedPromo, actual);

    }

    static List<Arguments> promoByProviderSource() {
        return List.of(
                Arguments.of(YANDEXMINI_2, plus90, Experiments.PLUS_MULTI_3M, plus90_multi),
                Arguments.of(YANDEXMINI_2, plus90, Experiments.PLUS_MULTI_1M, plus30_multi),
                Arguments.of(YANDEXMICRO, plus90, Experiments.PLUS_MULTI_3M, plus90_multi),
                Arguments.of(YANDEXMICRO, plus90, Experiments.PLUS_MULTI_1M, plus30_multi),
                Arguments.of(YANDEXSTATION_2, kinopoisk_a6m, Experiments.KP_A_3M, kinopoisk_a3m_multi),
                Arguments.of(YANDEXSTATION_2, kinopoisk_a6m, Experiments.KP_A_3M_KPA, kinopoisk_a3m_to_kpa),
                Arguments.of(YANDEXSTATION_2, kinopoisk_a6m, Experiments.KP_A_6M_KPA, kinopoisk_a6m_to_kpa)
        );
    }


    private void verifyCodeActivated(String code) {
        verify(mediaBillingClient).activatePromoCode(eq(Long.valueOf(uid)), eq(code), eq(validCard.getId()),
                anyString(), any());
    }


}
