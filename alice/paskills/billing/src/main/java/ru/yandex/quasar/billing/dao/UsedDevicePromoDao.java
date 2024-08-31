package ru.yandex.quasar.billing.dao;

import java.util.List;
import java.util.Optional;
import java.util.Set;

import org.springframework.dao.DuplicateKeyException;

import ru.yandex.quasar.billing.services.promo.DeviceId;
import ru.yandex.quasar.billing.services.promo.Platform;

public interface UsedDevicePromoDao {
    UsedDevicePromo save(UsedDevicePromo usedDevicePromo) throws DuplicateKeyException;

    List<UsedDevicePromo> findByDeviceIdAndPlatform(String deviceId, Platform platform);

    Optional<UsedDevicePromo> findByUidAndDeviceIdAndPlatformAndProvider(String deviceId, Platform platform,
                                                                         String provider);

    List<UsedDevicePromo> findByDevices(Set<DeviceId> deviceIds);

    List<UsedDevicePromo> findAllByUid(String uid);

    List<PromoStatistics> getPromoStatistics();
}
