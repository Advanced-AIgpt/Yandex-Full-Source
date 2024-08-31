package ru.yandex.quasar.billing.controller;

import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;

import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.exception.UnauthorizedException;
import ru.yandex.quasar.billing.providers.IProvider;
import ru.yandex.quasar.billing.providers.ProviderManager;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.UserPurchaseLockService;
import ru.yandex.quasar.billing.services.promo.DeviceId;
import ru.yandex.quasar.billing.services.promo.DeviceInfo;
import ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult;
import ru.yandex.quasar.billing.services.promo.Platform;
import ru.yandex.quasar.billing.services.promo.PromoProvider;
import ru.yandex.quasar.billing.services.promo.QuasarBackendException;
import ru.yandex.quasar.billing.services.promo.QuasarPromoService;

import static java.util.stream.Collectors.partitioningBy;
import static java.util.stream.Collectors.toSet;

@RestController
public class ProviderManagerController {

    private static final Logger logger = LogManager.getLogger();
    private static final DateTimeFormatter FORMATTER = DateTimeFormatter.ISO_INSTANT;

    private final ProviderManager providerManager;
    private final AuthorizationService authorizationService;
    private final QuasarPromoService quasarPromoService;
    private final UserPurchaseLockService purchaseLockService;

    public ProviderManagerController(ProviderManager providerManager, AuthorizationService authorizationService,
                                     QuasarPromoService quasarPromoService,
                                     UserPurchaseLockService purchaseLockService) {
        this.providerManager = providerManager;
        this.authorizationService = authorizationService;
        this.quasarPromoService = quasarPromoService;
        this.purchaseLockService = purchaseLockService;
    }

    /**
     * Get list of supported providers
     */
    @GetMapping(path = "/billing/provider/all")
    public ProviderListResponse getProvidersList(HttpServletRequest request) {

        try {
            Collection<DeviceInfo> userDevices;
            try {
                String uid = authorizationService.getSecretUid(request);
                userDevices = quasarPromoService.getUserDevices(uid);
            } catch (UnauthorizedException | QuasarBackendException e) {
                userDevices = Collections.emptySet();
            }

            Set<String> providersWithPromo = userDevices.stream()
                    .map(DeviceInfo::getAvailablePromos)
                    .flatMap(Collection::stream)
                    .map(PromoProvider::name)
                    .collect(toSet());

            return new ProviderListResponse(providersWithPromo.contains(PromoProvider.yandexplus.name()),
                    providersWithPromo.contains(PromoProvider.kinopoisk.name()));
        } catch (Exception e) {
            logger.error(e.getMessage(), e);
            throw e;
        }
    }

    @GetMapping(path = "/billing/provider/{provider_name}")
    public ProviderInfoResponse getProviderInfo(
            @PathVariable("provider_name") String providerName,
            HttpServletRequest request
    ) {
        IProvider provider = providerManager.getProvider(providerName);

        Set<ProviderInfoResponse.DeviceShortInfo> devicesWithPromo = new HashSet<>();
        boolean authorized;
        try {
            String uid = authorizationService.getSecretUid(request);

            try {
                PromoProvider.byProviderName(providerName).ifPresent(promoProvider ->
                        quasarPromoService.getUserDevices(uid).stream()
                                .filter(device -> device.getAvailablePromoTypes().containsKey(promoProvider))
                                .map(device ->
                                        new ProviderInfoResponse.DeviceShortInfo(device.getDeviceId(),
                                                device.getPlatform(), device.getName(),
                                                device.getAvailablePromoTypes().get(promoProvider),
                                                getExpirationTime(device, promoProvider))
                                )
                                .forEach(devicesWithPromo::add));
            } catch (QuasarBackendException ignored) {
                // if something happened connecting backend just leave devices promo info empty as its crucial to
                // respond on provider method
            }


            authorized =
                    authorizationService.getProviderTokenByUid(uid, provider.getSocialAPIServiceName()).isPresent();
        } catch (UnauthorizedException e) {
            authorized = false;
        }

        return new ProviderInfoResponse(provider.getProviderName(), provider.getSocialAPIServiceName(),
                provider.getSocialAPIClientId(), authorized, devicesWithPromo);
    }

    // Задел на будущее, когда общение будет без разделения по провайдерам
    // Ручка заменит provider/all, /provider/*
    @GetMapping(path = "/billing/promo")
    public PromoInfoResponse promo(HttpServletRequest request) {

        String uid = authorizationService.getSecretUid(request);

        Function<DeviceInfo, PromoInfoResponse.DeviceShortInfo> createShortInfo =
                (device) -> new PromoInfoResponse.DeviceShortInfo(device.getDeviceId(),
                        device.getPlatform(),
                        device.getName(),
                        // we've checked is not empty
                        device.getAvailablePromoTypes().values().iterator().next()
                );

        Map<Boolean, Set<PromoInfoResponse.DeviceShortInfo>> allDevicesWithPromo =
                quasarPromoService.getUserDevices(uid).stream()
                        .filter(device -> !device.getAvailablePromoTypes().isEmpty())
                        .map(createShortInfo)
                        .sorted(Comparator.<PromoInfoResponse.DeviceShortInfo>comparingInt(deviceInfo ->
                                deviceInfo.getPromo().getDuration()).reversed())
                        .collect(partitioningBy(deviceShortInfo -> deviceShortInfo.getPromo().isMulti(), toSet()));

        var devicesWithMultiPromo = allDevicesWithPromo.get(true);
        var devicesWithPersonalPromo = allDevicesWithPromo.get(false);

        return new PromoInfoResponse(devicesWithMultiPromo, devicesWithPersonalPromo);
    }

    /**
     * Activate promo period in Quasar UI
     *
     * @param deviceId      device ID for which promo to use
     * @param platform      device platform for which promo to use
     * @param paymentCardId card information
     * @return activation result
     */
    @PostMapping(path = "/billing/provider/{providerName}/activatePromoOnDevice")
    public ResponseEntity<DevicePromoActivationResponse> activateProvidersPromo(
            @PathVariable("providerName") PromoProvider promoProvider,
            @RequestParam("deviceId") String deviceId,
            @RequestParam("platform") Platform platform,
            @RequestParam("paymentCardId") String paymentCardId,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        try {
            DevicePromoActivationResult result = purchaseLockService.processWithLock(
                    Long.valueOf(uid),
                    promoProvider.name(),
                    () -> quasarPromoService.activatePromoPeriodFromPP(
                            promoProvider,
                            uid,
                            userIp,
                            DeviceId.create(deviceId, platform),
                            paymentCardId
                    )
            );

            return ResponseEntity.ok(DevicePromoActivationResponse.create(result));
        } catch (UserPurchaseLockService.UserPurchaseLockException e) {
            throw new ResponseStatusException(HttpStatus.CONFLICT, e.getMessage());
        }

    }

    /**
     * Get devices available for yandexplus promoperiod activation
     *
     * @throws QuasarBackendException if problem occurred when accessing backend. Leads to 500 error and balance retry.
     */
    @GetMapping(path = "/billing/provider/yandexplus")
    public ProviderInfoResponse getYandexPlusProviderInfo(
            HttpServletRequest request
    ) {

        String uid;
        try {
            uid = authorizationService.getSecretUid(request);
        } catch (UnauthorizedException ignored) {
            return ProviderInfoResponse.unauthorized("yandexplus");
        }

        Set<ProviderInfoResponse.DeviceShortInfo> devicesWithPromo = quasarPromoService.getUserDevices(uid)
                .stream()
                .filter(device -> device.getAvailablePromos().contains(PromoProvider.yandexplus))
                .map(device -> new ProviderInfoResponse.DeviceShortInfo(device.getDeviceId(), device.getPlatform(),
                        device.getName(), device.getAvailablePromoTypes().get(PromoProvider.yandexplus),
                        getExpirationTime(device, PromoProvider.yandexplus)))
                .collect(toSet());
        return new ProviderInfoResponse("yandexplus", null, null, true, devicesWithPromo);
    }

    @Nullable
    String getExpirationTime(DeviceInfo device, PromoProvider promoProvider) {
        PromoType promoType = device.getAvailablePromoTypes().get(promoProvider);
        Optional<Instant> expirationTime = PromoType.getExpirationTime(promoType, device.getFirstActivation());
        return expirationTime.map(FORMATTER::format).orElse(null);
    }
}
