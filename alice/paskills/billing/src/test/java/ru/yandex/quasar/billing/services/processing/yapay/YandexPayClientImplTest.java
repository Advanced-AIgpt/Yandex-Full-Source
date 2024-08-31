package ru.yandex.quasar.billing.services.processing.yapay;

import java.math.BigDecimal;
import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.web.client.RestTemplateAutoConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.HttpStatus;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.test.web.client.MockRestServiceServer;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;
import static org.springframework.http.HttpMethod.GET;
import static org.springframework.http.HttpMethod.POST;
import static org.springframework.http.MediaType.APPLICATION_JSON;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.content;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.method;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withStatus;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;

@SpringJUnitConfig(classes = {TestConfigProvider.class, YandexPayClientImpl.class, AuthorizationContext.class,
        RestTemplateAutoConfiguration.class})
class YandexPayClientImplTest {
    private static final String SUCCESS_ACCESS_REQUEST_RESPONSE = "{\n" +
            "  \"code\": 200,\n" +
            "  \"data\": {\n" +
            "    \"revision\": 1,\n" +
            "    \"uid\": 756727506,\n" +
            "    \"updated\": \"2019-06-06T07:15:15.904254+00:00\",\n" +
            "    \"entity_id\": \"1234\",\n" +
            "    \"deleted\": false,\n" +
            "    \"organization\": {\n" +
            "      \"inn\": \"5043041353\",\n" +
            "      \"name\": \"Yandex\",\n" +
            "      \"englishName\": \"HH\",\n" +
            "      \"ogrn\": \"1234567890\",\n" +
            "      \"type\": \"OOO\",\n" +
            "      \"kpp\": \"504301001\",\n" +
            "      \"fullName\": \"Hoofs & Horns\",\n" +
            "      \"siteUrl\": \"pay.yandex.ru\",\n" +
            "      \"description\": null,\n" +
            "      \"scheduleText\": \"с 9 до 6\"\n" +
            "    },\n" +
            "    \"addresses\": {\n" +
            "      \"legal\": {\n" +
            "        \"country\": \"RU\",\n" +
            "        \"home\": \"16\",\n" +
            "        \"city\": \"Moscow\",\n" +
            "        \"street\": \"Lva Tolstogo\",\n" +
            "        \"zip\": \"123456\"\n" +
            "      },\n" +
            "      \"post\": {\n" +
            "        \"country\": \"RU\",\n" +
            "        \"home\": \"16\",\n" +
            "        \"city\": \"Moscow\",\n" +
            "        \"street\": \"Lva Tolstogo\",\n" +
            "        \"zip\": \"22222\"\n" +
            "      }\n" +
            "    },\n" +
            "    \"enabled\": false,\n" +
            "    \"service_merchant_id\": 289,\n" +
            "    \"service_id\": 10,\n" +
            "    \"created\": \"2019-06-06T07:15:15.904254+00:00\",\n" +
            "    \"description\": \"Test\"\n" +
            "  },\n" +
            "  \"status\": \"success\"\n" +
            "}";
    private static final String CONFLICT_ACCESS_REQUEST_RESPONSE = "{\n" +
            "  \"code\": 409,\n" +
            "  \"data\": {\n" +
            "    \"message\": \"ServiceMerchant with keys [{'service_merchant_id': 289}] already exists\",\n" +
            "    \"params\": {\n" +
            "      \"service_merchant_id\": 289\n" +
            "    }\n" +
            "  },\n" +
            "  \"status\": \"fail\"\n" +
            "}";
    @MockBean
    private TvmClient tvmClient;
    @Autowired
    private YandexPayClientImpl client;
    @Autowired
    private BillingConfig config;
    private MockRestServiceServer service;

    @BeforeEach
    void setUp() {
        service = MockRestServiceServer.bindTo(client.getRestTemplate()).build();
        when(tvmClient.getServiceTicketFor(any())).thenReturn("ticket");
    }

    @Test
    void testRequestMerchantAccess() throws AccessRequestConflictException, TokenNotFound {
        // given
        service.expect(requestTo(config.getYaPayConfig().getApiBaseUrl() + "v1/internal/service"))
                //.andExpect(header(TvmService.SERVICE_TICKET_HEADER, anyString()))
                .andExpect(method(POST))
                .andExpect(content().json("{\n" +
                        "    \"token\": \"85e6adb4-d566-48b9-b031-8af10934cdac\",\n" +
                        "    \"entity_id\": \"shop.ru\",\n" +
                        "    \"description\": \"Some description for user\"\n" +
                        "}"))
                .andRespond(withSuccess(SUCCESS_ACCESS_REQUEST_RESPONSE, APPLICATION_JSON));

        // when
        ServiceMerchantInfo merchantInfo = client.requestMerchantAccess("85e6adb4-d566-48b9-b031-8af10934cdac", "shop" +
                ".ru", "Some description for user");

        // then
        var expected = ServiceMerchantInfo.builder()
                .serviceMerchantId(289L)
                .deleted(false)
                .enabled(false)
                .entityId("1234")
                .description("Test")
                .organization(Organization.builder()
                        .inn("5043041353")
                        .name("Yandex")
                        .englishName("HH")
                        .ogrn("1234567890")
                        .type("OOO")
                        .kpp("504301001")
                        .fullName("Hoofs & Horns")
                        .siteUrl("pay.yandex.ru")
                        .scheduleText("с 9 до 6")
                        .build())
                .legalAddress("123456, RU, Moscow, Lva Tolstogo, 16")
                .build();
        assertEquals(expected, merchantInfo);
    }

    @Test
    void testRequestMerchantAccessOnConflict() throws AccessRequestConflictException {
        // given
        service.expect(requestTo(config.getYaPayConfig().getApiBaseUrl() + "v1/internal/service"))
                //.andExpect(header(TvmService.SERVICE_TICKET_HEADER, anyString()))
                .andExpect(method(POST))
                .andExpect(content().json("{\n" +
                        "    \"token\": \"85e6adb4-d566-48b9-b031-8af10934cdac\",\n" +
                        "    \"entity_id\": \"shop.ru\",\n" +
                        "    \"description\": \"Some description for user\"\n" +
                        "}"))
                .andRespond(withStatus(HttpStatus.CONFLICT).body(CONFLICT_ACCESS_REQUEST_RESPONSE)
                        .contentType(APPLICATION_JSON));

        // when
        var ex = assertThrows(AccessRequestConflictException.class,
                () -> client.requestMerchantAccess("85e6adb4-d566-48b9-b031-8af10934cdac", "shop.ru", "Some " +
                        "description for user"));

        assertEquals(289L, ex.getServiceMerchantId());
    }

    @Test
    void testMerchantInfo() throws AccessRequestConflictException {
        // given
        service.expect(requestTo(config.getYaPayConfig().getApiBaseUrl() + "v1/internal/service/289"))
                .andExpect(method(GET))
                .andRespond(withSuccess(SUCCESS_ACCESS_REQUEST_RESPONSE, APPLICATION_JSON));

        // when
        ServiceMerchantInfo merchantInfo = client.merchantInfo(289L);

        // then
        var expected = ServiceMerchantInfo.builder()
                .serviceMerchantId(289L)
                .deleted(false)
                .enabled(false)
                .entityId("1234")
                .description("Test")
                .organization(Organization.builder()
                        .inn("5043041353")
                        .name("Yandex")
                        .englishName("HH")
                        .ogrn("1234567890")
                        .type("OOO")
                        .kpp("504301001")
                        .fullName("Hoofs & Horns")
                        .siteUrl("pay.yandex.ru")
                        .scheduleText("с 9 до 6")
                        .build())
                .legalAddress("123456, RU, Moscow, Lva Tolstogo, 16")
                .build();
        assertEquals(expected, merchantInfo);
    }

    @Test
    @Disabled
    void testCreateOrder() {
        // given
        service.expect(requestTo(config.getYaPayConfig().getApiBaseUrl() + "v1/internal/order/289"))
                .andExpect(method(GET))
                .andRespond(withSuccess(SUCCESS_ACCESS_REQUEST_RESPONSE, APPLICATION_JSON));

        // when
        Order order = client.createOrder(289L, CreateOrderRequest.builder()
                .caption("caption")
                .description("description")
                .userEmail("ya@ya.ru")
                .paymethodId("card-xxxx")
                .customerUid("123123")
                .autoClear(true)
                .userDescription("userDescription")
                .returnUrl("http://ya.ru")
                .items(List.of(OrderItem.builder()
                        //.productId(1L)
                        .price(BigDecimal.TEN)
                        .currency(TrustCurrency.RUB)
                        .name("test1")
                        .amount(BigDecimal.ONE)
                        .nds(NdsType.nds_20)
                        .build()))
                .build());
    }
}
