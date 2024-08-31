package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.List;

import org.json.JSONObject;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.autoconfigure.web.client.RestClientTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.test.web.client.MockRestServiceServer;
import org.springframework.web.client.HttpClientErrorException;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.when;
import static org.springframework.http.MediaType.APPLICATION_JSON;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.content;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.header;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.method;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withBadRequest;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;
import static ru.yandex.quasar.billing.services.processing.NdsType.nds_20;
import static ru.yandex.quasar.billing.services.processing.TrustCurrency.RUB;
import static ru.yandex.quasar.billing.services.processing.trust.TrustBillingService.COUNTRY_CODE_RUSSIA;

@RestClientTest({TrustBillingClientImpl.class})
@AutoConfigureWebClient(registerRestTemplate = true)
@SpringJUnitConfig(classes = {TestConfigProvider.class, TrustClientConfig.class})
class TrustBillingClientImplTest {

    private static final String CARD_ID = "card-x8508";
    private static final String UID = "605289217";
    private static final String USER_IP = "0.0.0.0";
    private static final String TOKEN = "token";
    private static final String CARD_LIST = "{\n" +
            "    \"status\": \"success\",\n" +
            "    \"bound_payment_methods\": [\n" +
            "        {\n" +
            "            \"region_id\": 225,\n" +
            "            \"payment_method\": \"card\",\n" +
            "            \"expiration_month\": \"04\",\n" +
            "            \"binding_ts\": \"1533295778.845\",\n" +
            "            \"id\": \"card-x8508\",\n" +
            "            \"expired\": false,\n" +
            "            \"card_bank\": \"RBS BANK (ROMANIA), S.A.\",\n" +
            "            \"system\": \"MasterCard\",\n" +
            "            \"recommended_verification_type\": null,\n" +
            "            \"orig_uid\": 605289217,\n" +
            "            \"card_country\": \"ROU\",\n" +
            "            \"payment_system\": \"MasterCard\",\n" +
            "            \"card_level\": \"\",\n" +
            "            \"holder\": \"TEST TEST\",\n" +
            "            \"binding_systems\": [\n" +
            "                \"trust\"\n" +
            "            ],\n" +
            "            \"account\": \"510000****1573\",\n" +
            "            \"expiration_year\": \"2019\"\n" +
            "        }\n" +
            "    ]\n" +
            "}";
    private static final String CREATE_BASKET_PAYMENT_REQUEST = "{\n" +
            "\t\"amount\": 100,\n" +
            "\t\"currency\": \"RUB\",\n" +
            "\t\"product_id\": \"2\",\n" +
            "\t\"paymethod_id\": \"card-x8508\",\n" +
            "\t\"user_email\": \"test@yandex.ru\",\n" +
            "\t\"back_url\": \"https://test.quasar.common.yandex.ru\",\n" +
            "\t\"fiscal_nds\": \"nds_20\",\n" +
            "\t\"fiscal_title\": \"fiscalTitle\",\n" +
            "\t\"commission_category\": \"commission\"\n" +
            "}";
    private static final String CREATE_BASKET_PAYMENT_OK_RESPONSE = "{\n" +
            "    \"status\": \"success\",\n" +
            "    \"status_code\": \"payment_created\",\n" +
            "    \"purchase_token\": \"8d58e0a7c36eb4ec846a5c9fceec945e\"\n" +
            "}";
    private static final String START_PAYMENT_RESPONSE = "{\n" +
            "    \"status\": \"success\",\n" +
            "    \"card_type\": \"MasterCard\",\n" +
            "    \"uid\": \"605289217\",\n" +
            "    \"payment_method\": \"card\",\n" +
            "    \"payment_status\": \"started\",\n" +
            "    \"start_ts\": \"1533646031.085\",\n" +
            "    \"purchase_token\": \"8d58e0a7c36eb4ec846a5c9fceec945e\",\n" +
            "    \"update_ts\": \"1533643149.994\",\n" +
            "    \"currency\": \"RUB\",\n" +
            "    \"amount\": \"100.00\",\n" +
            "    \"payment_timeout\": \"1200.000\",\n" +
            "    \"paymethod_id\": \"card-x8508\",\n" +
            "    \"orders\": [\n" +
            "        {\n" +
            "            \"product_type\": \"app\",\n" +
            "            \"uid\": \"605289217\",\n" +
            "            \"paid_amount\": \"0.00\",\n" +
            "            \"current_amount\": [],\n" +
            "            \"order_ts\": \"1533643150.177\",\n" +
            "            \"current_qty\": \"0.00\",\n" +
            "            \"order_id\": \"96417721\",\n" +
            "            \"orig_amount\": \"100.00\",\n" +
            "            \"product_name\": \"ivi HD content buy\",\n" +
            "            \"product_id\": \"2\"\n" +
            "        }\n" +
            "    ],\n" +
            "    \"user_email\": \"test@yandex.ru\",\n" +
            "    \"user_account\": \"510000****1573\"\n" +
            "}";
    private static final String BASKET_STATUS_RESPONSE_ERROR = "{\n" +
            "    \"payment_resp_desc\": \"fiscal_problem\",\n" +
            "    \"payment_resp_code\": \"unknown_error\",\n" +
            "    \"uid\": \"605289217\",\n" +
            "    \"payment_method\": \"card\",\n" +
            "    \"payment_status\": \"not_authorized\",\n" +
            "    \"cancel_ts\": \"1533646032.000\",\n" +
            "    \"currency\": \"RUB\",\n" +
            "    \"orders\": [\n" +
            "        {\n" +
            "            \"product_type\": \"app\",\n" +
            "            \"uid\": \"605289217\",\n" +
            "            \"paid_amount\": \"0.00\",\n" +
            "            \"current_amount\": [],\n" +
            "            \"order_ts\": \"1533643150.177\",\n" +
            "            \"current_qty\": \"0.00\",\n" +
            "            \"order_id\": \"96417721\",\n" +
            "            \"orig_amount\": \"100.00\",\n" +
            "            \"product_name\": \"ivi HD content buy\",\n" +
            "            \"product_id\": \"2\"\n" +
            "        }\n" +
            "    ],\n" +
            "    \"user_account\": \"510000****1573\",\n" +
            "    \"paysys_sent_ts\": \"1533646032.036\",\n" +
            "    \"status\": \"success\",\n" +
            "    \"start_ts\": \"1533646031.085\",\n" +
            "    \"update_ts\": \"1533643149.994\",\n" +
            "    \"purchase_token\": \"8d58e0a7c36eb4ec846a5c9fceec945e\",\n" +
            "    \"paymethod_id\": \"card-x8508\",\n" +
            "    \"final_status_ts\": \"1533646032.000\",\n" +
            "    \"card_type\": \"MasterCard\",\n" +
            "    \"amount\": \"100.00\",\n" +
            "    \"payment_timeout\": \"1200.000\",\n" +
            "    \"user_email\": \"test@yandex.ru\"\n" +
            "}";
    private static final String BASKET_STATUS_RESPONSE_SUCCESS = "{\n" +
            "    \"rrn\": \"514523\",\n" +
            "    \"uid\": \"605289217\",\n" +
            "    \"payment_method\": \"card\",\n" +
            "    \"payment_status\": \"authorized\",\n" +
            "    \"currency\": \"RUB\",\n" +
            "    \"orders\": [\n" +
            "        {\n" +
            "            \"product_type\": \"app\",\n" +
            "            \"uid\": \"605289217\",\n" +
            "            \"paid_amount\": \"100.00\",\n" +
            "            \"current_amount\": [\n" +
            "                [\n" +
            "                    \"RUB\",\n" +
            "                    \"100.00\"\n" +
            "                ]\n" +
            "            ],\n" +
            "            \"order_ts\": \"1533646205.568\",\n" +
            "            \"fiscal_title\": \"fiscalTitle\",\n" +
            "            \"order_id\": \"96417964\",\n" +
            "            \"current_qty\": \"0.00\",\n" +
            "            \"product_id\": \"2\",\n" +
            "            \"orig_amount\": \"100.00\",\n" +
            "            \"fiscal_nds\": \"nds_20\",\n" +
            "            \"product_name\": \"ivi HD content buy\",\n" +
            "            \"commission_category\": \"commission\"\n" +
            "        }\n" +
            "    ],\n" +
            "    \"user_account\": \"510000****1573\",\n" +
            "    \"paysys_sent_ts\": \"1533646218.862\",\n" +
            "    \"current_amount\": \"100.00\",\n" +
            "    \"fiscal_receipt_url\": \"https://trust-test.yandex" +
            ".ru/checks/8d58e0a7c36eb4ec846a5c9fceec945e/receipts/a9253c15c311f35c3738b20ef2ee91cd?mode=mobile\",\n" +
            "    \"status\": \"success\",\n" +
            "    \"orig_amount\": \"100.00\",\n" +
            "    \"start_ts\": \"1533646217.827\",\n" +
            "    \"update_ts\": \"1533646205.546\",\n" +
            "    \"purchase_token\": \"8d58e0a7c36eb4ec846a5c9fceec945e\",\n" +
            "    \"paymethod_id\": \"card-x8508\",\n" +
            "    \"final_status_ts\": \"1533646218.000\",\n" +
            "    \"fiscal_status\": \"success\",\n" +
            "    \"payment_ts\": \"1533646218.000\",\n" +
            "    \"card_type\": \"MasterCard\",\n" +
            "    \"amount\": \"100.00\",\n" +
            "    \"payment_timeout\": \"1200.000\",\n" +
            "    \"user_email\": \"test@yandex.ru\"\n" +
            "}";
    private static final String CLEAR_RESPONSE_INVALID_STATE = "{\n" +
            "    \"status\": \"error\",\n" +
            "    \"status_code\": \"invalid_state\"\n" +
            "}";
    private static final String CLEAR_RESPONSE_PAYMENT_NOT_FOUND = "{\n" +
            "    \"status\": \"error\",\n" +
            "    \"status_code\": \"payment_not_found\",\n" +
            "    \"status_desc\": \"Payment not found, trust_payment_id=5b69957e910d390273e2063d\"\n" +
            "}";
    private static final String CLEAR_RESPONSE_PAYMENT_OK = "{\n" +
            "    \"status\": \"success\",\n" +
            "    \"status_code\": \"payment_is_updated\"\n" +
            "}";
    private static final String CLEAR_RESPONSE_ALREADY_CREATED = "{\n" +
            "    \"status\": \"success\",\n" +
            "    \"status_code\": \"already_cleared\"\n" +
            "}";
    private static final String UNHOLD_RESPONSE_INVALID_STATE = "{\n" +
            "    \"status\": \"error\",\n" +
            "    \"status_code\": \"invalid_state\"\n" +
            "}";
    private static final String UNHOLD_RESPONSE_OK = "{\n" +
            "    \"status\": \"success\",\n" +
            "    \"status_code\": \"payment_is_updated\"\n" +
            "}";
    private static final String CREATE_PRODUCT_REQUEST = "{\n" +
            "    \"product_type\": \"subs\",\n" +
            "    \"product_id\": \"999\",\n" +
            "    \"prices\": [\n" +
            "        {\n" +
            "            \"currency\": \"RUB\",\n" +
            "            \"price\": \"399.11\",\n" +
            "            \"start_ts\": 1533317240,\n" +
            "            \"region_id\": 225\n" +
            "        }\n" +
            "    ],\n" +
            "    \"partner_id\": 80943110,\n" +
            "    \"subs_period\": \"30D\",\n" +
            "    \"fiscal_nds\": \"nds_20\",\n" +
            "    \"fiscal_title\": \"title\",\n" +
            "    \"name\": \"test product 20180807\"\n" +
            "}";
    private static final String SUBSCRIPTION_INFO_RESPONSE = "{\n" +
            "    \"status\": \"success\",\n" +
            "    \"subs_until_ts\": \"1532786149.635\",\n" +
            "    \"product_type\": \"subs\",\n" +
            "    \"uid\": \"605289217\",\n" +
            "    \"subs_period_count\": 2,\n" +
            "    \"current_amount\": [\n" +
            "        [\n" +
            "            \"RUB\",\n" +
            "            \"399.00\"\n" +
            "        ]\n" +
            "    ],\n" +
            "    \"order_ts\": \"1530107737.760\",\n" +
            "    \"current_qty\": \"2.00\",\n" +
            "    \"next_charge_payment_method\": \"card-x8508\",\n" +
            "    \"subs_trial_period\": \"1D\",\n" +
            "    \"has_trial_period\": 1,\n" +
            "    \"subs_period\": \"30D\",\n" +
            "    \"order_id\": \"66\",\n" +
            "    \"product_id\": \"1017\",\n" +
            "    \"commission_category\": \"1100\",\n" +
            "    \"payments\": [\n" +
            "        \"c8246740a3c99bc1d6b11a8f43adb527\",\n" +
            "        \"caab8853632c2db677de00034f5be44d\"\n" +
            "    ],\n" +
            "    \"finish_ts\": \"1532786149.635\",\n" +
            "    \"subs_state\": 4,\n" +
            "    \"ready_to_pay\": 0,\n" +
            "    \"product_name\": \"ivi subscription 1_30\",\n" +
            "    \"begin_ts\": \"1530107749.635\"\n" +
            "}";
    private MockRestServiceServer server;
    @Autowired
    @Qualifier("trustBillingClient")
    private TrustBillingClientImpl client;
    @Autowired
    private BillingConfig billingConfig;
    @MockBean
    private TvmClient tvmClient;

    @BeforeEach
    void setUp() {
        when(tvmClient.getServiceTicketFor(eq(billingConfig.getTrustBillingConfig().getTvmClientId())))
                .thenReturn(TOKEN);
        server = MockRestServiceServer.bindTo(client.getRestTemplate()).build();
    }

    @Test
    void getCardsListTest() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() +
                "/v2/payment-methods?payment-method=card"))
                .andExpect(method(HttpMethod.GET))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(CARD_LIST, APPLICATION_JSON));
        List<PaymentMethod> cardsList = client.getCardsList(UID, USER_IP);

        assertEquals(List.of(PaymentMethod.builder(CARD_ID, "card")
                .system("MasterCard")
                .account("510000****1573")
                .cardBank("RBS BANK (ROMANIA), S.A.")
                .expirationMonth(4)
                .expirationYear(2019)
                .holder("TEST TEST")
                .paymentSystem("MasterCard")
                .regionId(225)
                .expired(false)
                .build()), cardsList);
        assertEquals(1, cardsList.size());
        server.verify();
    }

    @Test
    void createBasket() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andExpect(content().contentType(APPLICATION_JSON))
                // .andExpect(content().string(Matchers.equalToIgnoringWhiteSpace(CREATE_BASKET_PAYMENT_REQUEST
                // .replace("\n",""))))
                .andRespond(withSuccess(CREATE_BASKET_PAYMENT_OK_RESPONSE, APPLICATION_JSON));

        CreateBasketRequest request = CreateBasketRequest.createSimpleBasket(BigDecimal.valueOf(100L), RUB, "2",
                CARD_ID, "test@yandex.ru", "https://test.quasar.common.yandex.ru", nds_20,
                "fiscalTitle", "commission", null);
        CreateBasketResponse actual = client.createBasket(UID, USER_IP, request);

        CreateBasketResponse expected = CreateBasketResponse.success("8d58e0a7c36eb4ec846a5c9fceec945e");

        assertEquals(expected, actual);
        server.verify();
    }

    @Test
    void createBasketInvalidPaymentMethod() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andExpect(content().contentType(APPLICATION_JSON))
                .andExpect(content().json(CREATE_BASKET_PAYMENT_REQUEST))
                //.andExpect(content().string(Matchers.equalToIgnoringWhiteSpace(CREATE_BASKET_PAYMENT_REQUEST
                // .replace("\n",""))))
                .andRespond(withBadRequest().contentType(APPLICATION_JSON)
                        .body("{\n" +
                                "    \"status\": \"error\",\n" +
                                "    \"status_code\": \"invalid_payment_method\"\n" +
                                "}"));

        CreateBasketRequest request = CreateBasketRequest.createSimpleBasket(BigDecimal.valueOf(100L), RUB, "2",
                CARD_ID, "test@yandex.ru", "https://test.quasar.common.yandex.ru", nds_20,
                "fiscalTitle", "commission", null);

        try {
            CreateBasketResponse actual = client.createBasket(UID, USER_IP, request);
            fail();
        } catch (HttpClientErrorException e) {
            assertEquals(HttpStatus.BAD_REQUEST, e.getStatusCode());
            assertEquals("invalid_payment_method",
                    new JSONObject(e.getResponseBodyAsString()).optString("status_code")
            );
        }
        server.verify();
    }

    @Test
    void createBasketBadToken() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andExpect(content().contentType(APPLICATION_JSON))
                .andExpect(content().json(CREATE_BASKET_PAYMENT_REQUEST))
                .andRespond(withBadRequest().contentType(APPLICATION_JSON)
                        .body("{\n" +
                                "    \"status\": \"error\",\n" +
                                "    \"status_code\": \"unknown_error\",\n" +
                                "    \"method\": \"yandex_balance_simple.post_payments\",\n" +
                                "    \"status_desc\": \"NOT_FOUND: Object not found: Service: filter: {'token': " +
                                "'yandex_quasar_e1e7cd7684199fc4c930d62040a84d3b3'}\"\n" +
                                "}"));
        CreateBasketRequest request = CreateBasketRequest.createSimpleBasket(BigDecimal.valueOf(100L), RUB, "2",
                CARD_ID, "test@yandex.ru", "https://test.quasar.common.yandex.ru", nds_20,
                "fiscalTitle", "commission", null);
        try {
            CreateBasketResponse actual = client.createBasket(UID, USER_IP, request);
            fail();
        } catch (HttpClientErrorException e) {
            assertEquals(HttpStatus.BAD_REQUEST, e.getStatusCode());
            assertEquals("unknown_error", new JSONObject(e.getResponseBodyAsString())
                    .optString("status_code"));
        }
        server.verify();
    }

    @Test
    void startPaymentTest() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e/start"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(START_PAYMENT_RESPONSE, APPLICATION_JSON));

        client.startPayment(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");
        server.verify();
    }

    @Test
    void getPaymentShortInfoTest() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e"))
                .andExpect(method(HttpMethod.GET))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(BASKET_STATUS_RESPONSE_SUCCESS, APPLICATION_JSON));

        TrustPaymentShortInfo actual = client.getPaymentShortInfo(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");

        Instant paymentTs = ZonedDateTime.of(2018, 8, 7, 12, 50, 18, 0, ZoneId.of("UTC"))
                .toInstant();
        TrustPaymentShortInfo expected =
                new TrustPaymentShortInfo(null, null, "authorized", "510000****1573",
                        "MasterCard", new BigDecimal("100.00"), RUB, paymentTs, CARD_ID, null,
                        List.of(new TrustPaymentShortInfo.TrustOrderInfo("96417964", new BigDecimal("100.00"))));

        assertEquals(expected, actual);
        server.verify();
    }

    // if problems with nds
    @Test
    void getPaymentShortInfoTestFiscalProblem() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e"))
                .andExpect(method(HttpMethod.GET))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(BASKET_STATUS_RESPONSE_ERROR, APPLICATION_JSON));

        TrustPaymentShortInfo actual = client.getPaymentShortInfo(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");

        TrustPaymentShortInfo expected =
                new TrustPaymentShortInfo("fiscal_problem", "unknown_error", "not_authorized",
                        "510000****1573", "MasterCard", new BigDecimal("100.00"), RUB, null, CARD_ID, null,
                        List.of(new TrustPaymentShortInfo.TrustOrderInfo("96417721", new BigDecimal("100.00"))));

        assertEquals(expected, actual);
        server.verify();
    }

    @Test
    void clearPaymentTestInvalidState() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e/clear"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withBadRequest().contentType(APPLICATION_JSON).body(CLEAR_RESPONSE_INVALID_STATE));

        try {
            client.clearPayment(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");
            fail();
        } catch (HttpClientErrorException e) {
            assertEquals(HttpStatus.BAD_REQUEST, e.getStatusCode());
            assertEquals("invalid_state", new JSONObject(e.getResponseBodyAsString())
                    .optString("status_code"));
        }
        server.verify();
    }

    // if inconsistent uid and payment
    @Test
    void clearPaymentTestPaymentNotFound() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e/clear"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withBadRequest().contentType(APPLICATION_JSON).body(CLEAR_RESPONSE_PAYMENT_NOT_FOUND));

        try {
            client.clearPayment(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");
            fail();
        } catch (HttpClientErrorException e) {
            assertEquals(HttpStatus.BAD_REQUEST, e.getStatusCode());
            assertEquals("payment_not_found", new JSONObject(e.getResponseBodyAsString())
                    .optString("status_code"));
        }
        server.verify();
    }

    @Test
    void clearPaymentTestAlreadyCreated() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e/clear"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(CLEAR_RESPONSE_ALREADY_CREATED, APPLICATION_JSON));

        client.clearPayment(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");
        server.verify();
    }

    @Test
    void clearPaymentTest() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e/clear"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(CLEAR_RESPONSE_PAYMENT_OK, APPLICATION_JSON));

        client.clearPayment(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");
        server.verify();
    }

    @Test
    void unholdPaymentTestInvalidState() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e/unhold"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withBadRequest().contentType(APPLICATION_JSON).body(UNHOLD_RESPONSE_INVALID_STATE));

        try {
            client.unholdPayment(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");
        } catch (HttpClientErrorException e) {
            assertEquals(HttpStatus.BAD_REQUEST, e.getStatusCode());
            assertEquals("invalid_state", new JSONObject(e.getResponseBodyAsString())
                    .optString("status_code"));
        }
        server.verify();
    }

    @Test
    void unholdPaymentTest() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/payments" +
                "/8d58e0a7c36eb4ec846a5c9fceec945e/unhold"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(UNHOLD_RESPONSE_OK, APPLICATION_JSON));

        client.unholdPayment(UID, USER_IP, "8d58e0a7c36eb4ec846a5c9fceec945e");
        server.verify();
    }

    @Test
    void createProductTest() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/products"))
                .andExpect(method(HttpMethod.POST))
                // .andExpect(header("X-Uid", uid))
                // .andExpect(header("X-User-Ip", userIp))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andExpect(content().json(CREATE_PRODUCT_REQUEST))
                .andRespond(withSuccess("{\n" +
                        "    \"status\": \"success\"\n" +
                        "}", APPLICATION_JSON));

        // amediateka partner_id
        CreateProductRequest request =
                CreateProductRequest.createSubscriptionProduct("999", 80943110L,
                        "test product 20180807", "30D",
                        CreateProductRequest.Price.createPrice(RUB, BigDecimal.valueOf(399.11), COUNTRY_CODE_RUSSIA,
                                1533317240L),
                        nds_20, "title");
        client.createProduct(request);
        server.verify();
    }

    @Test
    void getSubscriptionShortInfoTest() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() +
                "/v2/subscriptions/66"))
                .andExpect(method(HttpMethod.GET))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess(SUBSCRIPTION_INFO_RESPONSE, APPLICATION_JSON));

        SubscriptionShortInfo actual = client.getSubscriptionShortInfo(UID, USER_IP, "66");

        Instant at =
                Instant.ofEpochMilli(new BigDecimal("1532786149.635").multiply(BigDecimal.valueOf(1000))
                        .longValue());
        SubscriptionShortInfo expected = new SubscriptionShortInfo("success", at, at);

        assertEquals(expected, actual);
        server.verify();
    }

    @Test
    void createOrdersBatch() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/orders_batch"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andExpect(content().json("{\"orders\":[{\"product_id\":\"2\"},{\"product_id\":\"2\"}]}"))
                .andRespond(withSuccess("{\"status\": \"success\", \"updated_count\": 0, \"orders\": [{\"order_id\": " +
                        "\"97393409\"}, {\"order_id\": \"97393412\"}], \"inserted_count\": 2}", APPLICATION_JSON));
        // to record set X-Service-Token correctly
        List<String> orders = client.createOrdersBatch(UID, USER_IP, List.of("2", "2"));

        assertEquals(List.of("97393409", "97393412"), orders);
    }

    @Test
    void createOrders() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/orders"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andExpect(content().json("{\"product_id\":\"2\"}"))
                .andRespond(withSuccess("{\"status\": \"success\", \"status_code\": \"created\", \"order_id\": " +
                        "\"97393409\", \"product_id\": \"2\"}", APPLICATION_JSON));
        // to record set X-Service-Token correctly
        String orderId = client.createOrder(UID, USER_IP, "2");

        assertEquals("97393409", orderId);
    }

    @Test
    void testCreateRefund() {
        // given
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/refunds"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andExpect(content().json("{\n" +
                        "    \"purchase_token\": \"4767e33182feb7ed97a9fd157f2358bf\",\n" +
                        "    \"reason_desc\": \"Cancel payment BILLINGSUP-31\",\n" +
                        "    \"orders\": [\n" +
                        "        {\n" +
                        "            \"delta_amount\": \"399.00\",\n" +
                        "            \"order_id\": \"2833\"\n" +
                        "        }\n" +
                        "    ]\n" +
                        "}"))
                .andRespond(withSuccess("{\"status\": \"success\", \"trust_refund_id\": " +
                        "\"5c31cf15bb044914caec5880\"}", APPLICATION_JSON));

        // when
        SubscriptionPaymentRefundParams refundParams = SubscriptionPaymentRefundParams.subscriptionPayment(
                "4767e33182feb7ed97a9fd157f2358bf",
                "Cancel payment BILLINGSUP-31",
                "2833",
                new BigDecimal("399.00"));
        CreateRefundResponse actual = client.createRefund(UID, USER_IP, refundParams);

        // then
        CreateRefundResponse expected = new CreateRefundResponse(CreateRefundStatus.success,
                "5c31cf15bb044914caec5880");
        assertEquals(expected, actual);

        server.verify();
    }

    @Test
    void testStartRefund() {
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/refunds" +
                "/5c31cf15bb044914caec5880/start"))
                .andExpect(method(HttpMethod.POST))
                .andExpect(header("X-Uid", UID))
                .andExpect(header("X-User-Ip", USER_IP))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, TOKEN))
                .andRespond(withSuccess("{\"status\": \"wait_for_notification\", \"status_desc\": \"refund is in " +
                        "queue\"}", APPLICATION_JSON));

        // when
        TrustRefundInfo actual = client.startRefund(UID, USER_IP, "5c31cf15bb044914caec5880");

        // then
        TrustRefundInfo expected =
                new TrustRefundInfo(RefundStatus.wait_for_notification, "refund is in queue", null);
        assertEquals(expected, actual);
    }

    @Test
    void testRefundStatusInitial() {
        // given
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/refunds" +
                "/5c31cf15bb044914caec5880"))
                .andExpect(method(HttpMethod.GET))
                .andRespond(withSuccess("{\"status\": \"wait_for_notification\", \"status_desc\": \"refund is in " +
                        "queue\"}", APPLICATION_JSON));

        // when
        TrustRefundInfo actual = client.getRefundStatus("5c31cf15bb044914caec5880");

        // then
        TrustRefundInfo expected =
                new TrustRefundInfo(RefundStatus.wait_for_notification, "refund is in queue", null);
        assertEquals(expected, actual);

    }

    @Test
    void testRefundStatusOk() {
        // given
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/refunds" +
                "/5c31cf15bb044914caec5880"))
                .andExpect(method(HttpMethod.GET))
                .andRespond(withSuccess("{\"status\": \"success\", \"fiscal_receipt_url\": \"https://trust.yandex" +
                        ".ru/checks/675d1a1973aae32905f9a198aceabcfb/receipts/5c31cf15bb044914caec5880?mode=mobile\"," +
                        " \"status_desc\": \"refund sent to payment system\"}", APPLICATION_JSON));

        // when
        TrustRefundInfo actual = client.getRefundStatus("5c31cf15bb044914caec5880");

        // then
        TrustRefundInfo expected =
                new TrustRefundInfo(RefundStatus.success, "refund sent to payment system",
                        "https://trust.yandex.ru/checks/675d1a1973aae32905f9a198aceabcfb" +
                                "/receipts/5c31cf15bb044914caec5880?mode=mobile");
        assertEquals(expected, actual);

    }

    @Test
    void testRefundStatusFailure() {
        // given
        server.expect(requestTo(billingConfig.getTrustBillingConfig().getApiBaseUrl() + "/v2/refunds" +
                "/5c31cf15bb044914caec5880"))
                .andExpect(method(HttpMethod.GET))
                .andRespond(withSuccess("{\"status\": \"failed\", \"status_desc\": \"some failure description\"}",
                        APPLICATION_JSON));

        // when
        TrustRefundInfo actual = client.getRefundStatus("5c31cf15bb044914caec5880");

        // then
        TrustRefundInfo expected = new TrustRefundInfo(RefundStatus.failed, "some failure description", null);
        assertEquals(expected, actual);

    }

}
