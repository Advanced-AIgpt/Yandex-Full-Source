package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.UUID;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.jdbc.core.ColumnMapRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.core.namedparam.MapSqlParameterSource;
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcTemplate;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.common.billing.model.api.SkillProductItem;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationRequest;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductsResult;
import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.dao.SkillInfoDAO;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSkillProduct;
import ru.yandex.quasar.billing.dao.UserSkillProductDao;
import ru.yandex.quasar.billing.services.TestAuthorizationService;
import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.springframework.web.client.HttpClientErrorException.BadRequest;
import static org.springframework.web.client.HttpClientErrorException.Forbidden;
import static ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult.ActivationResult;
import static ru.yandex.quasar.billing.beans.PricingOptionType.BUY;
import static ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl.UID;
import static ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl.getTestTicket;

/**
 * Test for {@link AliceBillingController}
 */
@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {
                TestConfigProvider.class,
        }
)
@AutoConfigureWebClient(registerRestTemplate = true)
class UserSkillProductsIntegrationTest {
    private static final String SKILL_NAME = "s_name";
    private static final String SKILL_IMAGE_URL = "image.com";
    private static final String PRODUCT_UUID_1 = "358f2599-b820-4b2d-8b64-6316fcb311d7";
    private static final String PRODUCT_UUID_2 = "547f9978-b720-4b2d-7b64-4211fcb311d9";
    private static final String PRODUCT_UUID_3 = "88eb7d00-a4c7-4f24-999e-9667b96fa8ce";
    private static final String PRODUCT_UUID_4 = "03644a66-2739-4ded-a880-2d01f9a0fbf0";

    @Autowired
    private SkillInfoDAO skillInfoDAO;

    @Autowired
    private UserSkillProductDao userSkillProductDao;

    @Autowired
    private UserPurchasesDAO userPurchasesDAO;

    @Autowired
    private JdbcTemplate jdbcTemplate;

    @Autowired
    private NamedParameterJdbcTemplate namedParameterJdbcTemplate;

    @LocalServerPort
    private int port;
    private final RestTemplate restTemplate = new RestTemplate();

    @Test
    @DisplayName("Успешное нахождение продуктов юзера")
    void successGetSkillProductsByUser() {
        skillInfoDAO.save(createSkillInfo("skill1", 99L));
        skillInfoDAO.save(createSkillInfo("skill2", 98L));

        insertSkillProduct(PRODUCT_UUID_1, "skill1", "p1", "2.2");
        insertSkillProduct(PRODUCT_UUID_2, "skill1", "p2", "3.3");
        insertSkillProduct(PRODUCT_UUID_3, "skill1", "p3", "8.5");
        insertSkillProduct(PRODUCT_UUID_4, "skill2", "o1", "9.3");

        insertProductToken(1L, PRODUCT_UUID_2, "skill1", "y_merchant", "code");

        insertUserSkillProduct(UID, null, PRODUCT_UUID_1, "skill1");
        insertUserSkillProduct(UID, 1L, PRODUCT_UUID_2, "skill1");
        // should not find, because user_id doesn't match
        insertUserSkillProduct("57", null, PRODUCT_UUID_3, "skill1");
        // should not find, because skill_uuid doesn't match
        insertUserSkillProduct(UID, null, PRODUCT_UUID_4, "skill2");

        UserSkillProductsResult result = requestUserSkillProducts("skill1", true).getBody();

        assertNotNull(result, "Skill product result should not be empty");
        assertEquals(2L, result.getSkillProducts().size(), "Should find only 1 product");

        var skillProducts = result.getSkillProducts().stream()
                .sorted(Comparator.comparing(SkillProductItem::getUuid))
                .collect(Collectors.toList());

        verifyResult(skillProducts.get(0), PRODUCT_UUID_1, "p1");
        verifyResult(skillProducts.get(1), PRODUCT_UUID_2, "p2");
    }

    @Test
    @DisplayName("Успешное нахождение продуктов из тегов девайсов юзера")
    void successGetSkillProductsByUserFromDeviceTags() {
        skillInfoDAO.save(createSkillInfo("skill2", 99L));

        insertSkillProduct(PRODUCT_UUID_1, "skill2", "p1", "2.2");
        insertSkillProduct(PRODUCT_UUID_2, "skill2", "p2", "5.2");

        insertUserSkillProduct(UID, null, PRODUCT_UUID_2, "skill2");

        UserSkillProductsResult result = requestUserSkillProducts("skill2", true).getBody();

        assertNotNull(result, "Skill product result should not be empty");
        assertEquals(2L, result.getSkillProducts().size(), "Should find 2 products: 1 from DB and 1 from device tag");

        var skillProducts = result.getSkillProducts().stream()
                .sorted(Comparator.comparing(SkillProductItem::getUuid))
                .collect(Collectors.toList());

        verifyResult(skillProducts.get(0), PRODUCT_UUID_1, "p1");
        verifyResult(skillProducts.get(1), PRODUCT_UUID_2, "p2");
    }

    @Test
    @DisplayName("Успешное нахождение продуктов из тегов девайсов юзера")
    void successGetSkillProductsByUser_SameProductFromDeviceTagAndDb() {
        skillInfoDAO.save(createSkillInfo("skill2", 99L));

        insertSkillProduct(PRODUCT_UUID_1, "skill2", "p1", "2.2");

        insertUserSkillProduct(UID, null, PRODUCT_UUID_1, "skill2");

        UserSkillProductsResult result = requestUserSkillProducts("skill2", true).getBody();

        assertNotNull(result, "Skill product result should not be empty");
        assertEquals(1L, result.getSkillProducts().size(),
                "Should find 1 products: same from DB and from device tag");

        verifyResult(result.getSkillProducts().get(0), PRODUCT_UUID_1, "p1");
    }

    @Test
    @DisplayName("Нет TVM тикета при получении списка продуктов пользователя")
    void authAbsentGetSkillProductsByUser() {
        var e = assertThrows(Forbidden.class, () -> requestUserSkillProducts("skill1", false));
        assertEquals(HttpStatus.FORBIDDEN, e.getStatusCode());
    }

    @Test
    @DisplayName("Успешная активация продукта с использованием токена")
    void successUserSkillProductActivation() {
        skillInfoDAO.save(createSkillInfo("skill1", 99L));
        insertSkillProduct(PRODUCT_UUID_1, "skill1", "p1", "2.2");
        insertProductToken(1L, PRODUCT_UUID_1, "skill1", "y_merchant", "code");

        var result = activateUserSkillProduct("skill1", "code", true);

        assertEquals(ActivationResult.SUCCESS, result.getActivationResult());
        assertEquals(PRODUCT_UUID_1, result.getSkillProduct().getUuid());
        assertEquals("p1", result.getSkillProduct().getName());

        var activatedProduct = userSkillProductDao.getUserSkillProduct(UID, "skill1", UUID.fromString(PRODUCT_UUID_1));
        assertTrue(activatedProduct.isPresent());
        UserSkillProduct userSkillProduct = activatedProduct.get();
        assertEquals(PRODUCT_UUID_1, userSkillProduct.getSkillProduct().getUuid().toString());
        assertEquals(SKILL_NAME, userSkillProduct.getSkillName());
        assertEquals(SKILL_IMAGE_URL, userSkillProduct.getSkillImageUrl());

        var userPurchase = userPurchasesDAO.getPurchaseInfo(getCurrentPurchaseId()).orElseThrow();
        var expectedOption = PricingOption.builder("p1", BUY, BigDecimal.ZERO, BigDecimal.ZERO, "RUB")
                .items(List.of(new PricingOption.PricingOptionLine("358f2599-b820-4b2d-8b64-6316fcb311d7",
                        BigDecimal.ZERO, BigDecimal.ZERO, "p1", BigDecimal.ONE, NdsType.nds_none)))
                .build();
        var lastUserSkillProductId = getCurrentUserSkillPurchaseId();

        assertAll(
                () -> assertEquals(Long.valueOf(UID), userPurchase.getUid()),
                () -> assertEquals(userPurchase.getPurchaseId() + "|" + PRODUCT_UUID_1,
                        userPurchase.getPurchaseToken()),
                () -> assertNotNull(userPurchase.getPurchaseDate()),
                () -> assertEquals(PurchaseInfo.Status.CLEARED, userPurchase.getStatus()),
                () -> assertEquals("skill", userPurchase.getProvider()),
                () -> assertEquals("RUB", userPurchase.getCurrencyCode()),
                () -> assertEquals(expectedOption, userPurchase.getSelectedOption()),
                () -> assertEquals(PaymentProcessor.FREE, userPurchase.getPaymentProcessor()),
                () -> assertEquals(lastUserSkillProductId, userPurchase.getUserSkillProductId())
        );
    }

    @Transactional()
    Long getCurrentPurchaseId() {
        List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                "SELECT last_value FROM PurchaseIdsSeq",
                new ColumnMapRowMapper()
        );

        return (Long) purchaseIdRes.get(0).get("last_value");
    }

    @Transactional()
    Long getCurrentUserSkillPurchaseId() {
        List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                "SELECT last_value FROM user_skill_product_seq",
                new ColumnMapRowMapper()
        );

        return (Long) purchaseIdRes.get(0).get("last_value");
    }

    @Test
    @DisplayName("Продукт уже активирован")
    void userSkillProductAlreadyActivated() {
        skillInfoDAO.save(createSkillInfo("skill1", 99L));
        insertSkillProduct(PRODUCT_UUID_1, "skill1", "p1", "2.2");
        insertProductToken(1L, PRODUCT_UUID_1, "skill1", "y_merchant", "code");

        insertUserSkillProduct(UID, 1L, PRODUCT_UUID_1, "skill1");

        var result = activateUserSkillProduct("skill1", "code", true);

        assertEquals(ActivationResult.ALREADY_ACTIVATED, result.getActivationResult());
        assertEquals(PRODUCT_UUID_1, result.getSkillProduct().getUuid());
        assertEquals("p1", result.getSkillProduct().getName());
    }

    @Test
    @DisplayName("Нет TVM тикета при активации продукта у пользователя")
    void authAbsentActivateUserSkillProduct() {
        var e = assertThrows(Forbidden.class, () -> activateUserSkillProduct("skill1", "C", false));
        assertEquals(HttpStatus.FORBIDDEN, e.getStatusCode());
    }

    private void verifyResult(SkillProductItem item, String uuid, String name) {
        assertEquals(uuid, item.getUuid());
        assertEquals(name, item.getName());
    }

    @Test
    @DisplayName("Нет TVM тикета при удалении продукта у пользователя")
    void authAbsentDeleteUserSkillProduct() {
        var e = assertThrows(Forbidden.class, () -> deleteUserSkillProduct("skill1", PRODUCT_UUID_1, false));
        assertEquals(HttpStatus.FORBIDDEN, e.getStatusCode());
    }

    @Test
    @DisplayName("Удалить продукт у пользователя")
    void deleteUserSkillProduct() {
        skillInfoDAO.save(createSkillInfo("skill1", 99L));
        insertSkillProduct(PRODUCT_UUID_1, "skill1", "p1", "2.2");
        insertProductToken(1L, PRODUCT_UUID_1, "skill1", "y_merchant", "code");

        insertUserSkillProduct(UID, 1L, PRODUCT_UUID_1, "skill1");

        var response = deleteUserSkillProduct("skill1", PRODUCT_UUID_1, true);

        assertEquals(response.getResult(), "ok");
    }

    @Test
    @DisplayName("Попытка удалить продукт, которого нет у пользователя")
    void shouldFailToDeleteUserSkillProduct() {
        skillInfoDAO.save(createSkillInfo("skill1", 99L));
        insertSkillProduct(PRODUCT_UUID_1, "skill1", "p1", "2.2");
        insertProductToken(1L, PRODUCT_UUID_1, "skill1", "y_merchant", "code");

        assertThrows(BadRequest.class, () -> deleteUserSkillProduct("skill1", PRODUCT_UUID_1, true));
    }

    private SkillInfo createSkillInfo(String skillUuid, long ownerUid) {
        return SkillInfo.builder(skillUuid)
                .ownerUid(ownerUid)
                .createdAt(Instant.now())
                .publicKey("1")
                .privateKey("1")
                .build();
    }

    private void insertProductToken(long id, String productUuid, String skillUuid, String provider, String code) {
        namedParameterJdbcTemplate.update(
                "insert into product_token (\n" +
                        "    id,\n" +
                        "    product_uuid,\n" +
                        "    skill_uuid,\n" +
                        "    provider,\n" +
                        "    code,\n" +
                        "    reusable\n" +
                        ") values (\n" +
                        "    :id,\n" +
                        "    :product_uuid,\n" +
                        "    :skill_uuid,\n" +
                        "    :provider,\n" +
                        "    :code,\n" +
                        "    :reusable\n" +
                        ")",
                new MapSqlParameterSource("id", id)
                        .addValue("product_uuid", UUID.fromString(productUuid))
                        .addValue("skill_uuid", skillUuid)
                        .addValue("provider", provider)
                        .addValue("code", code)
                        .addValue("reusable", true)
        );
    }

    private void insertSkillProduct(String uuid, String skillUuid, String productName, String price) {
        namedParameterJdbcTemplate.update(
                "insert into skill_product (\n" +
                "    uuid,\n" +
                "    skill_uuid,\n" +
                "    name,\n" +
                "    type,\n" +
                "    price,\n" +
                "    deleted\n" +
                ") values (\n" +
                "    :uuid,\n" +
                "    :skill_uuid,\n" +
                "    :name,\n" +
                "    'NON_CONSUMABLE',\n" +
                "    :price,\n" +
                "    false\n" +
                ")",
                new MapSqlParameterSource("uuid", UUID.fromString(uuid))
                        .addValue("skill_uuid", skillUuid)
                        .addValue("name", productName)
                        .addValue("price", new BigDecimal(price))
        );
    }

    private void insertUserSkillProduct(String uid, @Nullable Long tokenId, String productUuid, String skillUuid) {
        namedParameterJdbcTemplate.update(
                "insert into user_skill_product (" +
                "   uid,\n" +
                "   token_id,\n" +
                "   product_uuid,\n" +
                "   skill_uuid\n" +
                ") values (\n" +
                "   :uid,\n" +
                "   :token_id,\n" +
                "   :product_uuid,\n" +
                "   :skill_uuid\n" +
                ")",
                new MapSqlParameterSource("uid", uid)
                        .addValue("token_id", tokenId)
                        .addValue("product_uuid", UUID.fromString(productUuid))
                        .addValue("skill_uuid", skillUuid)
        );
    }

    private ResponseEntity<UserSkillProductsResult> requestUserSkillProducts(String skillUuid, boolean userTvm) {
        var uri = UriComponentsBuilder.fromUriString("http://localhost:" + port)
                .pathSegment("billing", "user", "skill_product", skillUuid)
                .build()
                .toUri();

        return restTemplate.exchange(
                uri,
                HttpMethod.GET,
                new HttpEntity<>(userTvm ? getOAuthHeader() : new HttpHeaders()),
                UserSkillProductsResult.class
        );
    }

    private UserSkillProductActivationResult activateUserSkillProduct(
            String skillUuid,
            String tokenCode,
            boolean userTvm
    ) {
        var uri = UriComponentsBuilder.fromUriString("http://localhost:" + port)
                .pathSegment("billing", "user", "skill_product", skillUuid, tokenCode)
                .build()
                .toUri();

        var request = new UserSkillProductActivationRequest(SKILL_NAME, SKILL_IMAGE_URL);
        return Objects.requireNonNull(
                restTemplate.exchange(
                        uri,
                        HttpMethod.POST,
                        new HttpEntity<>(request, userTvm ? getOAuthHeader() : new HttpHeaders()),
                        UserSkillProductActivationResult.class
                ).getBody()
        );
    }

    private SimpleResult deleteUserSkillProduct(
            String skillUuid,
            String productUuid,
            boolean userTvm
    ) {
        var uri = UriComponentsBuilder.fromUriString("http://localhost:" + port)
                .pathSegment("billing", "user", "skill_product", skillUuid, productUuid)
                .build()
                .toUri();
        return Objects.requireNonNull(
                restTemplate.exchange(
                        uri,
                        HttpMethod.DELETE,
                        new HttpEntity<>(userTvm ? getOAuthHeader() : new HttpHeaders()),
                        SimpleResult.class
                ).getBody()
        );
    }

    private HttpHeaders getOAuthHeader() {
        HttpHeaders headers = new HttpHeaders();
        headers.add("Authorization", "OAuth " + TestAuthorizationService.OAUTH_TOKEN);
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.dialogovo));
        return headers;
    }
}
