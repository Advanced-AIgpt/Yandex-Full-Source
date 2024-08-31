package ru.yandex.alice.paskill.dialogovo.service.state

import com.fasterxml.jackson.databind.ObjectMapper
import com.yandex.ydb.table.Session
import com.yandex.ydb.table.transaction.TxControl
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.function.Executable
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext
import ru.yandex.alice.paskill.dialogovo.service.state.SkillState.Companion.create
import ru.yandex.alice.paskill.dialogovo.utils.BaseYdbTest
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Instant

private const val SESSION_ID = "sess-2"
private const val SKILL_ID = "skill-id"
private const val USER_ID = "1"
private const val APP_ID = "application-id"
private val DEFAULT_STATE_ID = SkillStateId(SKILL_ID, USER_ID, SESSION_ID, APP_ID)
private val DEFAULT_TIMESTAMP = Instant.ofEpochMilli(123123L)

internal class SkillStateDaoImplTest : BaseYdbTest() {
    private lateinit var stateDao: SkillStateDaoImpl
    private lateinit var executorService: DialogovoInstrumentedExecutorService
    private lateinit var requestTime: Instant

    @BeforeEach
    @Throws(Exception::class)
    public override fun setUp() {
        super.setUp()
        requestTime = Instant.now()
        val executorsFactory = ExecutorsFactory(MetricRegistry(), RequestContext(), DialogovoRequestContext())

        executorService = executorsFactory.fixedThreadPool(3, "test")
        stateDao = SkillStateDaoImpl(ydbClient, ObjectMapper(), executorService, MetricRegistry())
    }

    @AfterEach
    @Throws(Exception::class)
    public override fun tearDown() {
        executorService.shutdownNow()
        super.tearDown()
    }

    @Test
    fun testFetchEmptyRecord() {
        Assertions.assertEquals(
            SkillState.EMPTY,
            stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(
                SkillStateId("1", "1", "1", "1")
            )
        )
    }

    @Test
    fun testWarmUpDoesntFail() {
        stateDao.prepareQueries()
    }

    @Test
    fun testFetchUserPlusSessionPlusApplicationState() {
        withSession { session -> prepareState(session) }

        val expected = createSkillState(
            DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 123, "b" to "q"),
            sessionState = mapOf<String, Any>("c" to 123, "d" to "q"),
            applicationState = mapOf<String, Any>("e" to 123, "f" to "q")
        )

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        Assertions.assertEquals(expected, actual)
    }

    @Test
    fun testFetchSessionPlusApplicationState() {
        withSession { session: Session -> prepareState(session) }
        val actual2 = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(
            SkillStateId(SKILL_ID, "$USER_ID-wrong", SESSION_ID, APP_ID)
        )
        val expected2 = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            sessionState = mapOf<String, Any>("c" to 123, "d" to "q"),
            applicationState = mapOf<String, Any>("e" to 123, "f" to "q")
        )
        Assertions.assertEquals(expected2, actual2)
    }

    @Test
    fun testFetchApplicationOnlyState() {
        withSession { session: Session -> prepareState(session) }
        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(
            SkillStateId(SKILL_ID, "$USER_ID-wrong", "$SESSION_ID-wrong", APP_ID)
        )

        val expected =
            createSkillState(
                DEFAULT_TIMESTAMP,
                null,
                null,
                applicationState = mapOf<String, Any>("e" to 123, "f" to "q")
            )
        Assertions.assertEquals(actual, expected)
    }

    @Test
    fun testFetchWrongSkillState() {
        withSession { session: Session -> prepareState(session) }

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(
            SkillStateId("$SKILL_ID-wrong", USER_ID, SESSION_ID, APP_ID)
        )
        Assertions.assertEquals(SkillState.EMPTY, actual)
    }

    @Test
    fun testFetchUserPlusApplicationState() {
        withSession { session: Session -> prepareState(session) }

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(
            SkillStateId(SKILL_ID, USER_ID, "$SESSION_ID-wrong", APP_ID)
        )

        val expected = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 123, "b" to "q"),
            applicationState = mapOf<String, Any>("e" to 123, "f" to "q")
        )
        Assertions.assertEquals(expected, actual)
    }

    @Test
    fun testUpdateUserState() {
        withSession { session: Session -> prepareState(session) }

        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID,
            requestTime,
            emptyMap(), mapOf("a" to 456),
            applicationState = emptyMap()
        )

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 456, "b" to "q"),
        )

        assertEqualStatesOnly(expected, actual)
        assertThat(actual.user?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
    }

    @Test
    fun testUpdateApplicationState() {
        withSession { session: Session -> prepareState(session) }

        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID, requestTime, emptyMap(), emptyMap(), mapOf("e" to 456)
        )

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 123, "b" to "q"),
            applicationState = mapOf<String, Any>("e" to 456)
        )
        assertEqualStatesOnly(expected, actual)
        assertThat(actual.application?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
    }

    @Test
    fun testAddValueToUserState() {
        withSession { session: Session -> prepareState(session) }
        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID, requestTime, emptyMap(), mapOf("c" to 456), null
        )

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 123, "b" to "q", "c" to 456),
            applicationState = mapOf<String, Any>("e" to 123, "f" to "q")
        )

        assertEqualStatesOnly(expected, actual)
        assertThat(actual.user?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
    }

    @Test
    fun testDeleteValueFromUserState() {
        withSession { session: Session -> prepareState(session) }
        val stateIncrement = HashMap<String, Any?>(1)
        stateIncrement["b"] = null

        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID, requestTime, emptyMap(), stateIncrement, emptyMap()
        )
        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 123)
        )

        assertEqualStatesOnly(expected, actual)
        assertThat(actual.user?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
    }

    @Test
    fun testDeleteValueFromApplicationState() {
        withSession { session: Session -> prepareState(session) }
        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID, requestTime, emptyMap(), emptyMap(), emptyMap()
        )

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 123, "b" to "q")
        )
        assertEqualStatesOnly(expected, actual)
    }

    @Test
    fun testComplexStateChange() {
        withSession { session: Session -> prepareState(session) }

        val userStateIncrement = mutableMapOf<String, Any?>()
        userStateIncrement["b"] = null
        userStateIncrement["c"] = 1
        userStateIncrement["a"] = "123"

        val applicationState = mutableMapOf<String, Any>()
        applicationState["g"] = 1
        applicationState["e"] = "123"

        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID, requestTime,
            mapOf("session_state" to mapOf("a" to "a")),
            userStateIncrement,
            applicationState
        )
        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            DEFAULT_TIMESTAMP,
            mapOf<String, Any>("a" to "123", "c" to 1),
            mapOf<String, Any>("session_state" to mapOf("a" to "a")),
            mapOf<String, Any>("e" to "123", "g" to 1)
        )

        assertEqualStatesOnly(expected, actual)
        assertThat(actual.user?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
        assertThat(actual.session?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
        assertThat(actual.application?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
    }

    @Test
    fun testDeleteSessionState() {
        withSession { session: Session -> prepareState(session) }
        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID, requestTime, emptyMap(), emptyMap(), null
        )

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            timestamp = DEFAULT_TIMESTAMP,
            userState = mapOf<String, Any>("a" to 123, "b" to "q"),
            applicationState = mapOf<String, Any>("e" to 123, "f" to "q")
        )
        Assertions.assertEquals(expected, actual)
    }

    @Test
    fun testUpdateSessionState() {
        withSession { session: Session -> prepareState(session) }
        stateDao.storeSessionAndUserAndApplicationState(
            DEFAULT_STATE_ID, requestTime,
            mapOf("t" to 1),
            emptyMap(), null
        )

        val actual = stateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationId(DEFAULT_STATE_ID)
        val expected = createSkillState(
            DEFAULT_TIMESTAMP,
            mapOf<String, Any>("a" to 123, "b" to "q"),
            mapOf<String, Any>("t" to 1),
            mapOf<String, Any>("e" to 123, "f" to "q")
        )

        assertEqualStatesOnly(expected, actual)
        assertThat(actual.session?.timestamp, Matchers.greaterThan(DEFAULT_TIMESTAMP))
    }

    private fun assertEqualStatesOnly(expected: SkillState, actual: SkillState) {
        Assertions.assertAll(
            Executable {
                Assertions.assertEquals(
                    expected.getSessionState(), actual.getSessionState(),
                    "Session state comparison failed"
                )
            },
            Executable {
                Assertions.assertEquals(
                    expected.getUserState(), actual.getUserState(),
                    "User state comparison failed"
                )
            },
            Executable {
                Assertions.assertEquals(
                    expected.getApplicationState(), actual.getApplicationState(),
                    "Application state comparison failed"
                )
            }
        )
    }

    private fun createSkillState(
        timestamp: Instant = DEFAULT_TIMESTAMP,
        userState: Map<String, Any>? = null,
        sessionState: Map<String, Any>? = null,
        applicationState: Map<String, Any>? = null
    ): SkillState {
        return create(
            userState?.let { UserStateEntity(SKILL_ID, USER_ID, timestamp, userState) },
            sessionState?.let { SessionStateEntity(SKILL_ID, SESSION_ID, timestamp, sessionState) },
            applicationState?.let { ApplicationStateEntity(SKILL_ID, APP_ID, timestamp, applicationState) }
        )
    }

    private fun prepareState(session: Session) {
        session.executeDataQuery(
            """
                PRAGMA TablePathPrefix('$ydbDatabase');
                INSERT INTO skill_user_state (user_id_hash, user_id, skill_id, changed_at, state)
                VALUES(Digest::CityHash('$USER_ID'), '$USER_ID', '$SKILL_ID', CAST('$DEFAULT_TIMESTAMP' as Timestamp), CAST('{"a": 123, "b": "q"}' as Json));
                INSERT INTO skill_session_state (session_id_hash, session_id, skill_id, changed_at, state)
                VALUES(Digest::CityHash('$SESSION_ID'), '$SESSION_ID', '$SKILL_ID', CAST('$DEFAULT_TIMESTAMP' as Timestamp), CAST('{"c": 123, "d": "q"}' as Json));
                INSERT INTO skill_application_state (application_id_hash, skill_id, application_id, changed_at, state)
                VALUES(Digest::CityHash('$APP_ID'), '$SKILL_ID', '$APP_ID', CAST('$DEFAULT_TIMESTAMP' as Timestamp), CAST('{"e": 123, "f": "q"}' as Json));

            """.trimIndent(),
            TxControl.serializableRw().setCommitTx(true)
        ).join().expect("failed to insert preparedState")
    }
}
