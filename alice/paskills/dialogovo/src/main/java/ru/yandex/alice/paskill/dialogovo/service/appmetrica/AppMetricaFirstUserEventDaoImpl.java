package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

import com.yandex.ydb.core.Result;
import com.yandex.ydb.table.Session;
import com.yandex.ydb.table.query.DataQueryResult;
import com.yandex.ydb.table.query.Params;
import com.yandex.ydb.table.transaction.TxControl;
import com.yandex.ydb.table.values.PrimitiveValue;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient;
import ru.yandex.alice.paskills.common.solomon.utils.Instrument;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

class AppMetricaFirstUserEventDaoImpl implements AppMetricaFirstUserEventDao {
    private static final Logger logger = LogManager.getLogger();

    private static final String FIRST_USER_EVENT_EXISTS_QUERY = "" +
            "--!syntax_v1\n" +
            "DECLARE $appMetricaApiKey AS String;\n" +
            "DECLARE $skillId AS String;\n" +
            "DECLARE $uuid AS String;\n" +
            "\n" +
            "$skillIdUuidHash = Digest::CityHash(YQL::Concat($skillId, $uuid));\n" +
            "\n" +
            "SELECT 1 FROM skill_app_metrica_first_user_event\n" +
            "WHERE skill_id_uuid_hash = $skillIdUuidHash AND\n" +
            "app_metrica_api_key = $appMetricaApiKey AND\n" +
            "skill_id = $skillId AND\n" +
            "`uuid` = $uuid";

    private static final String SAVE_FIRST_USER_EVENT_QUERY = "" +
            "--!syntax_v1\n" +
            "DECLARE $appMetricaApiKey AS String;\n" +
            "DECLARE $skillId AS String;\n" +
            "DECLARE $uuid AS String;\n" +
            "DECLARE $timestamp AS UInt64;\n" +
            "\n" +
            "$skillIdUuidHash = Digest::CityHash(YQL::Concat($skillId, $uuid));\n" +
            "\n" +
            "REPLACE INTO skill_app_metrica_first_user_event (skill_id_uuid_hash, app_metrica_api_key, skill_id, " +
            "`uuid`, `timestamp`)\n" +
            "VALUES($skillIdUuidHash, $appMetricaApiKey, $skillId, $uuid, $timestamp);";
    private static final Duration TIMEOUT = Duration.ofMillis(500);

    private final Instrument saveFirstUserEventQueryInstrument;
    private final Instrument firstUserEventExistsQueryInstrument;
    private final YdbClient ydbClient;

    AppMetricaFirstUserEventDaoImpl(
            MetricRegistry metricRegistry,
            YdbClient ydbClient) {
        this.ydbClient = ydbClient;
        NamedSensorsRegistry registry = new NamedSensorsRegistry(metricRegistry, "ydb")
                .withLabels(Labels.of("target", "skill-appmetrica-user-first-event"));
        this.saveFirstUserEventQueryInstrument = registry.instrument("appMetricaSaveFirstUserEvent");
        this.firstUserEventExistsQueryInstrument = registry.instrument("appMetricaFirstUserEventExists");
    }

    void prepareQueries() {
        logger.info("warming up sessions");
        List<String> queries = new ArrayList<>();

        queries.add(FIRST_USER_EVENT_EXISTS_QUERY);
        queries.add(SAVE_FIRST_USER_EVENT_QUERY);

        ydbClient.warmUpSessionPool(queries);
        logger.info("warm up completed");
    }

    @Override
    public void saveFirstUserEvent(
            String appMetricaApiKey,
            String skillId,
            String uuid,
            Instant timestamp) {
        Params params = Params.of(
                "$appMetricaApiKey", PrimitiveValue.string(appMetricaApiKey.getBytes()),
                "$skillId", PrimitiveValue.string(skillId.getBytes()),
                "$uuid", PrimitiveValue.string(uuid.getBytes()),
                "$timestamp", PrimitiveValue.uint64(timestamp.toEpochMilli())
        );

        saveFirstUserEventQueryInstrument.time(() ->
                ydbClient.execute("app_metrica.save-first-user-event", TIMEOUT,
                        session -> session.executeDataQuery(
                                SAVE_FIRST_USER_EVENT_QUERY,
                                TxControl.serializableRw(),
                                params,
                                ydbClient.keepInQueryCache())
                                .orTimeout(2, TimeUnit.SECONDS)
                )
        );
    }

    @Override
    public boolean firstUserEventExists(String appMetricaApiKey, String skillId, String uuid) {
        return !firstUserEventExistsQueryInstrument.time(
                () -> ydbClient.readFirstResultSet(
                        "app_metrica.first-user-event-exists", TIMEOUT,
                        session -> query(appMetricaApiKey, skillId, uuid, session),
                        resultSet -> true
                )
        ).isEmpty();
    }

    private CompletableFuture<Result<DataQueryResult>> query(String appMetricaApiKey, String skillId, String uuid,
                                                             Session session) {
        Params params = Params.of(
                "$appMetricaApiKey", PrimitiveValue.string(appMetricaApiKey.getBytes()),
                "$skillId", PrimitiveValue.string(skillId.getBytes()),
                "$uuid", PrimitiveValue.string(uuid.getBytes()));
        return session.executeDataQuery(
                FIRST_USER_EVENT_EXISTS_QUERY,
                TxControl.onlineRo(),
                params,
                ydbClient.keepInQueryCache());
    }
}
