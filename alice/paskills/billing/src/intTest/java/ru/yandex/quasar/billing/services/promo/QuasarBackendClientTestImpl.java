package ru.yandex.quasar.billing.services.promo;

import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.springframework.context.annotation.Primary;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.TestAuthorizationService;

import static java.util.Collections.emptyList;
import static java.util.Collections.emptyMap;
import static java.util.Collections.emptySet;

@Component
@Primary
public class QuasarBackendClientTestImpl implements QuasarBackendClient {

    public static final DeviceId USER_NEW_NOT_ACTIVATED_YANDEXSTATION = DeviceId.create("deviceId",
            Platform.YANDEXSTATION);
    public static final DeviceId USER_NEW_NOT_ACTIVATED_YANDEXSTATION_BY = DeviceId.create("deviceId_by",
            Platform.YANDEXSTATION);
    public static final DeviceId USER_ACTIVATED_YANDEXSTATION = DeviceId.create("deviceId_activated",
            Platform.YANDEXSTATION);
    public static final DeviceId SOME_OTHER_DEVICE = DeviceId.create("deviceId2", Platform.YANDEXSTATION);
    public static final DeviceId USER_RED_STATION = DeviceId.create("deviceId3", Platform.YANDEXSTATION);
    public static final DeviceId UNKNOWN_DEVICE = DeviceId.create("deviceId_unknown", Platform.create("unknown"));

    public static final DeviceId USER_NEW_NOT_ACTIVATED_TV = DeviceId.create("TvDeviceId", Platform.create(
            "yandex_tv_rt2871_hikeen"));
    public static final BackendDeviceInfo USER_NEW_NOT_ACTIVATED_TV_FULL =
            BackendDeviceInfo.create(USER_NEW_NOT_ACTIVATED_TV.getId(),
            USER_NEW_NOT_ACTIVATED_TV.getPlatform(), Set.of("plus90"), null, "serial", "wifiMac",
            "ethernetMac");

    private final AuthorizationContext authorizationContext;

    public QuasarBackendClientTestImpl(AuthorizationContext authorizationContext) {
        this.authorizationContext = authorizationContext;
    }

    @Override
    public Collection<BackendDeviceInfo> getUserDeviceList(String uid) {
        if (TestAuthorizationService.USER_TICKET.equals(authorizationContext.getCurrentUserTicket())) {
            return List.of(
                    new BackendDeviceInfo(USER_NEW_NOT_ACTIVATED_YANDEXSTATION.getId(),
                            USER_NEW_NOT_ACTIVATED_YANDEXSTATION.getPlatform(), Set.of("plus360"), "station",
                            null, Instant.EPOCH),
                    BackendDeviceInfo.create(USER_NEW_NOT_ACTIVATED_TV.getId(),
                            USER_NEW_NOT_ACTIVATED_TV.getPlatform(), Set.of("plus90"), null, "serial", "wifiMac",
                            "ethernetMac"),
                    new BackendDeviceInfo(USER_NEW_NOT_ACTIVATED_YANDEXSTATION_BY.getId(),
                            USER_NEW_NOT_ACTIVATED_YANDEXSTATION_BY.getPlatform(), Set.of("kinopoisk_a6m_plus6m"),
                            "station", BackendDeviceInfo.Region.BY, Instant.EPOCH),
                    new BackendDeviceInfo(USER_ACTIVATED_YANDEXSTATION.getId(),
                            USER_ACTIVATED_YANDEXSTATION.getPlatform(), Set.of("plus360"), "activated station",
                            null, Instant.EPOCH),
                    new BackendDeviceInfo(USER_RED_STATION.getId(), USER_RED_STATION.getPlatform(), emptySet(),
                            "Red station", null, Instant.EPOCH),
                    new BackendDeviceInfo(UNKNOWN_DEVICE.getId(), UNKNOWN_DEVICE.getPlatform(), Set.of(),
                            "unknown device", null, Instant.EPOCH)
            );
        } else {
            return emptyList();
        }
    }

    @Override
    public Map<String, Set<String>> getUserDeviceTags(String uid) {
        if (TestAuthorizationService.USER_TICKET.equals(authorizationContext.getCurrentUserTicket())) {
            return Map.of(
                    USER_NEW_NOT_ACTIVATED_YANDEXSTATION.getId(),
                    Set.of("plus360", "test_skill_product_tag"),
                    USER_NEW_NOT_ACTIVATED_YANDEXSTATION_BY.getId(), Set.of("kinopoisk_a6m_plus6m"),
                    USER_ACTIVATED_YANDEXSTATION.getId(), Set.of("plus360"),
                    USER_RED_STATION.getId(), emptySet(),
                    UNKNOWN_DEVICE.getId(), Set.of(),
                    USER_NEW_NOT_ACTIVATED_TV.getId(), Set.of("plus90")
            );
        } else {
            return emptyMap();
        }
    }
}
