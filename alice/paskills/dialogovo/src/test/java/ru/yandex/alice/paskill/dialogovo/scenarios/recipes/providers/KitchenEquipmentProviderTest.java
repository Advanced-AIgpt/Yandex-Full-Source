package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(classes = TestConfigProvider.class, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class KitchenEquipmentProviderTest {

    @Autowired
    KitchenEquipmentProvider eqipmentProvider;

    @Test
    public void hasEquipment() {
        assertTrue(eqipmentProvider.size() > 0);
    }

}
