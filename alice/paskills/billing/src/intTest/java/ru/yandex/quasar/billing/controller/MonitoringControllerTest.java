package ru.yandex.quasar.billing.controller;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.skyscreamer.jsonassert.JSONAssert;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.test.context.junit.jupiter.SpringExtension;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.PromoCodeBase;
import ru.yandex.quasar.billing.dao.PromoCodeBaseDao;
import ru.yandex.quasar.billing.dao.PromocodePrototypeDao;
import ru.yandex.quasar.billing.dao.PromocodePrototypeDb;
import ru.yandex.quasar.billing.dao.UsedDevicePromo;
import ru.yandex.quasar.billing.dao.UsedDevicePromoDao;
import ru.yandex.quasar.billing.services.promo.PromoProvider;

import static java.util.Objects.requireNonNull;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static ru.yandex.quasar.billing.beans.PromoType.kinopoisk_a6m_plus6m;
import static ru.yandex.quasar.billing.beans.PromoType.plus90;
import static ru.yandex.quasar.billing.beans.PromoType.plus90_by;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXMINI;
import static ru.yandex.quasar.billing.services.promo.Platform.YANDEXSTATION;

/**
 * Test for {@link MonitoringController}
 */
@ExtendWith(SpringExtension.class)
@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {
                TestConfigProvider.class,
        }
)
@AutoConfigureWebClient(registerRestTemplate = true)
class MonitoringControllerTest {
    public static final String PROMO_STATISTICS_EXPECTED_JSON = "{\n" +
            "  \"sensors\": [\n" +
            "    {\n" +
            "      \"kind\": \"IGAUGE\",\n" +
            "      \"labels\": {\n" +
            "        \"platform\": \"yandexstation\",\n" +
            "        \"promoType\": \"kinopoisk_a6m_plus6m\",\n" +
            "        \"provider\": \"kinopoisk\",\n" +
            "        \"sensor\": \"billing.promo.code.statistic.left.count.gaugei\"\n" +
            "      },\n" +
            "      \"value\": 1\n" +
            "    },\n" +
            "    {\n" +
            "      \"kind\": \"IGAUGE\",\n" +
            "      \"labels\": {\n" +
            "        \"platform\": \"yandexstation\",\n" +
            "        \"promoType\": \"kinopoisk_a6m_plus6m\",\n" +
            "        \"provider\": \"kinopoisk\",\n" +
            "        \"sensor\": \"billing.promo.code.statistic.total.count.gaugei\"\n" +
            "      },\n" +
            "      \"value\": 2\n" +
            "    },\n" +
            "    {\n" +
            "      \"kind\": \"IGAUGE\",\n" +
            "      \"labels\": {\n" +
            "        \"platform\": \"yandexmini\",\n" +
            "        \"promoType\": \"plus90\",\n" +
            "        \"provider\": \"yandexplus\",\n" +
            "        \"sensor\": \"billing.promo.code.statistic.left.count.gaugei\"\n" +
            "      },\n" +
            "      \"value\": 1\n" +
            "    },\n" +
            "    {\n" +
            "      \"kind\": \"IGAUGE\",\n" +
            "      \"labels\": {\n" +
            "        \"platform\": \"yandexmini\",\n" +
            "        \"promoType\": \"plus90\",\n" +
            "        \"provider\": \"yandexplus\",\n" +
            "        \"sensor\": \"billing.promo.code.statistic.total.count.gaugei\"\n" +
            "      },\n" +
            "      \"value\": 1\n" +
            "    },\n" +
            "    {\n" +
            "      \"kind\": \"IGAUGE\",\n" +
            "      \"labels\": {\n" +
            "        \"platform\": \"any\",\n" +
            "        \"promoType\": \"plus90_by\",\n" +
            "        \"provider\": \"yandexplus\",\n" +
            "        \"sensor\": \"billing.promo.code.statistic.left.count.gaugei\"\n" +
            "      },\n" +
            "      \"value\": 1\n" +
            "    },\n" +
            "    {\n" +
            "      \"kind\": \"IGAUGE\",\n" +
            "      \"labels\": {\n" +
            "        \"platform\": \"any\",\n" +
            "        \"promoType\": \"plus90_by\",\n" +
            "        \"provider\": \"yandexplus\",\n" +
            "        \"sensor\": \"billing.promo.code.statistic.total.count.gaugei\"\n" +
            "      },\n" +
            "      \"value\": -1\n" +
            "    }\n" +
            "  ]\n" +
            "}";

    @LocalServerPort
    private int port;

    @Autowired
    private PromoCodeBaseDao promoCodeBaseDao;

    @Autowired
    private UsedDevicePromoDao usedDevicePromoDao;

    @Autowired
    private PromocodePrototypeDao promocodePrototypeDao;

    private final RestTemplate restTemplate = new RestTemplate();

    @Test
    void test_getInternalSensors() {
        var result = restTemplate.getForObject(
                "http://localhost:" + port + "/billing/monitoring/solomon",
                String.class);

        assertNotNull(result);

        // for solomon JvmRuntime#addMetrics
        assertTrue(result.contains("jvm.runtime.totalMemory"));
        // for solomon JvmThreads#addMetrics
        assertTrue(result.contains("jvm.threads.total"));
        // for solomon JvmMemory#addMetric
        assertTrue(result.contains("jvm.memory.used"));
        // for solomon JvmGc#addMetrics
        assertTrue(result.contains("jvm.gc.timeMs"));
    }

    @Test
    void test_getPromoStatisticsSensors() {
        var promo = promoCodeBaseDao.save(PromoCodeBase.create("kinopoisk", kinopoisk_a6m_plus6m, "TV", YANDEXSTATION));
        promoCodeBaseDao.save(PromoCodeBase.create("kinopoisk", kinopoisk_a6m_plus6m, "RV", YANDEXSTATION));
        promoCodeBaseDao.save(PromoCodeBase.create("yandexplus", plus90, "FV", YANDEXMINI));
        promoCodeBaseDao.save(PromoCodeBase.create("yandexplus", plus90_by, "GG"));

        usedDevicePromoDao.save(UsedDevicePromo.builder()
                .deviceId("d_id")
                .platform(YANDEXSTATION)
                .provider(PromoProvider.kinopoisk)
                .codeId(promo.getId())
                .build()
        );

        var result = restTemplate.getForObject(
                "http://localhost:" + port + "/billing/monitoring/solomon/promo",
                String.class);

        JSONAssert.assertEquals(PROMO_STATISTICS_EXPECTED_JSON, result, JSONCompareMode.NON_EXTENSIBLE);
    }

    @Test
    void testPrototypeNotCounted() {

        var prototype = promocodePrototypeDao.save(
                new PromocodePrototypeDb(null, YANDEXMINI.getName(), plus90.name(), "code_prototype", "task-1")
        );
        promoCodeBaseDao.save(PromoCodeBase.create("yandexplus", plus90, "FV2", YANDEXMINI));

        var promo = promoCodeBaseDao.save(
                PromoCodeBase.create("yandexplus", plus90, "FV", YANDEXMINI, requireNonNull(prototype.getId()))
        );

        var result = restTemplate.getForObject(
                "http://localhost:" + port + "/billing/monitoring/solomon/promo",
                String.class);


        usedDevicePromoDao.save(UsedDevicePromo.builder()
                .deviceId("d_id")
                .platform(YANDEXSTATION)
                .provider(PromoProvider.kinopoisk)
                .codeId(promo.getId())
                .build()
        );

        var expected = "{\n" +
                "  \"sensors\": [\n" +
                "    {\n" +
                "      \"kind\": \"IGAUGE\",\n" +
                "      \"labels\": {\n" +
                "        \"platform\": \"yandexmini\",\n" +
                "        \"promoType\": \"plus90\",\n" +
                "        \"provider\": \"yandexplus\",\n" +
                "        \"sensor\": \"billing.promo.code.statistic.left.count.gaugei\"\n" +
                "      },\n" +
                "      \"value\": 2\n" +
                "    },\n" +
                "    {\n" +
                "      \"kind\": \"IGAUGE\",\n" +
                "      \"labels\": {\n" +
                "        \"platform\": \"yandexmini\",\n" +
                "        \"promoType\": \"plus90\",\n" +
                "        \"provider\": \"yandexplus\",\n" +
                "        \"sensor\": \"billing.promo.code.statistic.total.count.gaugei\"\n" +
                "      },\n" +
                "      \"value\": -1\n" +
                "    }\n" +
                "  ]\n" +
                "}";

        JSONAssert.assertEquals(expected, result, JSONCompareMode.NON_EXTENSIBLE);
    }
}
