package ru.yandex.quasar.billing.dao;

import java.time.Instant;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.dao.DuplicateKeyException;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.promo.DeviceId;
import ru.yandex.quasar.billing.services.promo.Platform;
import ru.yandex.quasar.billing.services.promo.PromoProvider;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
public class UsedDevicePromoDaoImplTest {

    private static final Long UID = 123L;
    private static final String DEVICE_ID = "deviceId";
    @Autowired
    private UsedDevicePromoDao usedDevicePromoDao;

    @Autowired
    private PromoCodeBaseDao promoCodeBaseDao;

    @Test
    public void testSave() {
        UsedDevicePromo record = UsedDevicePromo.builder()
                .uid(UID)
                .deviceId(DEVICE_ID)
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build();

        usedDevicePromoDao.save(record);

        List<UsedDevicePromo> fetchedRecord = usedDevicePromoDao.findByDeviceIdAndPlatform(DEVICE_ID,
                Platform.YANDEXSTATION);

        assertEquals(List.of(record), fetchedRecord);
    }

    @Test
    public void testFailOnDuplicate() {
        UsedDevicePromo record = UsedDevicePromo.builder()
                .uid(UID)
                .deviceId(DEVICE_ID)
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build();

        usedDevicePromoDao.save(record);

        UsedDevicePromo record2 = UsedDevicePromo.builder()
                .uid(UID + 1)
                .deviceId(DEVICE_ID)
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build();

        assertThrows(DuplicateKeyException.class, () -> usedDevicePromoDao.save(record2));
    }

    @Test
    void testFindDevices() {
        UsedDevicePromo record = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID)
                .deviceId(DEVICE_ID)
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        UsedDevicePromo record2 = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID + 1)
                .deviceId(DEVICE_ID + "2")
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        UsedDevicePromo record3 = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID + 1)
                .deviceId(DEVICE_ID + "3")
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        List<UsedDevicePromo> devices = usedDevicePromoDao.findByDevices(Set.of(DeviceId.create(DEVICE_ID,
                Platform.YANDEXSTATION), DeviceId.create(DEVICE_ID + "2", Platform.YANDEXSTATION)));

        assertEquals(Set.of(record, record2), new HashSet<>(devices));


    }

    @Test
    void testFindDevicesWithEmptyList() {
        UsedDevicePromo record = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID)
                .deviceId(DEVICE_ID)
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        UsedDevicePromo record2 = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID + 1)
                .deviceId(DEVICE_ID + "2")
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        UsedDevicePromo record3 = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID + 1)
                .deviceId(DEVICE_ID + "3")
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        List<UsedDevicePromo> devices = usedDevicePromoDao.findByDevices(Set.of());

        assertEquals(Set.of(), new HashSet<>(devices));
    }

    @Test
    void testFindByUid() {
        UsedDevicePromo record = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID)
                .deviceId(DEVICE_ID)
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        UsedDevicePromo record2 = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID + 1)
                .deviceId(DEVICE_ID + "2")
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        UsedDevicePromo record3 = usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID + 1)
                .deviceId(DEVICE_ID + "3")
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .build());

        List<UsedDevicePromo> devices = usedDevicePromoDao.findAllByUid(String.valueOf(UID + 1));

        assertEquals(Set.of(record2, record3), new HashSet<>(devices));
    }

    @Test
    void testPromoStatistics() {
        promoCodeBaseDao.save(
                PromoCodeBase.create(PromoProvider.yandexplus.name(), PromoType.plus90, "FV", Platform.YANDEXSTATION,
                        null)
        );

        usedDevicePromoDao.save(UsedDevicePromo.builder()
                .uid(UID)
                .deviceId(DEVICE_ID)
                .platform(Platform.YANDEXSTATION)
                .provider(PromoProvider.yandexplus)
                .promoActivationTime(Instant.now())
                .build());

        List<PromoStatistics> promoStatistics = usedDevicePromoDao.getPromoStatistics();

        assertEquals(promoStatistics.get(0),
                new PromoStatistics(PromoType.plus90.name(),
                        Platform.YANDEXSTATION.getName(),
                        PromoProvider.yandexplus.name(), null, 1, 1));
    }
}
