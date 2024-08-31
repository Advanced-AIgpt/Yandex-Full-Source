package ru.yandex.alice.paskill.dialogovo.service.purchase;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.function.Function;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.yandex.ydb.table.query.Params;
import com.yandex.ydb.table.result.ResultSetReader;
import com.yandex.ydb.table.transaction.TxControl;
import com.yandex.ydb.table.values.PrimitiveValue;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient;
import ru.yandex.alice.paskills.common.solomon.utils.Instrument;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

class PurchaseCompleteResponseDaoImpl implements PurchaseCompleteResponseDao {
    private static final Logger logger = LogManager.getLogger();

    private static final String SELECT_QUERY = "" +
            "--!syntax_v1\n" +
            "declare $skillId AS String;\n" +
            "declare $userId AS String;\n" +
            "declare $purchaseOfferUuid AS String;\n" +
            "\n" +
            "$hashedUserId = Digest::CityHash($userId);\n" +
            "\n" +
            "select \n" +
            "    response\n" +
            "from purchase_complete_skill_response\n" +
            "where user_id_hash == $hashedUserId\n" +
            "   and user_id == $userId\n" +
            "   and skill_id == $skillId\n" +
            "   and purchase_offer_uuid == $purchaseOfferUuid;";

    private static final String STORE_QUERY = "" +
            "--!syntax_v1\n" +
            "declare $userId AS String;\n" +
            "declare $skillId AS String;\n" +
            "declare $purchaseOfferUuid AS String;\n" +
            "declare $response AS Json;\n" +
            "declare $timestamp AS Timestamp;\n" +
            "\n" +
            "$hashedUserId = Digest::CityHash($userId);\n" +
            "\n" +
            "replace into purchase_complete_skill_response (\n" +
            "    user_id_hash,\n" +
            "    user_id,\n" +
            "    skill_id,\n" +
            "    purchase_offer_uuid,\n" +
            "    `timestamp`,\n" +
            "    response\n" +
            ") values (\n" +
            "    $hashedUserId,\n" +
            "    $userId,\n" +
            "    $skillId,\n" +
            "    $purchaseOfferUuid,\n" +
            "    $timestamp,\n" +
            "    $response\n" +
            ");";

    private static final String DELETE_QUERY = "" +
            "--!syntax_v1\n" +
            "declare $userId AS String;\n" +
            "declare $skillId AS String;\n" +
            "declare $purchaseOfferUuid AS String;\n" +
            "\n" +
            "$hashedUserId = Digest::CityHash($userId);\n" +
            "\n" +
            "delete from purchase_complete_skill_response \n" +
            "where user_id_hash = $hashedUserId\n" +
            "    and user_id = $userId\n" +
            "    and skill_id = $skillId\n" +
            "    and purchase_offer_uuid = $purchaseOfferUuid;";

    private final YdbClient ydbClient;
    private final ObjectMapper objectMapper;
    private final Duration timeout;
    private final Instrument findMethodInstrument;
    private final Instrument storeMethodInstrument;
    private final Instrument deleteMethodInstrument;
    private final Function<ResultSetReader, WebhookResponse> webhookResponseRowMapper;

    PurchaseCompleteResponseDaoImpl(
            YdbClient ydbClient,
            ObjectMapper objectMapper,
            MetricRegistry metricRegistry,
            Duration timeout
    ) {
        this.ydbClient = ydbClient;
        this.objectMapper = objectMapper;
        this.timeout = timeout;

        NamedSensorsRegistry registry = new NamedSensorsRegistry(metricRegistry, "ydb")
                .withLabels(Labels.of("target", "purchase-complete-skill-response"));

        this.findMethodInstrument = registry.instrument("findPurchaseCompleteResponse");
        this.storeMethodInstrument = registry.instrument("storePurchaseCompleteResponse");
        this.deleteMethodInstrument = registry.instrument("deletePurchaseCompleteResponse");
        this.webhookResponseRowMapper = new WebhookResponseRowMapper();
    }

    void prepareQueries() {
        ydbClient.warmUpSessionPool(List.of(
                SELECT_QUERY,
                STORE_QUERY,
                DELETE_QUERY
        ));
    }

    public Optional<WebhookResponse> findResponse(PurchaseCompleteResponseKey key) {
        return findMethodInstrument.time(() -> {
            Params params = Params.of(
                    "$userId", PrimitiveValue.string(key.getUserId().getBytes()),
                    "$skillId", PrimitiveValue.string(key.getSkillId().getBytes()),
                    "$purchaseOfferUuid", PrimitiveValue.string(key.getPurchaseOfferUuid().getBytes())
            );

            logger.info("searching purchase complete response");
            var txControl = TxControl.onlineRo();
            var dataQueryResult = ydbClient.execute(
                    "purchase_complete_skill_response.find",
                    timeout,
                    session -> session.executeDataQuery(SELECT_QUERY, txControl, params, ydbClient.keepInQueryCache())
            );

            var responses = ydbClient.readResultSetByIndex(dataQueryResult, 0, webhookResponseRowMapper);

            if (responses.isEmpty()) {
                return Optional.empty();
            } else if (responses.size() > 1) {
                throw new PurchaseCompleteResponseException("Only one response should be store for one key = " + key);
            }
            return Optional.of(responses.get(0));
        });
    }

    public void storeResponse(PurchaseCompleteResponseKey key, Instant timestamp, WebhookResponse response) {
        storeMethodInstrument.time(() -> {
            Params params = Params.of(
                    "$userId", PrimitiveValue.string(key.getUserId().getBytes()),
                    "$skillId", PrimitiveValue.string(key.getSkillId().getBytes()),
                    "$purchaseOfferUuid", PrimitiveValue.string(key.getPurchaseOfferUuid().getBytes()),
                    "$response", PrimitiveValue.json(convertToString(response)),
                    "$timestamp", PrimitiveValue.timestamp(timestamp)
            );

            var txControl = TxControl.serializableRw().setCommitTx(true);
            return ydbClient.execute(
                    "purchase_complete_skill_response.store",
                    timeout,
                    session -> session.executeDataQuery(STORE_QUERY, txControl, params, ydbClient.keepInQueryCache())
            );
        });
    }

    public void deleteResponse(PurchaseCompleteResponseKey key) {
        deleteMethodInstrument.time(() -> {
            Params params = Params.of(
                    "$userId", PrimitiveValue.string(key.getUserId().getBytes()),
                    "$skillId", PrimitiveValue.string(key.getSkillId().getBytes()),
                    "$purchaseOfferUuid", PrimitiveValue.string(key.getPurchaseOfferUuid().getBytes())
            );

            var txControl = TxControl.serializableRw().setCommitTx(true);
            return ydbClient.execute(
                    "purchase_complete_skill_response.delete",
                    timeout,
                    session -> session.executeDataQuery(DELETE_QUERY, txControl, params, ydbClient.keepInQueryCache())
            );
        });
    }

    private String convertToString(WebhookResponse response) {
        try {
            return objectMapper.writeValueAsString(response);
        } catch (JsonProcessingException e) {
            logger.error("Failed to serialize response to String", e);
            throw new RuntimeException(e);
        }
    }

    private class WebhookResponseRowMapper implements Function<ResultSetReader, WebhookResponse> {
        @Override
        public WebhookResponse apply(ResultSetReader reader) {
            try {
                return objectMapper.readValue(reader.getColumn("response").getJson(), WebhookResponse.class);
            } catch (JsonProcessingException e) {
                logger.error("Failed to deserialize response from ydb", e);
                throw new RuntimeException(e);
            }
        }
    }
}
