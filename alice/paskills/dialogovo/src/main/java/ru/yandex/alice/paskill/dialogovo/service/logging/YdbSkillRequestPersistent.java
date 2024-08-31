package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.time.Duration;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;

import com.google.common.collect.ImmutableMap;
import com.yandex.ydb.core.Result;
import com.yandex.ydb.table.Session;
import com.yandex.ydb.table.query.DataQueryResult;
import com.yandex.ydb.table.query.Params;
import com.yandex.ydb.table.transaction.TxControl;
import com.yandex.ydb.table.values.OptionalType;
import com.yandex.ydb.table.values.PrimitiveType;
import com.yandex.ydb.table.values.PrimitiveValue;
import com.yandex.ydb.table.values.Value;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient;
import ru.yandex.alice.paskills.common.solomon.utils.Instrument;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

class YdbSkillRequestPersistent implements SkillRequestLogPersistent {

    private static final Duration TIMEOUT = Duration.ofMillis(500);
    private final Logger logger = LogManager.getLogger();

    private static final String SAVE_LOG_RECORD = "" +
            "--!syntax_v1\n" +
            "DECLARE $skillId AS String;\n" +
            "DECLARE $sessionId AS String;\n" +
            "DECLARE $messageId AS Uint64;\n" +
            "DECLARE $eventId AS String?;\n" +
            "DECLARE $reqId AS String;\n" +
            "DECLARE $timestamp AS Uint64;\n" +
            "DECLARE $changedAt AS Timestamp;\n" +
            "DECLARE $status AS String;\n" +
            "DECLARE $requestBody AS Json?;\n" +
            "DECLARE $responseBody AS Json?;\n" +
            "DECLARE $validationErrors AS Utf8;\n" +
            "DECLARE $callDuration AS Uint64;\n" +
            "DECLARE $requestId AS String?;\n" +
            "DECLARE $deviceId AS String?;\n" +
            "DECLARE $uuid AS String?;\n" +
            "\n" +
            "$shardKey = Digest::CityHash($skillId);\n" +
            "\n" +
            "REPLACE INTO skill_request_log \n" +
            "(shard_key, session_id, message_id, event_id, req_id, skill_id, status,\n" +
            "`timestamp`, changed_at, request_body, response_body, validation_errors, callDuration, " +
            "mm_request_id, device_id, `uuid`)\n" +
            "VALUES\n" +
            "($shardKey, $sessionId, $messageId, $eventId, $reqId, $skillId, $status,\n" +
            "$timestamp, $changedAt, $requestBody, $responseBody, $validationErrors, $callDuration, " +
            "$requestId, $deviceId, $uuid);";

    private final DialogovoInstrumentedExecutorService executorService;
    private final Instrument saveRecordInstrument;
    private final YdbClient ydbClient;

    YdbSkillRequestPersistent(
            YdbClient ydbClient,
            DialogovoInstrumentedExecutorService executorService,
            MetricRegistry metricRegistry
    ) {
        this.ydbClient = ydbClient;
        this.executorService = executorService;
        NamedSensorsRegistry registry = new NamedSensorsRegistry(metricRegistry, "ydb")
                .withLabels(Labels.of("target", "skill-request-log"));
        this.saveRecordInstrument = registry.instrument("saveRecord");
    }

    void prepareQueries() {
        ydbClient.warmUpSessionPool(List.of(SAVE_LOG_RECORD));
    }

    @Override
    public CompletableFuture<Void> save(LogRecord record) {
        return executorService.runAsyncInstrumented(() -> saveImpl(record), Duration.ofSeconds(25));
    }

    private void saveImpl(LogRecord record) {
        saveRecordInstrument.time(() -> {
            logger.debug("skill request log record storage init");
            return ydbClient.execute("skill-request-log.save", TIMEOUT, session -> query(session, record));
        });
    }

    private CompletableFuture<Result<DataQueryResult>> query(Session session, LogRecord record) {
        var oJsonType = OptionalType.of(PrimitiveType.json());
        var oStringType = OptionalType.of(PrimitiveType.string());

        Params params = Params.copyOf(new ImmutableMap.Builder<String, Value<?>>()
                .put("$reqId", PrimitiveValue.string(UUID.randomUUID().toString().getBytes()))
                .put("$skillId", PrimitiveValue.string(record.getSkillId().getBytes()))
                .put("$sessionId", PrimitiveValue.string(record.getSessionId().getBytes()))
                .put("$messageId", PrimitiveValue.uint64(record.getMessageId()))
                .put("$timestamp", PrimitiveValue.uint64(record.getTimestamp().toEpochMilli()))
                .put("$changedAt", PrimitiveValue.timestamp(record.getTimestamp()))
                .put("$status", PrimitiveValue.string(record.getStatus().getStatusCode().getBytes()))
                // empty string is converted to optional type
                .put("$eventId", record.getEventId() != null ?
                        oStringType.newValue(PrimitiveValue.string(record.getEventId().getBytes())) :
                        oStringType.emptyValue())
                .put("$requestBody", record.getRequestBody() != null ?
                        oJsonType.newValue(PrimitiveValue.json(record.getRequestBody())) :
                        oJsonType.emptyValue())
                .put("$responseBody", record.getResponseBody() != null ?
                        oJsonType.newValue(PrimitiveValue.json(record.getResponseBody())) :
                        oJsonType.emptyValue())
                .put("$validationErrors", PrimitiveValue.utf8(
                        String.join("\n", record.getValidationErrors())))
                .put("$callDuration", PrimitiveValue.uint64(record.getCallDuration()))
                .put("$requestId", record.getRequestId() != null ?
                        oStringType.newValue(PrimitiveValue.string(record.getRequestId().getBytes())) :
                        oStringType.emptyValue())
                .put("$deviceId", record.getDeviceId() != null ?
                        oStringType.newValue(PrimitiveValue.string(record.getDeviceId().getBytes())) :
                        oStringType.emptyValue())
                .put("$uuid", record.getUuid() != null ?
                        oStringType.newValue(PrimitiveValue.string(record.getUuid().getBytes())) :
                        oStringType.emptyValue())
                .build()
        );

        logger.debug("skill request log record storage started");
        return session.executeDataQuery(
                SAVE_LOG_RECORD, TxControl.serializableRw(),
                params, ydbClient.keepInQueryCache())
                .whenComplete((r, e) -> {
                    if (e != null) {
                        logger.error("skill request log record storage failed", e);
                    } else {
                        logger.debug("skill request log record stored");
                    }
                });
    }

}
