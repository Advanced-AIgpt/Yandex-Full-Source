package ru.yandex.quasar.billing.services.mediabilling;

import org.hamcrest.Matchers;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.autoconfigure.web.client.RestClientTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.MediaType;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.test.web.client.MockRestServiceServer;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.AuthorizationContext;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.queryParam;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;

@RestClientTest(MediaBillingClientImpl.class)
@AutoConfigureWebClient(registerRestTemplate = true)
@SpringJUnitConfig(classes = {TestConfigProvider.class, MediaBillingClientImpl.class})
class MediaBillingClientImplTest {

    private static final String CODE_NOT_EXISTS_RESPONSE = "{\n" +
            "    \"invocationInfo\": {\n" +
            "        \"hostname\": \"vla2-0546-1.z4ll6tzwa5dgp3i6.vla.yp-c.yandex.net\",\n" +
            "        \"action\": \"POST_CommonAccountActionContainer.consumePromoCode/consume-promo-code\",\n" +
            "        \"app-name\": \"music-web\",\n" +
            "        \"app-version\": \"2018-11-12.stable-27.4194168 (exported; 2018-11-12 11:52)\",\n" +
            "        \"req-id\": \"036aa9bd66d429e2d83fbc9ce4b63a61\",\n" +
            "        \"exec-duration-millis\": \"159\"\n" +
            "    },\n" +
            "    \"result\": {\n" +
            "        \"status\": \"code-not-exists\",\n" +
            "        \"statusDesc\": \"Gift code does not exist.\",\n" +
            "        \"accountStatus\": {\n" +
            "            \"account\": {\n" +
            "                \"now\": \"2018-11-12T13:13:01+00:00\",\n" +
            "                \"uid\": 605289217,\n" +
            "                \"region\": 225,\n" +
            "                \"fullName\": \"Pupkin Vasily\",\n" +
            "                \"secondName\": \"Pupkin\",\n" +
            "                \"firstName\": \"Vasily\",\n" +
            "                \"displayName\": \"yndx-quasar-test4\",\n" +
            "                \"serviceAvailable\": true,\n" +
            "                \"hostedUser\": false\n" +
            "            },\n" +
            "            \"permissions\": {\n" +
            "                \"until\": \"2019-08-30T20:59:59+00:00\",\n" +
            "                \"values\": [\n" +
            "                    \"feed-play\",\n" +
            "                    \"radio-play\",\n" +
            "                    \"mix-play\",\n" +
            "                    \"radio-skips\",\n" +
            "                    \"library-play\",\n" +
            "                    \"non-shuffled-play\",\n" +
            "                    \"high-quality\",\n" +
            "                    \"background-play\",\n" +
            "                    \"landing-play\",\n" +
            "                    \"library-cache\",\n" +
            "                    \"ads-skips\"\n" +
            "                ],\n" +
            "                \"default\": [\n" +
            "                    \"feed-play\",\n" +
            "                    \"radio-play\",\n" +
            "                    \"mix-play\",\n" +
            "                    \"landing-play\"\n" +
            "                ]\n" +
            "            },\n" +
            "            \"subscription\": {\n" +
            "                \"nonAutoRenewable\": {\n" +
            "                    \"start\": \"2018-08-28T16:04:08+03:00\",\n" +
            "                    \"end\": \"2019-08-28T23:59:59+03:00\"\n" +
            "                },\n" +
            "                \"canStartTrial\": false,\n" +
            "                \"mcdonalds\": false\n" +
            "            },\n" +
            "            \"subeditor\": false\n" +
            "        }\n" +
            "    }\n" +
            "}";
    private static final String SUCCESS_RESPONSE = "{\n" +
            "    \"invocationInfo\": {\n" +
            "        \"hostname\": \"man2-1084-1.lthirbvijxfr3vbu.man.yp-c.yandex.net\",\n" +
            "        \"action\": \"POST_CommonAccountActionContainer.consumePromoCode/consume-promo-code\",\n" +
            "        \"app-name\": \"music-web\",\n" +
            "        \"app-version\": \"2018-11-12.stable-27.4194168 (exported; 2018-11-12 11:52)\",\n" +
            "        \"req-id\": \"fde8dc753ac0d748ddd76ad0ea58f6c0\",\n" +
            "        \"exec-duration-millis\": \"1752\"\n" +
            "    },\n" +
            "    \"result\": {\n" +
            "        \"status\": \"success\",\n" +
            "        \"statusDesc\": \"\",\n" +
            "        \"orderId\": 17080966,\n" +
            "        \"givenDays\": 30,\n" +
            "        \"accountStatus\": {\n" +
            "            \"account\": {\n" +
            "                \"now\": \"2018-11-12T13:57:33+00:00\",\n" +
            "                \"uid\": 605289217,\n" +
            "                \"region\": 225,\n" +
            "                \"fullName\": \"Pupkin Vasily\",\n" +
            "                \"secondName\": \"Pupkin\",\n" +
            "                \"firstName\": \"Vasily\",\n" +
            "                \"displayName\": \"yndx-quasar-test4\",\n" +
            "                \"serviceAvailable\": true,\n" +
            "                \"hostedUser\": false\n" +
            "            },\n" +
            "            \"permissions\": {\n" +
            "                \"until\": \"2019-09-29T20:59:59+00:00\",\n" +
            "                \"values\": [\n" +
            "                    \"library-play\",\n" +
            "                    \"library-cache\",\n" +
            "                    \"radio-play\",\n" +
            "                    \"mix-play\",\n" +
            "                    \"ads-skips\",\n" +
            "                    \"non-shuffled-play\",\n" +
            "                    \"landing-play\",\n" +
            "                    \"feed-play\",\n" +
            "                    \"radio-skips\",\n" +
            "                    \"high-quality\",\n" +
            "                    \"background-play\"\n" +
            "                ],\n" +
            "                \"default\": [\n" +
            "                    \"radio-play\",\n" +
            "                    \"mix-play\",\n" +
            "                    \"landing-play\",\n" +
            "                    \"feed-play\"\n" +
            "                ]\n" +
            "            },\n" +
            "            \"subscription\": {\n" +
            "                \"nonAutoRenewable\": {\n" +
            "                    \"start\": \"2018-08-28T16:04:08+03:00\",\n" +
            "                    \"end\": \"2019-09-27T23:59:59+03:00\"\n" +
            "                },\n" +
            "                \"canStartTrial\": false,\n" +
            "                \"mcdonalds\": false\n" +
            "            },\n" +
            "            \"subeditor\": false\n" +
            "        }\n" +
            "    }\n" +
            "}";
    private static final String SUCCESS_SUBMIT_ORDER = "{\n" +
            "  \"invocationInfo\": {\n" +
            "    \"hostname\": \"vla2-0546-1.z4ll6tzwa5dgp3i6.vla.yp-c.yandex.net\",\n" +
            "    \"action\": \"POST_CommonAccountActionContainer.consumePromoCode/consume-promo-code\",\n" +
            "    \"app-name\": \"music-web\",\n" +
            "    \"app-version\": \"2018-11-12.stable-27.4194168 (exported; 2018-11-12 11:52)\",\n" +
            "    \"req-id\": \"036aa9bd66d429e2d83fbc9ce4b63a61\",\n" +
            "    \"exec-duration-millis\": \"159\"\n" +
            "  },\n" +
            "  \"result\": {\n" +
            "    \"status\": \"success\",\n" +
            "    \"orderId\": 5014075,\n" +
            "    \"trustPaymentId\": \"5c5426af910d390173cd18fe\"\n" +
            "  }\n" +
            "}";
    private static final String FAILED_TO_CREATE_PAYMENT = "{\n" +
            "  \"invocationInfo\": {\n" +
            "    \"hostname\": \"sas2-1182-sas-music-prestable-back-21802.gencfg-c.yandex.net\",\n" +
            "    \"action\": \"POST_CommonAccountActionContainer.consumePromoCode/consume-promo-code\",\n" +
            "    \"app-name\": \"music-web\",\n" +
            "    \"app-version\": \"2019-03-28.stable-110.4719616 (exported; 2019-03-28 21:26)\",\n" +
            "    \"req-id\": \"84652bede7e4f2a069444aded702d7e4\",\n" +
            "    \"exec-duration-millis\": \"534\"\n" +
            "  },\n" +
            "  \"result\": {\n" +
            "    \"status\": \"failed-to-create-payment\",\n" +
            "    \"statusDesc\": \"An error occurred. Please contact support.\",\n" +
            "    \"accountStatus\": {\n" +
            "      \"account\": {\n" +
            "        \"now\": \"2019-04-02T07:33:31+00:00\",\n" +
            "        \"uid\": 41733436,\n" +
            "        \"region\": 225,\n" +
            "        \"fullName\": \"Фимушкин Данил\",\n" +
            "        \"secondName\": \"Фимушкин\",\n" +
            "        \"firstName\": \"Данил\",\n" +
            "        \"displayName\": \"danilfimushkin\",\n" +
            "        \"birthday\": \"1989-03-29\",\n" +
            "        \"serviceAvailable\": true,\n" +
            "        \"hostedUser\": false\n" +
            "      },\n" +
            "      \"permissions\": {\n" +
            "        \"until\": \"2019-05-02T07:33:31+00:00\",\n" +
            "        \"values\": [\n" +
            "          \"mix-play\",\n" +
            "          \"landing-play\",\n" +
            "          \"feed-play\",\n" +
            "          \"radio-play\"\n" +
            "        ],\n" +
            "        \"default\": [\n" +
            "          \"mix-play\",\n" +
            "          \"landing-play\",\n" +
            "          \"feed-play\",\n" +
            "          \"radio-play\"\n" +
            "        ]\n" +
            "      },\n" +
            "      \"subscription\": {\n" +
            "        \"canStartTrial\": false,\n" +
            "        \"mcdonalds\": false\n" +
            "      },\n" +
            "      \"subeditor\": false,\n" +
            "      \"subeditorLevel\": 0\n" +
            "    }\n" +
            "  }\n" +
            "}";
    private final String uid = "999";
    private final String token = "token";
    private final String code1 = "code1";
    private final String cardId = "card-xxx";
    private final String email = "ya@ya.ru";
    private final String productId = "ru.yandex.mobile.music.1month.autorenewable.native.web.3month.trial.plus.169";
    @Autowired
    private MediaBillingClientImpl mediaBillingClient;
    @Autowired
    private MockRestServiceServer server;
    @Autowired
    private BillingConfig billingConfig;
    @MockBean
    private TvmClient tvmClient;
    @MockBean
    private AuthorizationContext authorizationContext;

    @BeforeEach
    void setUp() {
        when(tvmClient.getServiceTicketFor("music")).thenReturn("ticket");
        when(tvmClient.getServiceTicketFor("mediabilling")).thenReturn("ticket");
        when(authorizationContext.getUserIp()).thenReturn("127.0.0.1");
    }

    /*@Test
    @Disabled
    void unauthorized() {
        // given
        server.expect(requestTo(Matchers.startsWith(billingConfig.getMusicApiConfig().getApiUrl() + "/internal-api" +
                        "/account/consume-promo-code")))
                .andExpect(queryParam("__uid", uid))
                .andExpect(queryParam("code", code1))
                .andExpect(queryParam("language", "en"))
                .andExpect(method(HttpMethod.POST))
                .andRespond(withUnauthorizedRequest().body("""
                        {
                            "invocationInfo": {
                                "req-id": "2743306b9a9e5d276170f4fff5df8ed7",
                                "hostname": "sas2-7044-ccd-sas-music-stable-0b1-20086.gencfg-c.yandex.net",
                                "exec-duration-millis": 0
                            },
                            "error": {
                                "name": "not-authenticated",
                                "message": ""
                            }
                        }""").contentType(MediaType.APPLICATION_JSON));

        // when
        try {
            mediaBillingClient.activatePromoCode(uid, code1, cardId, email);
            fail();
        } catch (HttpClientErrorException e) {
            // then
            assertEquals(HttpStatus.UNAUTHORIZED, e.getStatusCode());
        }

        server.verify();

    }

    @Test
    void successCaseTest() {
        // given
        server.expect(requestTo(Matchers.startsWith(billingConfig.getMusicApiConfig().getApiUrl() + "/internal-api" +
                        "/account/consume-promo-code")))
                .andExpect(queryParam("__uid", uid))
                .andExpect(queryParam("code", code1))
                .andExpect(queryParam("language", "en"))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "ticket"))
                .andExpect(method(HttpMethod.POST))
                .andRespond(withSuccess(SUCCESS_RESPONSE, MediaType.APPLICATION_JSON));

        // when
        MediaBillingClient.MusicPromoActivationResult status = mediaBillingClient.activatePromoPeriod(uid, code1,
                cardId, email);

        // then
        assertEquals(MediaBillingClient.MusicPromoActivationResult.SUCCESS, status);
        server.verify();
    }

    @Test
    void testSuccessAfterRetry() {
        // given
        server.expect(requestTo(Matchers.startsWith(billingConfig.getMusicApiConfig().getApiUrl() + "/internal-api" +
                        "/account/consume-promo-code")))
                .andExpect(queryParam("__uid", uid))
                .andExpect(queryParam("code", code1))
                .andExpect(queryParam("language", "en"))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "ticket"))
                .andExpect(method(HttpMethod.POST))
                .andRespond(withSuccess(FAILED_TO_CREATE_PAYMENT, MediaType.APPLICATION_JSON));

        server.expect(requestTo(Matchers.startsWith(billingConfig.getMusicApiConfig().getApiUrl() + "/internal-api" +
                        "/account/consume-promo-code")))
                .andExpect(queryParam("__uid", uid))
                .andExpect(queryParam("code", code1))
                .andExpect(queryParam("language", "en"))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "ticket"))
                .andExpect(method(HttpMethod.POST))
                .andRespond(withSuccess(SUCCESS_RESPONSE, MediaType.APPLICATION_JSON));

        // when
        MediaBillingClient.MusicPromoActivationResult status = mediaBillingClient.activatePromoPeriod(uid, code1,
                cardId, email);

        // then
        assertEquals(MediaBillingClient.MusicPromoActivationResult.SUCCESS, status);
        server.verify();
    }

    @Test
    void codeNotExists() {
        // given
        server.expect(requestTo(Matchers.startsWith(billingConfig.getMusicApiConfig().getApiUrl() + "/internal-api" +
                        "/account/consume-promo-code")))
                .andExpect(queryParam("__uid", uid))
                .andExpect(queryParam("code", code1))
                .andExpect(queryParam("language", "en"))
                .andExpect(method(HttpMethod.POST))
                .andRespond(withSuccess(CODE_NOT_EXISTS_RESPONSE, MediaType.APPLICATION_JSON));

        // when
        MediaBillingClient.MusicPromoActivationResult status = mediaBillingClient.activatePromoPeriod(uid, code1,
                cardId, email);

        // then
        assertEquals(MediaBillingClient.MusicPromoActivationResult.CODE_NOT_EXISTS, status);
        server.verify();
    }*/

    @Test
    void testSubmitNativeOrderSuccess() {

        server.expect(requestTo(Matchers.containsString("/internal-api/account/submit-native-order")))
                .andExpect(queryParam("__uid", uid))
                .andExpect(queryParam("ip", authorizationContext.getUserIp()))
                .andExpect(queryParam("productId", productId))
                .andExpect(queryParam("paymentMethodId", cardId))
                .andExpect(queryParam("activateImmediately", "true"))
                .andRespond(withSuccess(SUCCESS_SUBMIT_ORDER, MediaType.APPLICATION_JSON));

        SubmitNativeOrderResult result = mediaBillingClient.submitNativeOrder(uid, productId, cardId);

        assertEquals(SubmitNativeOrderResult.success(5014075, "5c5426af910d390173cd18fe"), result);
    }

    @Test
    void testMBErrorHandling() {
        String response = """
                {
                  "invocationInfo": {
                    "req-id": "1643116057842899-6716716579955812881",
                    "hostname": "mediabilling-prod-api-man-47.man.yp-c.yandex.net",
                    "action": "POST_http://api.mediabilling.yandex.net/promo-codes/activate",
                    "app-name": "mediabilling-api",
                    "app-version": "2022-01-24.stable-298.9063462 (9063462; 2022-01-24T13:17:46)"
                  },
                  "result": {
                    "name": "CODE_NOT_EXISTS"
                  }
                }
                """;

        assertEquals(new ErrorResponseDto("CODE_NOT_EXISTS", null), mediaBillingClient.parseErrorResponse(response));
    }
}
