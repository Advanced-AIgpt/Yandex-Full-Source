package ru.yandex.alice.paskill.dialogovo.service.purchase;

import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Optional;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jdk8.Jdk8Module;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.fasterxml.jackson.module.kotlin.KotlinModule;
import com.yandex.ydb.core.StatusCode;
import com.yandex.ydb.table.Session;
import com.yandex.ydb.table.transaction.TxControl;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskill.dialogovo.external.ApiVersion;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ConfirmPurchase;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Directives;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.utils.BaseYdbTest;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static com.fasterxml.jackson.databind.DeserializationFeature.READ_UNKNOWN_ENUM_VALUES_USING_DEFAULT_VALUE;
import static com.fasterxml.jackson.databind.SerializationFeature.FAIL_ON_EMPTY_BEANS;
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class PurchaseCompleteResponseDaoImplTest extends BaseYdbTest {
    private static final Duration TIMEOUT = Duration.ofSeconds(10);
    private static final String USER_ID = "1";
    private static final String SKILL_ID = "skill-id";
    private static final String PURCHASE_OFFER_UUID = "purchase-uuid-1";
    private static final PurchaseCompleteResponseKey RESPONSE_KEY =
            new PurchaseCompleteResponseKey(USER_ID, SKILL_ID, PURCHASE_OFFER_UUID);
    private static final Instant NOW = Instant.now().truncatedTo(ChronoUnit.MICROS);
    private static final String TEXT = "expected-text";
    private static final String TTS = "expected-tts";

    private PurchaseCompleteResponseDaoImpl responseDao;
    private WebhookResponse response;
    private ObjectMapper objectMapper;

    @BeforeEach
    @Override
    public void setUp() throws Exception {
        super.setUp();
        objectMapper = new ObjectMapper().registerModules(
                new Jdk8Module(),
                new KotlinModule(),
                new JavaTimeModule()
        );
        objectMapper.getDeserializationConfig().withFeatures(READ_UNKNOWN_ENUM_VALUES_USING_DEFAULT_VALUE);
        objectMapper.getSerializationConfig().withoutFeatures(FAIL_ON_EMPTY_BEANS);
        responseDao = new PurchaseCompleteResponseDaoImpl(ydbClient, objectMapper, new MetricRegistry(), TIMEOUT);

        response = WebhookResponse.builder()
                .response(Optional.of(Response.builder()
                        .text(TEXT)
                        .tts(TTS)
                        .directives(Optional.of(new Directives(
                                Optional.empty(),
                                Optional.empty(),
                                Optional.empty(),
                                Optional.empty(),
                                Optional.empty(),
                                Optional.of(ConfirmPurchase.INSTANCE),
                                Optional.empty(),
                                Optional.empty(),
                                Optional.empty()
                        )))
                        .build()))
                .version(ApiVersion.V1_0)
                .build();
    }

    @Test
    void testWarmUpDoesntFail() {
        responseDao.prepareQueries();
    }

    @Test
    void findResponse() {
        withSession(this::insertResponse);

        var ydbResponseO = responseDao.findResponse(RESPONSE_KEY);
        assertTrue(ydbResponseO.isPresent());
        var ydbResponse = ydbResponseO.get();
        assertEquals(TEXT, ydbResponse.getResponse().get().getText());
        assertEquals(TTS, ydbResponse.getResponse().get().getTts());
    }

    @Test
    void storeResponse() {
        responseDao.storeResponse(RESPONSE_KEY, NOW, response);

        withSession(session -> {
            var reader = session.executeDataQuery(
                    "pragma TablePathPrefix(\"" + ydbDatabase + "\");\n" +
                            "select \n" +
                            "    user_id,\n" +
                            "    skill_id,\n" +
                            "    purchase_offer_uuid,\n" +
                            "    `timestamp`,\n" +
                            "    response\n" +
                            "from purchase_complete_skill_response\n" +
                            "where user_id_hash == Digest::CityHash('" + USER_ID + "')\n" +
                            "   and user_id == '" + USER_ID + "'\n" +
                            "   and skill_id == '" + SKILL_ID + "'\n" +
                            "   and purchase_offer_uuid == '" + PURCHASE_OFFER_UUID + "';",
                    TxControl.serializableRw()
            ).join().expect("").getResultSet(0);
            reader.next();

            assertEquals(USER_ID, reader.getColumn("user_id").getString(UTF_8));
            assertEquals(SKILL_ID, reader.getColumn("skill_id").getString(UTF_8));
            assertEquals(PURCHASE_OFFER_UUID, reader.getColumn("purchase_offer_uuid").getString(UTF_8));
            assertEquals(NOW, reader.getColumn("timestamp").getTimestamp());

            WebhookResponse storedResponse = convertToWebhookResponse(reader.getColumn("response").getJson());
            assertEquals(TTS, storedResponse.getResponse().get().getTts());
            assertEquals(TEXT, storedResponse.getResponse().get().getText());
        });
    }

    @Test
    void deleteResponse() {
        withSession(this::insertResponse);

        responseDao.deleteResponse(RESPONSE_KEY);

        var responseO = responseDao.findResponse(RESPONSE_KEY);
        assertTrue(responseO.isEmpty());
    }

    private void insertResponse(Session session) {
        var insertStatus = session.executeDataQuery(
                "pragma TablePathPrefix(\"" + ydbDatabase + "\");\n" +
                        "insert into purchase_complete_skill_response (\n" +
                        "    user_id_hash,\n" +
                        "    user_id,\n" +
                        "    skill_id,\n" +
                        "    purchase_offer_uuid,\n" +
                        "    `timestamp`,\n" +
                        "    response\n" +
                        ") values (\n" +
                        "   Digest::CityHash('" + USER_ID + "'),\n" +
                        "   '" + USER_ID + "',\n" +
                        "   '" + SKILL_ID + "',\n" +
                        "   '" + PURCHASE_OFFER_UUID + "',\n" +
                        "   cast('" + NOW + "' as timestamp),\n" +
                        "   cast('" + getResponseAsString() + "' as json)\n" +
                        ");\n",
                TxControl.serializableRw().setCommitTx(true)
        ).join().toStatus();
        if (insertStatus.getCode() != StatusCode.SUCCESS) {
            throw new RuntimeException("Can't insert row");
        }
    }

    private String getResponseAsString() {
        try {
            return objectMapper.writeValueAsString(response);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    private WebhookResponse convertToWebhookResponse(String res) {
        try {
            return objectMapper.readValue(res, WebhookResponse.class);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }
}
