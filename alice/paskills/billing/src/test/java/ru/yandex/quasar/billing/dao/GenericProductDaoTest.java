package ru.yandex.quasar.billing.dao;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class GenericProductDaoTest {


    @Autowired
    private GenericProductDao genericProductDao;

    @Test
    void testSave() {
        GenericProduct product = genericProductDao.save(GenericProduct.create(1L, "1"));

        GenericProduct found = genericProductDao.findById(product.getId()).get();

        Assertions.assertThat(found).isEqualToIgnoringGivenFields(product, "createdAt");
    }

    @Test
    void testFindByPartnerId() {
        GenericProduct product = genericProductDao.save(GenericProduct.create(2L, "code1"));

        GenericProduct found = genericProductDao.findByPartnerId(2L).get();

        Assertions.assertThat(found).isEqualToIgnoringGivenFields(product, "createdAt");
    }
}
