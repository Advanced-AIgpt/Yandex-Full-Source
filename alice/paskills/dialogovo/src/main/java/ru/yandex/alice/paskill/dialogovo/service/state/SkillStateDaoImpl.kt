package ru.yandex.alice.paskill.dialogovo.service.state

import com.fasterxml.jackson.core.JsonProcessingException
import com.fasterxml.jackson.core.type.TypeReference
import com.fasterxml.jackson.databind.ObjectMapper
import com.google.common.base.Strings
import com.yandex.ydb.core.Result
import com.yandex.ydb.core.Status
import com.yandex.ydb.table.Session
import com.yandex.ydb.table.query.DataQueryResult
import com.yandex.ydb.table.query.Params
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.transaction.TransactionMode
import com.yandex.ydb.table.transaction.TxControl
import com.yandex.ydb.table.values.PrimitiveValue
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.paskill.dialogovo.service.state.SkillState.Companion.create
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient
import ru.yandex.alice.paskills.common.solomon.utils.Instrument
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.nio.charset.StandardCharsets
import java.time.Duration
import java.time.Instant
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.function.Function

private const val SELECT_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}userId AS String;
    DECLARE ${"$"}sessionId AS String;
    DECLARE ${"$"}applicationId AS String;

    ${"$"}hashedUserId = Digest::CityHash(${"$"}userId);
    ${"$"}hashedSessionId = Digest::CityHash(${"$"}sessionId);
    ${"$"}hashedApplicationId = Digest::CityHash(${"$"}applicationId);

    SELECT skill_id, user_id, is_anonymous, changed_at, state
    FROM skill_user_state
    WHERE user_id_hash == ${"$"}hashedUserId
    AND user_id == ${"$"}userId
    AND skill_id == ${"$"}skillId;

    SELECT skill_id, session_id, changed_at, state
    FROM skill_session_state
    WHERE session_id_hash == ${"$"}hashedSessionId
    AND session_id == ${"$"}sessionId
    AND skill_id == ${"$"}skillId;

    SELECT skill_id, application_id, changed_at, state
    FROM skill_application_state
    WHERE application_id_hash == ${"$"}hashedApplicationId
    AND application_id == ${"$"}applicationId
    AND skill_id == ${"$"}skillId;
"""

private const val SELECT_SESS_AND_APP_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}sessionId AS String;
    DECLARE ${"$"}applicationId AS String;

    ${"$"}hashedSessionId = Digest::CityHash(${"$"}sessionId);
    ${"$"}hashedApplicationId = Digest::CityHash(${"$"}applicationId);

    SELECT skill_id, session_id, changed_at, state
    FROM skill_session_state
    WHERE session_id_hash == ${"$"}hashedSessionId
    AND session_id == ${"$"}sessionId
    AND skill_id == ${"$"}skillId;

    SELECT skill_id, application_id, changed_at, state
    FROM skill_application_state
    WHERE application_id_hash == ${"$"}hashedApplicationId
    AND application_id == ${"$"}applicationId
    AND skill_id == ${"$"}skillId;
"""

private const val REPLACE_SESSION_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}sessionId AS String;
    DECLARE ${"$"}changedAt AS Timestamp;
    DECLARE ${"$"}state AS Json;

    ${"$"}hashedSessionId = Digest::CityHash(${"$"}sessionId);

    REPLACE INTO skill_session_state (session_id_hash, session_id, skill_id, changed_at, state)
    VALUES(${"$"}hashedSessionId, ${"$"}sessionId, ${"$"}skillId, ${"$"}changedAt, ${"$"}state);
"""

private const val DELETE_SESSION_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}sessionId AS String;

    ${"$"}hashedSessionId = Digest::CityHash(${"$"}sessionId);

    DELETE FROM skill_session_state
    WHERE 1=1
    AND session_id_hash = ${"$"}hashedSessionId
    AND session_id = ${"$"}sessionId
    AND skill_id = ${"$"}skillId;
"""

private const val SELECT_USER_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}userId AS String;

    ${"$"}hashedUserId = Digest::CityHash(${"$"}userId);

    SELECT skill_id, user_id, changed_at, state
    FROM skill_user_state
    WHERE user_id_hash == ${"$"}hashedUserId
    AND user_id == ${"$"}userId
    AND skill_id == ${"$"}skillId;
"""
private const val SELECT_SESSION_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}sessionId AS String;

    ${"$"}hashedSessionId = Digest::CityHash(${"$"}sessionId);

    SELECT skill_id, session_id, changed_at, state
    FROM skill_session_state
    WHERE session_id_hash == ${"$"}hashedSessionId
    AND session_id == ${"$"}sessionId
    AND skill_id == ${"$"}skillId;
"""
private const val REPLACE_USER_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}userId AS String;
    DECLARE ${"$"}changedAt AS Timestamp;
    DECLARE ${"$"}state AS Json;

    ${"$"}hashedUserId = Digest::CityHash(${"$"}userId);

    REPLACE INTO skill_user_state (user_id_hash, user_id, skill_id, changed_at, state)
    VALUES(${"$"}hashedUserId, ${"$"}userId, ${"$"}skillId, ${"$"}changedAt, ${"$"}state);
"""

private const val SELECT_APPLICATION_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}applicationId AS String;

    ${"$"}hashedApplicationId = Digest::CityHash(${"$"}applicationId);

    SELECT skill_id, application_id, changed_at, state
    FROM skill_application_state
    WHERE application_id_hash == ${"$"}hashedApplicationId
    AND application_id == ${"$"}applicationId
    AND skill_id == ${"$"}skillId;
"""

private const val REPLACE_APPLICATION_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}applicationId AS String;
    DECLARE ${"$"}changedAt AS Timestamp;
    DECLARE ${"$"}state AS Json;

    ${"$"}hashedApplicationId = Digest::CityHash(${"$"}applicationId);

    REPLACE INTO skill_application_state (application_id_hash, application_id, skill_id, changed_at, state)
    VALUES(${"$"}hashedApplicationId, ${"$"}applicationId, ${"$"}skillId, ${"$"}changedAt, ${"$"}state);
"""

private const val DELETE_APPLICATION_STATE_QUERY = """--!syntax_v1
    DECLARE ${"$"}skillId AS String;
    DECLARE ${"$"}applicationId AS String;

    ${"$"}hashedApplicationId = Digest::CityHash(${"$"}applicationId);

    DELETE FROM skill_application_state
    WHERE application_id_hash == ${"$"}hashedApplicationId
    AND application_id == ${"$"}applicationId
    AND skill_id == ${"$"}skillId;
"""

internal class SkillStateDaoImpl(
    private val ydbClient: YdbClient,
    private val objectMapper: ObjectMapper,
    private val executorService: DialogovoInstrumentedExecutorService,
    metricRegistry: MetricRegistry
) : SkillStateDao {
    private val typeRef: TypeReference<Map<String, Any>> = object : TypeReference<Map<String, Any>>() {}
    private val userStateRowMapper: UserStateRowMapper
    private val sessionStateRowMapper: SessionStateRowMapper
    private val applicationStateRowMapper: ApplicationStateRowMapper
    private val findMethodInstrument: Instrument
    private val replaceSessionStatInstrument: Instrument
    private val deleteSessionStateInstrument: Instrument
    private val selectUserStateInstrument: Instrument
    private val replaceUserStateInstrument: Instrument
    private val selectSessionStateInstrument: Instrument
    private val selectApplicationStateInstrument: Instrument
    private val replaceApplicationStateInstrument: Instrument
    private val storeSessionAndUserState: Instrument
    private val storeApplicationState: Instrument

    // warmup reduces first call in a session timings by 30:50 ms
    fun prepareQueries() {
        val queries = listOf(
            SELECT_QUERY,
            SELECT_SESS_AND_APP_STATE_QUERY,
            DELETE_SESSION_STATE_QUERY,
            SELECT_USER_STATE_QUERY,
            SELECT_SESSION_STATE_QUERY,
            REPLACE_USER_STATE_QUERY,
            REPLACE_SESSION_STATE_QUERY,
            SELECT_APPLICATION_STATE_QUERY,
            REPLACE_APPLICATION_STATE_QUERY,
            DELETE_APPLICATION_STATE_QUERY
        )
        ydbClient.warmUpSessionPool(queries)
    }

    override fun findBySkillIdAndUserIdAndSessionIdAndApplicationId(skillStateId: SkillStateId): SkillState {

        return findMethodInstrument.time<SkillState> {

            val stateEntities = if (skillStateId.userId != null) {
                fetchAllStates(skillStateId)
            } else {
                fetchSessionAndAppState(skillStateId)
            }

            return@time stateEntities.run {
                logger.debug(
                    "Found {} session, {} user, {} app states",
                    sessionStates.size, userStates.size, applicationStates.size
                )

                if (userStates.isEmpty() && sessionStates.isEmpty() && applicationStates.isEmpty()) {
                    return@run SkillState.EMPTY
                } else if (userStates.size > 1 || sessionStates.size > 1 || applicationStates.size > 1) {
                    // should never happen as we select by primary key
                    throw IncorrectResultSizeDataAccessException()
                } else {
                    return@run create(
                        userStates.firstOrNull(),
                        sessionStates.firstOrNull(),
                        applicationStates.firstOrNull()
                    )
                }
            }
        }
    }

    private fun fetchSessionAndAppState(skillStateId: SkillStateId): StateEntities {
        val tx = TxControl.onlineRo()
        val params = Params.of(
            "\$skillId", PrimitiveValue.string(skillStateId.skillId.toByteArray()),
            "\$sessionId", PrimitiveValue.string(skillStateId.sessionId.toByteArray()),
            "\$applicationId", PrimitiveValue.string(skillStateId.applicationId.toByteArray())
        )

        logger.info("Selecting current session and application state")
        val dataQueryResult = ydbClient.execute("state.select-session-and-app-state") { session ->
            session.executeDataQuery(SELECT_SESS_AND_APP_STATE_QUERY, tx, params, ydbClient.keepInQueryCache())
        }

        return StateEntities(
            userStates = emptyList(),
            sessionStates = ydbClient.readResultSetByIndex(dataQueryResult, 0, sessionStateRowMapper),
            applicationStates = ydbClient.readResultSetByIndex(dataQueryResult, 1, applicationStateRowMapper),
        )
    }

    private fun fetchAllStates(skillStateId: SkillStateId): StateEntities {
        val tx = TxControl.onlineRo()
        val params = Params.of(
            "\$skillId", PrimitiveValue.string(skillStateId.skillId.toByteArray()),
            "\$userId", PrimitiveValue.string(skillStateId.userId!!.toByteArray()),
            "\$sessionId", PrimitiveValue.string(skillStateId.sessionId.toByteArray()),
            "\$applicationId", PrimitiveValue.string(skillStateId.applicationId.toByteArray())
        )

        logger.info("searching states by skill and user ids")
        val dataQueryResult = ydbClient.execute("state.find-by-skill-userId-sessionId", TIMEOUT) { session ->
            session.executeDataQuery(
                SELECT_QUERY,
                tx,
                params,
                ydbClient.keepInQueryCache()
            )
        }

        return StateEntities(
            userStates = ydbClient.readResultSetByIndex(dataQueryResult, 0, userStateRowMapper),
            sessionStates = ydbClient.readResultSetByIndex(dataQueryResult, 1, sessionStateRowMapper),
            applicationStates = ydbClient.readResultSetByIndex(dataQueryResult, 2, applicationStateRowMapper),
        )
    }

    private data class StateEntities(
        val userStates: List<UserStateEntity>,
        val sessionStates: List<SessionStateEntity>,
        val applicationStates: List<ApplicationStateEntity>,
    )

    override fun findBySkillIdAndUserIdAndSessionIdAndApplicationIdAsync(
        skillStateId: SkillStateId
    ): CompletableFuture<SkillState> {
        return executorService.supplyAsyncInstrumented(
            { findBySkillIdAndUserIdAndSessionIdAndApplicationId(skillStateId) },
            Duration.ofSeconds(1)
        )
    }

    override fun storeSessionAndUserAndApplicationState(
        skillStateId: SkillStateId,
        requestTime: Instant,
        sessionState: Map<String, Any>?,
        userStateIncrement: Map<String, Any?>,
        applicationState: Map<String, Any>?
    ) {
        val userId = skillStateId.userId
        val applicationId = skillStateId.applicationId
        val sessionId = skillStateId.sessionId
        val skillId = skillStateId.skillId
        logger.debug(
            "storing session and user state. sessionState: {}, userStateIncrement: {}, " +
                "applicationState: {}",
            sessionState, userStateIncrement, applicationState
        )

        val applicationStateFuture = applicationState?.let { state ->
            val txControl = TxControl.serializableRw()
            storeApplicationState.timeFuture {
                ydbClient.executeAsync("state.store-application-state") { session: Session ->
                    storeApplicationState(session, txControl, skillId, applicationId, requestTime, state)
                        .whenComplete { _, _: Throwable? -> logger.info("Application state stored") }
                }.orTimeout(TIMEOUT.toMillis(), TimeUnit.MILLISECONDS)
            }
        } ?: CompletableFuture.completedFuture(null)

        val updateUserStateFuture = if (userStateIncrement.isNotEmpty() && userId != null) {
            storeSessionAndUserState.timeFuture {
                ydbClient.executeAsync(
                    "state.store-user-state"
                ) { session ->
                    storeUserState(session, skillId, userId, userStateIncrement, requestTime)
                }.orTimeout(TIMEOUT.toMillis(), TimeUnit.MILLISECONDS)
            }
        } else {
            CompletableFuture.completedFuture(Status.SUCCESS)
        }

        // commit right away if no user state changes required
        val sessionStoreFuture: CompletableFuture<Status> = if (sessionState != null) {
            storeSessionAndUserState.timeFuture {
                val txControl = TxControl.serializableRw()
                ydbClient.executeAsync(
                    "state.store-session-user-state"
                ) { session ->
                    storeSessionState(session, txControl, sessionId, skillId, sessionState, requestTime)
                        .whenComplete { _, _: Throwable? -> logger.info("Session state stored") }
                }.orTimeout(TIMEOUT.toMillis(), TimeUnit.MILLISECONDS)
            }
        } else {
            CompletableFuture.completedFuture(Status.SUCCESS)
        }

        CompletableFuture.allOf(applicationStateFuture, updateUserStateFuture, sessionStoreFuture)
            .handle { _, ex -> logger.error("Skill state storage failed", ex) }
            .join()
    }

    private fun storeUserState(
        session: Session,
        skillId: String,
        userId: String?,
        userStateIncrement: Map<String, Any?>,
        requestTime: Instant
    ): CompletableFuture<Result<Status>> {
        logger.debug("Selecting current user state")
        return session.beginTransaction(TransactionMode.SERIALIZABLE_READ_WRITE)
            .thenCompose { res ->
                if (res.isSuccess) {
                    val txId = res.expect("").id
                    selectUserState(session, TxControl.id(txId).setCommitTx(false), skillId, userId)
                        .thenCompose { r: Result<Map<String, Any>?> ->
                            if (r.isSuccess) {
                                val currentState = r.expect("")
                                logger.debug("Current user state: {}", currentState)
                                val newState = updateStateMap(currentState, userStateIncrement)
                                logger.debug("Storing new user state: {}", newState)
                                replaceUserState(
                                    session,
                                    TxControl.id(txId).setCommitTx(true),
                                    skillId,
                                    userId,
                                    requestTime,
                                    newState
                                )
                                    .whenComplete { _, _: Throwable? -> logger.info("User state stored") }
                                    .thenApply { r1 -> r1.map { r1.toStatus() } }
                            } else {
                                CompletableFuture.completedStage(r.map { r.toStatus() })
                            }
                        }
                } else {
                    CompletableFuture.completedStage(res.map { res.toStatus() })
                }
            }
    }

    private fun selectUserState(
        session: Session,
        tx: TxControl<*>,
        skillId: String,
        userId: String?
    ): CompletableFuture<Result<Map<String, Any>?>> {
        return selectUserStateInstrument.time<CompletableFuture<Result<Map<String, Any>?>>> {
            val params = Params.of(
                "\$skillId", PrimitiveValue.string(skillId.toByteArray()),
                "\$userId", PrimitiveValue.string(userId!!.toByteArray())
            )

            logger.info("Selecting current user state")
            session.executeDataQuery(SELECT_USER_STATE_QUERY, tx, params, ydbClient.keepInQueryCache())
                .thenApply { result: Result<DataQueryResult> ->
                    result.map { queryResult: DataQueryResult ->
                        ydbClient.readFirstResultSet(queryResult, userStateRowMapper)
                    }.map { userStates -> userStates.map { it.state }.firstOrNull() ?: emptyMap() }
                }
                .whenComplete { _, _ -> logger.debug("selected current user state") }
        }
    }

    private fun replaceUserState(
        session: Session,
        tx: TxControl<*>,
        skillId: String,
        userId: String?,
        timestamp: Instant,
        state: Map<String, Any>
    ): CompletableFuture<Result<DataQueryResult>> {
        return replaceUserStateInstrument.time<CompletableFuture<Result<DataQueryResult>>> {
            val newStateStr = toString(state)
            val params = Params.of(
                "\$skillId", PrimitiveValue.string(skillId.toByteArray()),
                "\$userId", PrimitiveValue.string(userId!!.toByteArray()),
                "\$changedAt", PrimitiveValue.timestamp(timestamp),
                "\$state", PrimitiveValue.json(newStateStr)
            )
            session.executeDataQuery(REPLACE_USER_STATE_QUERY, tx, params, ydbClient.keepInQueryCache())
        }
    }

    private fun storeApplicationState(
        ydbSession: Session,
        tx: TxControl<*>,
        skillId: String,
        applicationId: String,
        timestamp: Instant,
        applicationState: Map<String, Any>
    ): CompletableFuture<Result<DataQueryResult>> {
        val future: CompletableFuture<Result<DataQueryResult>> = if (applicationState.isNotEmpty()) {
            replaceApplicationState(ydbSession, tx, skillId, applicationId, applicationState, timestamp)
        } else {
            deleteApplicationState(ydbSession, tx, skillId, applicationId)
        }
        return future.whenComplete { _, ex: Throwable? ->
            if (ex == null) {
                logger.info("Application state stored")
            }
        }
    }

    private fun replaceApplicationState(
        ydbSession: Session,
        txControl: TxControl<*>,
        skillId: String,
        applicationId: String,
        applicationState: Map<String, Any>,
        timestamp: Instant
    ): CompletableFuture<Result<DataQueryResult>> {
        return replaceApplicationStateInstrument.time<CompletableFuture<Result<DataQueryResult>>> {
            val newStateStr = toString(applicationState)
            val params = Params.of(
                "\$skillId", PrimitiveValue.string(skillId.toByteArray()),
                "\$applicationId", PrimitiveValue.string(applicationId.toByteArray()),
                "\$changedAt", PrimitiveValue.timestamp(timestamp),
                "\$state", PrimitiveValue.json(newStateStr)
            )
            ydbSession.executeDataQuery(
                REPLACE_APPLICATION_STATE_QUERY, txControl, params, ydbClient.keepInQueryCache()
            )
        }
    }

    private fun deleteApplicationState(
        ydbSession: Session,
        txControl: TxControl<*>,
        skillId: String,
        applicationId: String
    ): CompletableFuture<Result<DataQueryResult>> {
        return deleteSessionStateInstrument.time<CompletableFuture<Result<DataQueryResult>>> {
            val params = Params.of(
                "\$skillId", PrimitiveValue.string(skillId.toByteArray()),
                "\$applicationId", PrimitiveValue.string(applicationId.toByteArray())
            )
            ydbSession.executeDataQuery(
                DELETE_APPLICATION_STATE_QUERY, txControl, params, ydbClient.keepInQueryCache()
            )
        }
    }

    private fun updateStateMap(currentState: Map<String, Any>?, stateIncrement: Map<String, Any?>): Map<String, Any> {
        // LHM used to preserve order and allow null values
        val newState = LinkedHashMap(currentState)
        for ((key1, value1) in stateIncrement) {
            newState.compute(key1) { _, _ -> value1 }
        }
        return newState
    }

    private fun storeSessionState(
        ydbSession: Session,
        txControl: TxControl<*>,
        sessionId: String,
        skillId: String,
        sessionState: Map<String, Any>,
        stateTime: Instant
    ): CompletableFuture<Result<Status>> {
        val future: CompletableFuture<Result<Status>> = if (sessionState.isEmpty()) {
            deleteSessionState(ydbSession, txControl, sessionId, skillId)
        } else {
            replaceSessionState(ydbSession, txControl, sessionId, skillId, sessionState, stateTime)
        }
        return future.whenComplete { _, ex: Throwable? ->
            if (ex == null) {
                logger.info("Session state stored")
            }
        }
    }

    private fun replaceSessionState(
        ydbSession: Session,
        txControl: TxControl<*>,
        sessionId: String,
        skillId: String,
        sessionState: Map<String, Any>,
        stateTime: Instant
    ): CompletableFuture<Result<Status>> {
        return replaceSessionStatInstrument.timeFuture {
            val newStateStr = toString(sessionState)
            val params = Params.of(
                "\$skillId", PrimitiveValue.string(skillId.toByteArray()),
                "\$sessionId", PrimitiveValue.string(sessionId.toByteArray()),
                "\$changedAt", PrimitiveValue.timestamp(stateTime),
                "\$state", PrimitiveValue.json(newStateStr)
            )
            ydbSession.executeDataQuery(
                REPLACE_SESSION_STATE_QUERY, txControl, params, ydbClient.keepInQueryCache()
            ).thenApply { it.map { res -> it.toStatus() } }
        }
    }

    private fun deleteSessionState(
        ydbSession: Session,
        txControl: TxControl<*>,
        sessionId: String,
        skillId: String
    ): CompletableFuture<Result<Status>> {
        return deleteSessionStateInstrument.timeFuture {
            val params = Params.of(
                "\$skillId", PrimitiveValue.string(skillId.toByteArray()),
                "\$sessionId", PrimitiveValue.string(sessionId.toByteArray())
            )
            ydbSession.executeDataQuery(
                DELETE_SESSION_STATE_QUERY, txControl, params, ydbClient.keepInQueryCache()
            ).thenApply { it.map { res -> it.toStatus() } }
        }
    }

    private fun toString(state: Map<String, Any>): String {
        return try {
            objectMapper.writeValueAsString(state)
        } catch (e: JsonProcessingException) {
            throw RuntimeException(e)
        }
    }

    private inner class UserStateRowMapper : Function<ResultSetReader, UserStateEntity> {
        override fun apply(reader: ResultSetReader): UserStateEntity {
            return UserStateEntity(
                reader.getColumn("skill_id").getString(StandardCharsets.UTF_8),
                reader.getColumn("user_id").getString(StandardCharsets.UTF_8),
                reader.getColumn("changed_at").takeIf { it.isOptionalItemPresent }?.timestamp,
                jsonToMap(reader.getColumn("state").json)
            )
        }
    }

    private inner class SessionStateRowMapper : Function<ResultSetReader, SessionStateEntity> {
        override fun apply(reader: ResultSetReader): SessionStateEntity {
            return SessionStateEntity(
                reader.getColumn("skill_id").getString(StandardCharsets.UTF_8),
                reader.getColumn("session_id").getString(StandardCharsets.UTF_8),
                reader.getColumn("changed_at").takeIf { it.isOptionalItemPresent }?.timestamp,
                jsonToMap(reader.getColumn("state").json)
            )
        }
    }

    private inner class ApplicationStateRowMapper : Function<ResultSetReader, ApplicationStateEntity> {
        override fun apply(reader: ResultSetReader): ApplicationStateEntity {
            return ApplicationStateEntity(
                reader.getColumn("skill_id").getString(StandardCharsets.UTF_8),
                reader.getColumn("application_id").getString(StandardCharsets.UTF_8),
                reader.getColumn("changed_at").takeIf { it.isOptionalItemPresent }?.timestamp,
                jsonToMap(reader.getColumn("state").json)
            )
        }
    }

    private fun jsonToMap(stateJson: String): Map<String, Any> {
        return try {
            if (!Strings.isNullOrEmpty(stateJson)) objectMapper.readValue(stateJson, typeRef) else emptyMap()
        } catch (e: JsonProcessingException) {
            logger.error("Failed to deserialize state", e)
            emptyMap()
        }
    }

    companion object {
        private val logger = LogManager.getLogger()

        private val EMPTY_BYTES = ByteArray(0)
        private val TIMEOUT = Duration.ofMillis(500)
    }

    init {
        val registry = NamedSensorsRegistry(metricRegistry, "ydb")
            .withLabels(Labels.of("target", "skill-state"))
        findMethodInstrument = registry.instrument("findByUserIdAndSkillIdAndSessionId")
        replaceSessionStatInstrument = registry.instrument("replaceSessionStat")
        deleteSessionStateInstrument = registry.instrument("deleteSessionState")
        selectUserStateInstrument = registry.instrument("selectUserState")
        selectSessionStateInstrument = registry.instrument("selectSessionState")
        replaceUserStateInstrument = registry.instrument("replaceUserState")
        storeSessionAndUserState = registry.instrument("storeSessionAndUserState")
        selectApplicationStateInstrument = registry.instrument("selectApplicationState")
        replaceApplicationStateInstrument = registry.instrument("replaceApplicationState")
        storeApplicationState = registry.instrument("storeApplicationState")
        userStateRowMapper = UserStateRowMapper()
        sessionStateRowMapper = SessionStateRowMapper()
        applicationStateRowMapper = ApplicationStateRowMapper()
    }
}
