package ru.yandex.quasar.billing.dao;

import java.util.List;
import java.util.Optional;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.promo.Platform;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class PromocodePrototypeDaoTest {

    @Autowired
    private PromocodePrototypeDao promocodePrototypeDao;

    @Test
    void testEmpty() {
        List<PromocodePrototypeDb> prototypes = promocodePrototypeDao.findAll();

        assertEquals(List.of(), prototypes);
    }

    @Test
    void testFindByPartnerId() {
        PromocodePrototypeDb save = promocodePrototypeDao.save(new PromocodePrototypeDb(null,
                Platform.YANDEXSTATION.toString(), PromoType.plus90.name(), "code1", "testk1"));

        assertNotNull(save.getId());

        List<PromocodePrototypeDb> prototypes = promocodePrototypeDao.findAll();

        assertEquals(List.of(save), prototypes);

        Optional<PromocodePrototypeDb> found = promocodePrototypeDao.findById(save.getId());

        assertEquals(save, found.orElse(null));

    }
}
