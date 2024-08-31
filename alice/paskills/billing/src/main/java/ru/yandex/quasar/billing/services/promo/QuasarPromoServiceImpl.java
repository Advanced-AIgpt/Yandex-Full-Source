package ru.yandex.quasar.billing.services.promo;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Collection;
import java.util.Collections;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Stopwatch;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;
import org.springframework.transaction.PlatformTransactionManager;
import org.springframework.transaction.support.TransactionTemplate;

import ru.yandex.quasar.billing.beans.Experiments;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.dao.PromoCodeBase;
import ru.yandex.quasar.billing.dao.PromoCodeBaseDao;
import ru.yandex.quasar.billing.dao.PromocodePrototypeDao;
import ru.yandex.quasar.billing.dao.QuasarPromoBlacklistDao;
import ru.yandex.quasar.billing.dao.UsedDevicePromo;
import ru.yandex.quasar.billing.dao.UsedDevicePromoDao;
import ru.yandex.quasar.billing.exception.InternalErrorException;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.OfferCardData;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.laas.LaasClient;
import ru.yandex.quasar.billing.services.mediabilling.MediaBillingClient;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.sup.MobilePushService;
import ru.yandex.quasar.billing.util.ParallelHelper;

import static java.util.Collections.emptyMap;
import static java.util.Objects.requireNonNull;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.mapping;
import static java.util.stream.Collectors.toCollection;
import static java.util.stream.Collectors.toMap;
import static java.util.stream.Collectors.toSet;
import static ru.yandex.quasar.billing.beans.PromoType.getExpirationTime;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a6m;
import static ru.yandex.quasar.billing.beans.PromoType.plus120;
import static ru.yandex.quasar.billing.beans.PromoType.plus180_multi;
import static ru.yandex.quasar.billing.beans.PromoType.plus90;
import static ru.yandex.quasar.billing.beans.PromoType.plus90_149rub;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.RU;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.CANT_BE_CONSUMED;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.CODE_EXPIRED;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.FAILED_TO_CREATE_PAYMENT;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.NOT_ALLOWED_IN_CURRENT_REGION;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.NOT_EXISTS;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.ONLY_FOR_MOBILE;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.ONLY_FOR_NEW_USERS;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.ONLY_FOR_WEB;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.SUCCESS;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.USER_HAS_TEMPORARY_CAMPAIGN_RESTRICTIONS;
import static ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult.USER_TEMPORARY_BANNED;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.amediateka;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.kinopoisk;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.yandexplus;
import static ru.yandex.quasar.billing.services.sup.MobilePushService.DEFAULT_DATE_TO_OFFSET;

@Service
public class QuasarPromoServiceImpl implements QuasarPromoService {
    static final int RUS_ID = 225;
    static final int KZ_ID = 159;
    static final int BY_ID = 149;
    static final int WAIT_BETWEEN_CODES_MS = 10_000;

    static final PromoType DROIDEKA_PROMO = PromoType.plus90;
    private static final Logger logger = LogManager.getLogger();

    // If TV is connected via ethernet wifi_mac will be 02:00:00:00:00:00
    private static final String WIFI_MAC_FOR_ETHERNET_CONNECTION = "02:00:00:00:00:00";

    private static final String PROMO_NAME = "Яндекс.Плюс";
    private static final String PLUS_ACTIVATION_FAILURE_SIGNAL = "quasar_billing_plus_activation_failure_dmmm";
    private static final String PLUS_ACTIVATION_SUCCESS_SIGNAL = "quasar_billing_plus_activation_dmmm";
    private static final String ASYNC_ACTIVATION_PROCESS_INTERRUPTED = "async_activation_process_interrupted_dmmm";
    private static final String ASYNC_ACTIVATION_PROCESS_FAILED = "async_activation_process_failed_dmmm";
    private static final String NO_PROMO_CODE_LEFT = "quasar_billing_no_promocode_left_%s_%s_dmmm";
    private static final String NO_PROMO_CODE_LEFT_TOTAL = "quasar_billing_no_promocode_left-total_dmmm";
    private static final String PROVIDER_PROMO_ACTIVATION_SUCCESS_SIGNAL_FORMAT =
            "quasar_billing_%s_promo_activation_dmmm";
    private static final String PROVIDER_PROMO_ACTIVATION_FAILURE_SIGNAL_FORMAT =
            "quasar_billing_%s_promo_activation_failure_dmmm";
    private static final String PROMO_ACTIVATION_RESULT_SIGNAL_FORMAT =
            "quasar_billing_%s_%s_promo_activation_result_dmmm";
    private static final String PROMO_CODE_ACTIVATION_FAILURE_SIGNAL_FORMAT =
            "quasar_billing_%s_promo_activation_%s_dmmm";
    private final UsedDevicePromoDao usedDevicePromoDao;
    private final QuasarBackendClient quasarBackendClient;
    private final MediaBillingClient mediaBillingClient;
    private final BillingService billingService;
    private final MobilePushService mobilePushService;
    private final PromoCodeBaseDao promoCodeBaseDao;
    private final TransactionTemplate transactionTemplate;
    private final UnistatService unistatService;
    private final QuasarPromoBlacklistDao quasarPromoBlacklistDao;
    private final ParallelHelper parallelHelper;
    private final LaasClient laasClient;
    private final PromocodePrototypeDao promocodePrototypeDao;
    private final DroidekaClient droidekaClient;
    private Map<PrototypeKey, PromocodePrototype> prototypesCache = Map.of();
    private final AuthorizationContext authorizationContext;
    private final Map<ExpClause, Map<String, PromoType>> experimentsRemap = Map.of(
            new ExpClause(plus90, Platform.YANDEXMINI_2),
            Map.of(Experiments.PLUS_MULTI_3M, PromoType.plus90_multi,
                    Experiments.PLUS_MULTI_1M, PromoType.plus30_multi),

            new ExpClause(plus90, Platform.YANDEXMICRO),
            Map.of(Experiments.PLUS_MULTI_3M, PromoType.plus90_multi,
                    Experiments.PLUS_MULTI_1M, PromoType.plus30_multi),

            new ExpClause(plus180_multi, Platform.YANDEXMIDI),
            Map.of(Experiments.PLUS_MULTI_3M_STATION2, PromoType.plus90_multi),

            new ExpClause(kinopoisk_a6m, Platform.YANDEXSTATION_2),
            Map.of(Experiments.KP_A_3M, PromoType.kinopoisk_a3m_multi,
                    Experiments.KP_A_3M_KPA, PromoType.kinopoisk_a3m_to_kpa,
                    Experiments.KP_A_6M_KPA, PromoType.kinopoisk_a6m_to_kpa)
    );

    private record ExpClause(
            PromoType promoType,
            Platform platform
    ) {
    }


    private record PrototypeKey(
            PromoType promoType,
            Platform platform
    ) {
    }

    private record PromocodePrototype(
            int id,
            Platform platform,
            PromoType promoType,
            String code,
            String taskId
    ) {
    }

    @SuppressWarnings("ParameterNumber")
    QuasarPromoServiceImpl(UsedDevicePromoDao usedDevicePromoDao,
                           QuasarBackendClient quasarBackendClient,
                           MediaBillingClient mediaBillingClient,
                           BillingService billingService,
                           MobilePushService mobilePushService,
                           PromoCodeBaseDao promoCodeBaseDao,
                           PlatformTransactionManager transactionManager,
                           UnistatService unistatService,
                           QuasarPromoBlacklistDao quasarPromoBlacklistDao,
                           AuthorizationContext authorizationContext,
                           LaasClient laasClient,
                           PromocodePrototypeDao promocodePrototypeDao,
                           DroidekaClient droidekaClient) {
        this.usedDevicePromoDao = usedDevicePromoDao;
        this.quasarBackendClient = quasarBackendClient;
        this.mediaBillingClient = mediaBillingClient;
        this.billingService = billingService;
        this.mobilePushService = mobilePushService;
        this.promoCodeBaseDao = promoCodeBaseDao;
        this.transactionTemplate = new TransactionTemplate(transactionManager);
        this.unistatService = unistatService;
        this.quasarPromoBlacklistDao = quasarPromoBlacklistDao;
        this.laasClient = laasClient;
        this.promocodePrototypeDao = promocodePrototypeDao;
        this.parallelHelper = new ParallelHelper(Executors.newCachedThreadPool(), authorizationContext);
        this.droidekaClient = droidekaClient;
        this.authorizationContext = authorizationContext;
    }

    @Scheduled(fixedRate = 15 * 60 * 1000)
    public void loadPrototypes() {
        prototypesCache = promocodePrototypeDao.findAll().stream()
                .filter(it -> PromoType.getByTag(it.getPromoType()) != null)
                .map(it -> new PromocodePrototype(it.getId(), Platform.create(it.getPlatform()),
                        PromoType.getByTag(it.getPromoType()), it.getCode(), it.getTaskId()))
                .collect(toMap(it -> new PrototypeKey(it.promoType, it.platform), x -> x));
    }

    @Override
    public Collection<DeviceInfo> getUserDevices(String uid) {
        Collection<BackendDeviceInfo> backendDeviceInfos = quasarBackendClient.getUserDeviceList(uid);
        Map<DeviceId, Set<PromoProvider>> devicesUsedPromos = getDevicesUsedPromos(backendDeviceInfos);
        return backendDeviceInfos.stream()
                .map(device -> createDeviceInfo(uid, device,
                        devicesUsedPromos.getOrDefault(device.getDeviceIdentifier(), Collections.emptySet())))
                .collect(Collectors.toList());
    }

    @Override
    public DeviceInfo getUserDevice(String uid, DeviceId deviceId)
            throws DeviceNotOwnedByUserException {
        return getUserDeviceInternal(uid, parallelHelper.async(() -> getBackendDeviceInfo(uid, deviceId)));
    }

    @Override
    public DeviceInfo getUserDevice(@Nullable String uid, BackendDeviceInfo backendDevice)
            throws DeviceNotOwnedByUserException {
        return getUserDeviceInternal(uid, CompletableFuture.completedFuture(backendDevice));
    }

    private DeviceInfo getUserDeviceInternal(@Nullable String uid, CompletableFuture<BackendDeviceInfo> backendDeviceF)
            throws DeviceNotOwnedByUserException {

        BackendDeviceInfo backendDeviceInfo;
        try {
            backendDeviceInfo = backendDeviceF.join();
        } catch (CompletionException e) {
            if (e.getCause() instanceof DeviceNotOwnedByUserException) {
                throw (DeviceNotOwnedByUserException) e.getCause();
            } else {
                throw e;
            }
        }

        Map<DeviceId, Set<PromoProvider>> allDevicesUsedPromos = getDevicesUsedPromos(List.of(backendDeviceInfo));

        Set<PromoProvider> deviceUsedPromos = allDevicesUsedPromos
                .getOrDefault(backendDeviceInfo.getDeviceIdentifier(), Collections.emptySet());
        return createDeviceInfo(uid, backendDeviceInfo, deviceUsedPromos);
    }

    @Nonnull
    private BackendDeviceInfo getBackendDeviceInfo(String uid, DeviceId deviceId) {
        return quasarBackendClient.getUserDeviceList(uid).stream()
                .filter(it -> it.getDeviceIdentifier().equals(deviceId))
                .findFirst()
                .orElseThrow(() -> new DeviceNotOwnedByUserException(uid, deviceId));
    }

    @Override
    public DevicePromoActivationResult activatePromoPeriodFromPP(
            PromoProvider provider,
            String uid,
            String userIp,
            DeviceId deviceId,
            String paymentCardId
    ) {

        return activatePromoPeriod(
                provider, uid, userIp, paymentCardId, ActivationSurface.PP,
                parallelHelper.async(() -> getBackendDeviceInfo(uid, deviceId)));

    }

    @Override
    public DevicePromoActivationResult activatePromoPeriodFromTv(
            PromoProvider provider,
            String uid,
            String userIp,
            BackendDeviceInfo backendDevice,
            String paymentCardId
    ) {

        return activatePromoPeriod(
                provider, uid, userIp, paymentCardId, ActivationSurface.TV,
                CompletableFuture.completedFuture(backendDevice));

    }


    private DevicePromoActivationResult activatePromoPeriod(
            PromoProvider provider,
            String uid,
            String userIp,
            String paymentCardId,
            ActivationSurface activationSurface,
            CompletableFuture<BackendDeviceInfo> backendDeviceFuture
    ) {
        // check provider card exists in the user's passport
        PaymentProcessor paymentProcessor = PaymentProcessor.MEDIABILLING;
        CompletableFuture<List<PaymentMethod>> cardsList = parallelHelper.async(
                () -> billingService.getCardsList(uid, paymentProcessor, userIp)
        );

        CompletableFuture<Integer> regionFuture =
                parallelHelper.async(laasClient::getCountryId).exceptionally(it -> null);

        BackendDeviceInfo backendDevice = backendDeviceFuture.join();

        Set<PromoProvider> usedPromos = getDevicesUsedPromos(List.of(backendDevice))
                .getOrDefault(backendDevice.getDeviceIdentifier(), Collections.emptySet());

        @Nullable Integer region = regionFuture.join();
        Map<PromoProvider, PromoType> availableProvidersPromos =
                getAvailableProvidersPromos(uid, backendDevice, usedPromos);

        if (availableProvidersPromos.containsKey(provider)) {

            cardsList.join()
                    .stream()
                    .filter(it -> "card".equals(it.getPaymentMethod()))
                    .filter(it -> it.getExpired() != null && !it.getExpired())
                    .map(PaymentMethod::getId)
                    .filter(paymentCardId::equals)
                    .findAny()
                    .orElseThrow(NoCardBoundException::new);

            UsedDevicePromo promo;
            String code;
            try {
                PromoType promoType = availableProvidersPromos.get(provider);
                DeviceCodePair result = getCodeToUse(promoType, backendDevice.getDeviceIdentifier());
                promo = result.devicePromo();
                code = result.code();
            } catch (DuplicateKeyException e) {
                incrementPromoActivationSignal(provider, DevicePromoActivationResult.ALREADY_ACTIVATED);
                return DevicePromoActivationResult.ALREADY_ACTIVATED;
            }

            DevicePromoActivationResult result =
                    activateAllPromos(uid, backendDevice.getDeviceIdentifier(), paymentCardId, provider,
                            requireNonNull(code, "code can't be null"), region, activationSurface);

            if (result == SUCCESS) {
                // save successful activation result
                promo.setPromoActivationTime(Instant.now());
                promo.setUid(Long.valueOf(uid));
                usedDevicePromoDao.save(promo);
            }

            incrementPromoActivationSignal(provider, result);
            return result;

        } else {
            incrementPromoActivationSignal(provider, DevicePromoActivationResult.ALREADY_ACTIVATED);
            return DevicePromoActivationResult.ALREADY_ACTIVATED;
        }
    }

    private void incrementPromoActivationSignal(PromoProvider provider, DevicePromoActivationResult result) {
        unistatService.incrementStatValue(String.format(
                PROMO_ACTIVATION_RESULT_SIGNAL_FORMAT,
                provider.name(),
                result.getCode()
        ));
    }

    @Override
    public RequestPlusResult requestPlus(String uid) {
        return requestPlus(uid, true);
    }

    @Override
    public RequestPlusResult requestPlus(String uid, boolean sendPersonalCards) {
        Optional<PromoTypeWithExpiration> plusPromo = Optional.empty();
        Optional<PromoTypeWithExpiration> kinopoiskPromo = Optional.empty();
        try {
            Collection<DeviceInfo> deviceInfos = getUserDevices(uid);

            // if any of user devices has Plus promo-period available
            for (DeviceInfo deviceInfo : deviceInfos) {
                PromoType curPlusPromo = deviceInfo.getAvailablePromoTypes().getOrDefault(yandexplus, null);
                if (!plusPromo.map(it -> it.promo().isExperiment()).orElse(false)) {
                    plusPromo = Optional.ofNullable(curPlusPromo)
                            .map(it -> new PromoTypeWithExpiration(curPlusPromo, getExpirationTime(
                                    curPlusPromo, deviceInfo.getFirstActivation())));
                }

                PromoType curKinopoiskPromoType = deviceInfo.getAvailablePromoTypes().getOrDefault(kinopoisk, null);
                if (!kinopoiskPromo.map(it -> it.promo().isExperiment()).orElse(false)) {
                    kinopoiskPromo = Optional.ofNullable(curKinopoiskPromoType)
                            .map(it -> new PromoTypeWithExpiration(curKinopoiskPromoType, getExpirationTime(
                                    curKinopoiskPromoType, deviceInfo.getFirstActivation())));
                }
            }
        } catch (RuntimeException e) {
            // fallback to purchase scenario
            plusPromo = Optional.empty();
            kinopoiskPromo = Optional.empty();
        }

        if (kinopoiskPromo.isPresent()) {
            return processPromoAvailable(uid, sendPersonalCards, kinopoisk.name(),
                    kinopoiskPromo.get().expirationTime().orElse(null),
                    kinopoiskPromo.get().promo().isExperiment() ? kinopoiskPromo.get().promo().name() : null);
        } else if (plusPromo.isPresent()) {
            return processPromoAvailable(uid, sendPersonalCards, yandexplus.name(),
                    plusPromo.get().expirationTime().orElse(null),
                    plusPromo.get().promo().isExperiment() ? plusPromo.get().promo().name() : null);
        } else {
            // todo: send push to purchase yandexplus subscription
            return new RequestPlusResult(PromoState.PAYMENT_REQUIRED, null, null, null, null);
        }

    }

    private RequestPlusResult processPromoAvailable(
            String uid,
            boolean sendPersonalCards,
            String providerName,
            @Nullable Instant expirationTime,
            @Nullable String experiment
    ) {
        OfferCardData offerCardData = null;
        if (!sendPersonalCards) {
            long currentTimeSec = System.currentTimeMillis() / 1000;
            offerCardData = new OfferCardData(mobilePushService.generateCardId(),
                    mobilePushService.getActivatePromoPeriodUrl(providerName), "Активировать " + PROMO_NAME,
                    currentTimeSec, currentTimeSec + DEFAULT_DATE_TO_OFFSET, emptyMap());
        }

        String activatePromoPeriodUrl = mobilePushService.getActivatePromoPeriodUrl(providerName);
        String activatePromoUri = mobilePushService.getWrappedLinkUrl(activatePromoPeriodUrl);

        return new RequestPlusResult(PromoState.PROMO_AVAILABLE, offerCardData, activatePromoUri, expirationTime,
                experiment);
    }


    // associate and obtain promocode for the device activation
    private DeviceCodePair getCodeToUse(PromoType promoType, DeviceId deviceId) {
        PromoProvider provider = promoType.getProvider();

        Optional<UsedDevicePromo> promoMaybe = usedDevicePromoDao.findByUidAndDeviceIdAndPlatformAndProvider(
                deviceId.getId(),
                deviceId.getPlatform(),
                provider.name());

        if (promoMaybe.isPresent()) {
            UsedDevicePromo promo = promoMaybe.get();
            if (promo.isPromoActivated()) {
                throw new DevicePromoAlreadyUsedException(deviceId, provider);
            }
            String code = promoCodeBaseDao.findById(promo.getCodeId())
                    .map(PromoCodeBase::getCode)
                    .orElseThrow(() -> new InternalErrorException("Promo code not found"));

            return new DeviceCodePair(promo, code);
        } else {
            Stopwatch sw = Stopwatch.createStarted();

            PromocodePrototype prototype = prototypesCache.get(new PrototypeKey(promoType,
                    deviceId.getPlatform()));

            PromoCodeBase newCode = prototype != null ? cloneCode(prototype, provider) : null;
            return requireNonNull(
                    // we do this in a single transaction for atomicity, always not null
                    transactionTemplate.execute(status -> {

                        Optional<PromoCodeBase> codeO = Optional.ofNullable(newCode);

                        if (codeO.isEmpty()) {
                            codeO = promoCodeBaseDao.queryNextUnusedCode(provider.name(), promoType,
                                    deviceId.getPlatform());
                            unistatService.logOperationDurationHist(
                                    "quasar_billing_query_promocode_duration_dhhh",
                                    sw.stop().elapsed(TimeUnit.MILLISECONDS)
                            );

                            unistatService.logOperationDurationHist(
                                    "quasar_billing_query_promocode_type_" + promoType.name() +
                                            "_platform_" + deviceId.getPlatform().toString() +
                                            "_duration_dhhh",
                                    sw.elapsed(TimeUnit.MILLISECONDS)
                            );
                        }

                        if (codeO.isEmpty()) {
                            unistatService.incrementStatValue(
                                    String.format(NO_PROMO_CODE_LEFT, promoType.name(), deviceId.getPlatform())
                            );
                            unistatService.incrementStatValue(NO_PROMO_CODE_LEFT_TOTAL);
                            throw new NoPromoCodeLeftException(provider.name(), promoType.name());
                        }
                        PromoCodeBase code = codeO.get();

                        UsedDevicePromo newPromo = UsedDevicePromo.builder()
                                .deviceId(deviceId.getId())
                                .platform(deviceId.getPlatform())
                                .provider(provider)
                                .codeId(code.getId())
                                .build();
                        return new DeviceCodePair(usedDevicePromoDao.save(newPromo), code.getCode());
                    }));
        }
    }

    @Nonnull
    private PromoCodeBase cloneCode(PromocodePrototype prototype, PromoProvider provider) {
        Stopwatch sw = Stopwatch.createStarted();
        var codes = List.of(prototype.code().split("\\|"));
        String clonedCode = String.join("|", parallelHelper.processParallel(codes,
                mediaBillingClient::clonePrototype));

        logger.info("Cloned {} codes for promoType {} and platform {}. Took: {} ms",
                codes.size(), prototype.promoType.name(), prototype.platform.toString(), sw.elapsed().toMillis());

        sw = Stopwatch.createStarted();
        PromoCodeBase code = promoCodeBaseDao.save(
                PromoCodeBase.create(provider.name(), prototype.promoType, clonedCode, prototype.platform,
                        prototype.id(), prototype.taskId())
        );
        logger.info("Cloned code saved. Took: {} ms", sw.elapsed().toMillis());

        return code;
    }

    @SuppressWarnings("ParameterNumber")
    private DevicePromoActivationResult activateAllPromos(
            String uid,
            DeviceId deviceId,
            String paymentCardId,
            PromoProvider provider,
            String codeString,
            Integer region,
            ActivationSurface activationSurface) {
        String[] codes = codeString.split("\\|");

        var result = activateSinglePromo(uid, deviceId, paymentCardId, provider, codes[0], region,
                activationSurface);
        if (result == SUCCESS && codes.length > 1) {
            parallelHelper.async(() -> {
                var r = SUCCESS;
                for (int i = 1; i < codes.length && r == SUCCESS; i++) {
                    try {
                        // PASKILLS-6622: Проблемы медиабиллинга при активации двух промокодов подряд
                        Thread.sleep(WAIT_BETWEEN_CODES_MS);
                        r = activateSinglePromo(uid, deviceId, paymentCardId, provider, codes[i], region,
                                activationSurface);
                    } catch (InterruptedException e) {
                        unistatService.incrementStatValue(ASYNC_ACTIVATION_PROCESS_INTERRUPTED);
                        logger.error("Cant activate promocode for device_id=" + deviceId +
                                " and provider=" + provider, e);
                        return;
                    } catch (RuntimeException e) {
                        logger.error("Secondary promo activation for device_id=" + deviceId +
                                " and provider=" + provider + " failed", e);
                        unistatService.incrementStatValue(ASYNC_ACTIVATION_PROCESS_FAILED);
                        throw e;
                    }
                }
            });
        }
        return result;
    }

    private DevicePromoActivationResult activateSinglePromo(
            String uid,
            DeviceId deviceId,
            String paymentCardId,
            PromoProvider provider,
            String code,
            @Nullable Integer region,
            ActivationSurface activationSurface) {
        try {

            MediaBillingClient.MusicPromoActivationResult result;

            String origin = makeOrigin(deviceId.getPlatform(), activationSurface.getCode());


            result = mediaBillingClient.activatePromoCode(
                    Long.parseLong(uid), code, paymentCardId, origin, region
            ).getStatus();


            var activationResult = convertActivationResult(deviceId, provider, result);
            if (activationResult == SUCCESS) {
                if (provider == yandexplus) {
                    unistatService.incrementStatValue(PLUS_ACTIVATION_SUCCESS_SIGNAL);
                }
                unistatService.incrementStatValue(String.format(PROVIDER_PROMO_ACTIVATION_SUCCESS_SIGNAL_FORMAT,
                        provider.name()));
            } else {
                if (provider == yandexplus) {
                    unistatService.incrementStatValue(PLUS_ACTIVATION_FAILURE_SIGNAL);
                }
                unistatService.incrementStatValue(String.format(PROVIDER_PROMO_ACTIVATION_FAILURE_SIGNAL_FORMAT,
                        provider.name()));
            }
            return activationResult;
        } catch (Exception e) {
            if (provider == yandexplus) {
                unistatService.incrementStatValue(PLUS_ACTIVATION_FAILURE_SIGNAL);
            }
            unistatService.incrementStatValue(String.format(PROVIDER_PROMO_ACTIVATION_FAILURE_SIGNAL_FORMAT,
                    provider.name()));
            if (e instanceof UnknownPromoActivationResultException) {
                throw e;
            } else {
                logger.error("failed to activate promo", e);
                throw new DevicePromoActivationException(deviceId, provider, e);
            }
        }
    }

    private String makeOrigin(Platform platform, String activationSurface) {
        String subsource = platform.isTv() ? "yandex_tv" : "alice_device";
        return "clientSource=device&clientSubSource=" + subsource +
                "&clientPlace=" + platform.getName() +
                "&clientDeviceActivation=" + activationSurface;
    }

    private DevicePromoActivationResult convertActivationResult(DeviceId deviceId, PromoProvider provider,
                                                                MediaBillingClient.MusicPromoActivationResult result) {
        switch (result) {
            case SUCCESS:
                return SUCCESS;
            case CODE_ALREADY_CONSUMED:
                return DevicePromoActivationResult.ALREADY_ACTIVATED;
            case USER_TEMPORARY_BANNED:
                return USER_TEMPORARY_BANNED;
            case CODE_NOT_ALLOWED_IN_CURRENT_REGION:
                incrementActivationFailureStatValue(provider, NOT_ALLOWED_IN_CURRENT_REGION);
                return NOT_ALLOWED_IN_CURRENT_REGION;
            case USER_HAS_TEMPORARY_CAMPAIGN_RESTRICTIONS:
                return USER_HAS_TEMPORARY_CAMPAIGN_RESTRICTIONS;
            case CODE_EXPIRED:
                incrementActivationFailureStatValue(provider, CODE_EXPIRED);
                return CODE_EXPIRED;
            case CODE_NOT_EXISTS:
                incrementActivationFailureStatValue(provider, NOT_EXISTS);
                return NOT_EXISTS;
            case CODE_ONLY_FOR_NEW_USERS:
                incrementActivationFailureStatValue(provider, ONLY_FOR_NEW_USERS);
                return ONLY_FOR_NEW_USERS;
            case CODE_ONLY_FOR_WEB:
                incrementActivationFailureStatValue(provider, ONLY_FOR_WEB);
                return ONLY_FOR_WEB;
            case CODE_ONLY_FOR_MOBILE:
                incrementActivationFailureStatValue(provider, ONLY_FOR_MOBILE);
                return ONLY_FOR_MOBILE;
            case FAILED_TO_CREATE_PAYMENT:
                incrementActivationFailureStatValue(provider, FAILED_TO_CREATE_PAYMENT);
                return FAILED_TO_CREATE_PAYMENT;
            case CODE_CANT_BE_CONSUMED:
                incrementActivationFailureStatValue(provider, CANT_BE_CONSUMED);
                return CANT_BE_CONSUMED;
            default:
                throw new UnknownPromoActivationResultException(deviceId, provider, result);
        }
    }

    private void incrementActivationFailureStatValue(PromoProvider provider, DevicePromoActivationResult result) {
        String signal = String.format(PROMO_CODE_ACTIVATION_FAILURE_SIGNAL_FORMAT, provider.name(), result.getCode());
        unistatService.incrementStatValue(signal);
    }

    private Map<PromoProvider, PromoType> getAvailableProvidersPromos(@Nullable String uid,
                                                                      BackendDeviceInfo deviceInfo,
                                                                      Set<PromoProvider> usedPromos) {

        if (uid != null && quasarPromoBlacklistDao.userInBlacklist(uid)) {
            return Collections.emptyMap();
        }

        return promosByProvider(deviceInfo, usedPromos);
    }

    @Nonnull
    Map<PromoProvider, PromoType> promosByProvider(BackendDeviceInfo deviceInfo,
                                                   Set<PromoProvider> usedPromos) {
        Map<PromoProvider, PromoType> devicesPromos = deviceInfo.getTags().stream()
                .map(PromoType::getByTag)
                .filter(Objects::nonNull)
                .map(promoType -> promoType.forRegion(deviceInfo.getActivationRegion()))
                // if user activated old amediateka it can't activate the new one
                .filter(promoType -> !(promoType == PromoType.amediateka90 && usedPromos.contains(amediateka)))
                // exclude used promos and plus activated by in backed earlier
                .filter(promoType -> !usedPromos.contains(promoType.getProvider()))
                .filter(__ -> !promoActivatedInDroideka(deviceInfo))
                //if user activated kinopoisk it cant activate plus and vise versa
                .filter(promoType -> !(promoType.getProvider() == yandexplus && usedPromos.contains(kinopoisk)))
                // if device activated yandexplus via promo or by the old way it can't activate kinopoisk or yandex plus
                .filter(promoType -> !(
                        (
                                (promoType.getProvider() == kinopoisk && promoType != PromoType.amediateka90)
                                        || promoType.getProvider() == yandexplus
                        ) && (usedPromos.contains(yandexplus))
                ))
                .map(promoType -> mapExperimentalPromo(deviceInfo.getPlatform(), promoType))
                .map(promoType -> authorizationContext.hasExperiment(Experiments.PLUS120_EXPERIMENT) ?
                        mapPlus120Experiment(deviceInfo, promoType) :
                        promoType)
                .map(promoType -> authorizationContext.hasExperiment(Experiments.PLUS90_TV_149RUB) ?
                        mapPlus90Tv149Experiment(deviceInfo, promoType) :
                        promoType)
                .collect(toMap(PromoType::getProvider,
                        it -> it,
                        (u, v) -> u.isExperiment() ? u : v,
                        () -> new EnumMap<>(PromoProvider.class)));

        return devicesPromos;
    }

    private PromoType mapExperimentalPromo(Platform platform, PromoType promoType) {
        Set<String> requestExperiments = authorizationContext.getRequestExperiments();
        if (requestExperiments.isEmpty()) {
            return promoType;
        }

        ExpClause clause = new ExpClause(promoType, platform);
        Map<String, PromoType> remap = experimentsRemap.getOrDefault(clause, Map.of());
        for (String requestExperiment : requestExperiments) {
            if (remap.containsKey(requestExperiment)) {
                return remap.get(requestExperiment);
            }
        }
        return promoType;
    }

    private boolean promoActivatedInDroideka(BackendDeviceInfo deviceInfo) {
        return deviceInfo.isDroidekaMaintainedDevice() &&
                // Droideka fails with 400 http code for devices with such wifi_mac so there is no way gift may be
                // already activated.
                !WIFI_MAC_FOR_ETHERNET_CONNECTION.equals(deviceInfo.getWifiMac()) &&
                !droidekaClient.getGiftState(
                        deviceInfo.getSerial(), deviceInfo.getWifiMac(), deviceInfo.getEthernetMac()
                ).isGiftAvailable();
    }

    private PromoType mapPlus90Tv149Experiment(BackendDeviceInfo deviceInfo, PromoType promoType) {
        return promoType == PromoType.plus90
                && Platform.TV_PLATFORMS.contains(deviceInfo.getPlatform()) ?
                plus90_149rub : promoType;
    }

    private PromoType mapPlus120Experiment(BackendDeviceInfo deviceInfo, PromoType promoType) {
        return promoType == PromoType.plus90
                && (deviceInfo.getPlatform() == Platform.YANDEXMINI
                || deviceInfo.getPlatform() == Platform.YANDEXMICRO)
                && (deviceInfo.getActivationRegion() == null
                || deviceInfo.getActivationRegion().equals(RU))
                // Use !isBefore=after or equal for tests
                && deviceInfo.getFirstActivation() != null
                && !authorizationContext.getRequestTimestamp().isBefore(
                deviceInfo.getFirstActivation().plus(7, ChronoUnit.DAYS))
                && !authorizationContext.getRequestTimestamp().isAfter(
                getExpirationTime(plus120, deviceInfo.getFirstActivation())
                        .orElse(Instant.MAX))
                ? plus120 : promoType;
    }

    @Nonnull
    private DeviceInfo createDeviceInfo(@Nullable String uid, BackendDeviceInfo device,
                                        Set<PromoProvider> usedDevicePromo) {
        return new DeviceInfo(device.getId(),
                device.getPlatform(), device.getName(),
                getAvailableProvidersPromos(uid, device, usedDevicePromo), device.getFirstActivation());
    }

    private Map<DeviceId, Set<PromoProvider>> getDevicesUsedPromos(Collection<BackendDeviceInfo> devices) {
        Set<DeviceId> deviceIds = devices.stream()
                .map(BackendDeviceInfo::getDeviceIdentifier)
                .collect(toSet());
        return usedDevicePromoDao.findByDevices(deviceIds)
                .stream()
                .filter(UsedDevicePromo::isPromoActivated)
                .collect(groupingBy(
                        it -> DeviceId.create(it.getDeviceId(), it.getPlatform()),
                        mapping(
                                UsedDevicePromo::getProvider,
                                toCollection(() -> EnumSet.noneOf(PromoProvider.class))
                        )
                ));
    }

    private record PromoTypeWithExpiration(
            PromoType promo,
            Optional<Instant> expirationTime
    ) {
    }

    private record DeviceCodePair(
            UsedDevicePromo devicePromo,
            @Nullable String code
    ) {
    }

}
