package ru.yandex.alice.memento.storage

import com.google.common.base.Stopwatch
import com.google.common.util.concurrent.ThreadFactoryBuilder
import com.google.protobuf.InvalidProtocolBufferException
import com.yandex.ydb.table.query.Params
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.values.ListValue
import com.yandex.ydb.table.values.PrimitiveValue
import com.yandex.ydb.table.values.StructValue
import com.yandex.ydb.table.values.TupleValue
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.paskills.common.ydb.YdbClient
import java.time.Duration
import java.time.Instant
import java.util.concurrent.CompletableFuture
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import com.google.protobuf.Any as ProtoAny

private fun userSettingsTable(anonymous: Boolean): String =
    if (!anonymous) "user_settings" else "user_settings_anonymous"

private fun userDeviceSettingsTable(anonymous: Boolean): String =
    if (!anonymous) "user_device_settings" else "user_device_settings_anonymous"

private fun userScenarioDataTable(anonymous: Boolean): String =
    if (!anonymous) "user_scenario_data" else "user_scenario_data_anonymous"

private fun userSurfaceScenarioDataTable(anonymous: Boolean): String =
    if (!anonymous) "user_surface_scenario_data" else "user_surface_scenario_data_anonymous"

private fun QUERY_USER_SETTINGS(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}keys as "List<Utf8>";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select key, data from ${userSettingsTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id and key in ${"$"}keys;
    """

private fun QUERY_ALL_USER_SETTINGS(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select key, data from ${userSettingsTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id;
    """

private fun STORE_USER_SETTINGS(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}changes as "List<Struct<key:Utf8,data:String>>";
    DECLARE ${"$"}now as "Timestamp";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    UPSERT INTO ${userSettingsTable(anonymous)} (user_id_hash, user_id, key, data, changed_at)
    select ${"$"}user_id_hash, ${"$"}user_id, key, data, ${"$"}now from AS_TABLE(${"$"}changes);
    """

private fun QUERY_DEVICE_SETTINGS(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}devices_keys as "List<Tuple<Utf8,Utf8>>";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select device_id, key, data from ${userDeviceSettingsTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id and (device_id,key) in ${"$"}devices_keys;
    """

private fun QUERY_ALL_DEVICE_SETTINGS(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}devices as "List<Utf8>";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select device_id, key, data from ${userDeviceSettingsTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id and (device_id) in ${"$"}devices;
    """

private fun STORE_DEVICE_SETTINGS(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}changes as "List<Struct<device_id:Utf8,key:Utf8,data:String>>";
    DECLARE ${"$"}now as "Timestamp";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    UPSERT INTO ${userDeviceSettingsTable(anonymous)} (user_id_hash, user_id, device_id, key, data, changed_at)
    select ${"$"}user_id_hash, ${"$"}user_id, device_id, key, data, ${"$"}now from AS_TABLE(${"$"}changes);
    """

private fun QUERY_SCENARIO_DATA(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}scenario_names as "List<Utf8>";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select scenario_name, data from ${userScenarioDataTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id and scenario_name in ${"$"}scenario_names;
    """

private fun QUERY_ALL_SCENARIOS_DATA(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select scenario_name, data from ${userScenarioDataTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id;
    """

private fun STORE_SCENARIO_DATA(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}changes as "List<Struct<scenario_name:Utf8,data:String>>";
    DECLARE ${"$"}now as "Timestamp";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    UPSERT INTO ${userScenarioDataTable(anonymous)} (user_id_hash, user_id, scenario_name, data, changed_at)
    select ${"$"}user_id_hash, ${"$"}user_id, scenario_name, data, ${"$"}now from AS_TABLE(${"$"}changes);
    """

private fun QUERY_SURFACE_SCENARIO_DATA(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}surface_keys as "List<Tuple<Utf8,Utf8>>";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select surface_id, scenario_name, data from ${userSurfaceScenarioDataTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id and (surface_id,scenario_name) in ${"$"}surface_keys;
    """

private fun QUERY_ALL_SURFACE_SCENARIOS_DATA(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}surfaces as "List<Utf8>";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    select surface_id, scenario_name, data from ${userSurfaceScenarioDataTable(anonymous)}
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id and surface_id in ${"$"}surfaces;
    """

private fun STORE_SURFACE_SCENARIO_DATA(anonymous: Boolean) = """
    DECLARE ${"$"}user_id as "Utf8";
    DECLARE ${"$"}changes as "List<Struct<surface_id:Utf8,scenario_name:Utf8,data:String>>";
    DECLARE ${"$"}now as "Timestamp";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);
    UPSERT INTO ${userSurfaceScenarioDataTable(anonymous)} (user_id_hash, user_id, surface_id, scenario_name, data, changed_at)
    select ${"$"}user_id_hash, ${"$"}user_id, surface_id, scenario_name, data, ${"$"}now from AS_TABLE(${"$"}changes);
    """

private const val DELETE_ALL_USER_OBJECTS = """
    DECLARE ${"$"}user_id as "Utf8";
    ${"$"}user_id_hash = Digest::CityHash(${"$"}user_id);

    delete from user_settings
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id;

    delete from user_device_settings
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id;

    delete from user_scenario_data
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id;

    delete from user_surface_scenario_data
    where user_id_hash = ${"$"}user_id_hash and user_id = ${"$"}user_id;
    """

internal class StorageDaoImpl(
    private val ydbClient: YdbClient,
    private val timeout: Long
) : StorageDao {

    private val logger = LogManager.getLogger(javaClass)

    private val rowMapper = RowMapper()

    internal fun warmup(
        requestCount: Int,
        wavesCount: Int = 1
    ) {

        ydbClient.warmUpSessionPool(
            listOf(
                QUERY_USER_SETTINGS(false),
                QUERY_ALL_USER_SETTINGS(false),
                STORE_USER_SETTINGS(false),
                QUERY_DEVICE_SETTINGS(false),
                QUERY_ALL_DEVICE_SETTINGS(false),
                STORE_DEVICE_SETTINGS(false),
                QUERY_SCENARIO_DATA(false),
                QUERY_ALL_SCENARIOS_DATA(false),
                STORE_SCENARIO_DATA(false),
                QUERY_SURFACE_SCENARIO_DATA(false),
                QUERY_ALL_SURFACE_SCENARIOS_DATA(false),
                STORE_SURFACE_SCENARIO_DATA(false),
                QUERY_USER_SETTINGS(true),
                QUERY_ALL_USER_SETTINGS(true),
                STORE_USER_SETTINGS(true),
                QUERY_DEVICE_SETTINGS(true),
                QUERY_ALL_DEVICE_SETTINGS(true),
                STORE_DEVICE_SETTINGS(true),
                QUERY_SCENARIO_DATA(true),
                QUERY_ALL_SCENARIOS_DATA(true),
                STORE_SCENARIO_DATA(true),
                QUERY_SURFACE_SCENARIO_DATA(true),
                QUERY_ALL_SURFACE_SCENARIOS_DATA(true),
                STORE_SURFACE_SCENARIO_DATA(true),
            )
        )

        // make some queries to warmup sessions. First query in tests take 180ms+ but following queries execute in <20ms
        val executor = Executors.newFixedThreadPool(
            requestCount + 1,
            ThreadFactoryBuilder()
                .setNameFormat("ydb-warmup-executor")
                .build()
        )

        val svAll = Stopwatch.createStarted()
        for (waveIdx in 1..wavesCount) {

            val sw = Stopwatch.createStarted()
            logger.info("start warmup queries wave $waveIdx")
            CompletableFuture.allOf(
                *IntRange(1, requestCount + 1).map { i ->
                    val uid = waveIdx * 1000 + i + 100
                    CompletableFuture.runAsync(
                        {
                            logger.info("Perform warmup for user $uid")

                            val start = System.nanoTime()
                            fetchAll(uid.toString(), setOf(uid.toString()), false)

                            val dur = (System.nanoTime() - start) / 1_000_000
                            logger.info("Warmup for user $uid completed in ${dur}ms")
                        },
                        executor

                    )
                }.toTypedArray()
            ).join()
            logger.info("Completed warmup wave $waveIdx in $sw")
        }

        logger.info("complete warmup queries in $svAll")

        executor.shutdown()
    }

    override fun fetch(uid: String, keysToFetch: KeysToFetch, anonymous: Boolean): StoredData {
        val userKeys = keysToFetch.userKeys
        val devicesKeys = keysToFetch.devicesKeys
        val scenarios = keysToFetch.scenarios
        val surfaceScenarios = keysToFetch.surfaceScenarios
        val userDataFuture: CompletableFuture<List<UserData>> = fetchUserConfigs(uid, userKeys, anonymous)
        val devicesDataRowsFuture: CompletableFuture<List<DevicesData>> =
            fetchSurfaceConfigs(uid, devicesKeys, anonymous)
        val scenarioDataFuture: CompletableFuture<List<ScenarioData>> = fetchScenarioData(uid, scenarios, anonymous)

        val surfaceScenarioDataFuture: CompletableFuture<List<SurfaceScenarioData>> =
            fetchSurfaceScenarioData(uid, surfaceScenarios, anonymous)

        return makeStoredData(userDataFuture, devicesDataRowsFuture, scenarioDataFuture, surfaceScenarioDataFuture)
    }

    override fun fetchAll(uid: String, surfaces: Set<String>, anonymous: Boolean): StoredData {
        val userDataFuture = fetchAllUserConfigs(uid, anonymous)
        val devicesDataRowsFuture = fetchAllSurfaceConfigs(uid, surfaces, anonymous)
        val scenarioDataFuture = fetchAllScenariosData(uid, anonymous)
        val surfaceScenarioDataFuture = fetchAllSurfaceScenarioData(uid, surfaces, anonymous)

        return makeStoredData(userDataFuture, devicesDataRowsFuture, scenarioDataFuture, surfaceScenarioDataFuture)
    }

    private fun makeStoredData(
        userDataFuture: CompletableFuture<List<UserData>>,
        devicesDataRows: CompletableFuture<List<DevicesData>>,
        scenarioDataFuture: CompletableFuture<List<ScenarioData>>,
        surfaceScenarioDataFuture: CompletableFuture<List<SurfaceScenarioData>>
    ): StoredData {
        val userSettings = userDataFuture.join().associate { (key, data) -> key to data }
        val deviceSettings = devicesDataRows.join() // .map { it.deviceId to (it.key to it.deviceId) }
            .groupBy { it.deviceId }
            .mapValues { (_, v) -> v.associate { it.key to it.data } }
        val scenarioData = scenarioDataFuture.join().associate { (scenarioName, data) -> scenarioName to data }
        val surfaceScenarioData = surfaceScenarioDataFuture.join()
            .groupBy { it.surfaceId }
            .mapValues { (_, v) -> v.associate { it.scenarioName to it.data } }
        return StoredData(userSettings, deviceSettings, scenarioData, surfaceScenarioData)
    }

    private fun fetchUserConfigs(
        uid: String,
        userKeys: Set<String>,
        anonymous: Boolean,
    ): CompletableFuture<List<UserData>> {
        if (userKeys.isEmpty()) {
            return CompletableFuture.completedFuture(emptyList())
        }

        val userDataFuture: CompletableFuture<List<UserData>>
        logger.info("fetching user settings for user: {}, keys: {}", uid, userKeys)
        val userSettingsQueryParams = userQueryParams(uid, userKeys)
        userDataFuture = ydbClient.readFirstResultSetAsync(
            "query_" + userSettingsTable(anonymous), QUERY_USER_SETTINGS(anonymous),
            userSettingsQueryParams
        ) { reader: ResultSetReader -> rowMapper.mapUserSettingsRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<UserData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query user settings for user $uid", e)
                } else {
                    logger.info(
                        "Fetched user settings records: {} for user {}",
                        r?.map { it.key } ?: listOf<String>(),
                        uid
                    )
                }
            }
        return userDataFuture
    }

    private fun fetchAllUserConfigs(uid: String, anonymous: Boolean): CompletableFuture<List<UserData>> {
        logger.info("fetching all user settings for user: {}", uid)
        val params = userQueryAllParams(uid)
        val sw = Stopwatch.createStarted()
        return ydbClient.readFirstResultSetAsync(
            "query_all_" + userSettingsTable(anonymous), QUERY_ALL_USER_SETTINGS(anonymous),
            params
        ) { reader: ResultSetReader -> rowMapper.mapUserSettingsRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<UserData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query all user settings for user $uid", e)
                } else {
                    logger.info(
                        "Fetched all user settings records for keys: {} for user {} took {}ms",
                        r?.map { it.key } ?: listOf<String>(),
                        uid,
                        sw.stop().elapsed().toMillis()
                    )
                }
            }
    }

    private fun fetchSurfaceConfigs(
        uid: String,
        devicesKeys: Map<String, Set<String>>,
        anonymous: Boolean,
    ): CompletableFuture<List<DevicesData>> {
        if (devicesKeys.isEmpty()) {
            return CompletableFuture.completedFuture(emptyList())
        }

        logger.info("fetching device settings for user: {}, devices and keys: {}", uid, devicesKeys)
        val devicesSettingsParams = devicesQueryParams(uid, devicesKeys)
        val sw = Stopwatch.createStarted()
        return ydbClient.readFirstResultSetAsync(
            "query_" + userDeviceSettingsTable(anonymous), QUERY_DEVICE_SETTINGS(anonymous), devicesSettingsParams
        ) { reader: ResultSetReader -> rowMapper.mapUserDeviceSettingsRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<DevicesData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query device settings for user $uid", e)
                } else {
                    logger.info(
                        "Fetched device settings records: {} for user {} took {}ms",
                        r?.map { it.deviceId + " - " + it.key } ?: listOf<String>(),
                        uid,
                        sw.stop().elapsed().toMillis()
                    )
                }
            }
    }

    private fun fetchAllSurfaceConfigs(
        uid: String,
        surfaces: Set<String>,
        anonymous: Boolean,
    ): CompletableFuture<List<DevicesData>> {
        if (surfaces.isEmpty()) {
            return CompletableFuture.completedFuture(emptyList())
        }

        logger.info("fetching all device settings for user: {}, surfaces: {}", uid, surfaces)
        val devicesSettingsParams = devicesQueryAllParams(uid, surfaces)
        val sw = Stopwatch.createStarted()
        return ydbClient.readFirstResultSetAsync(
            "query_all_" + userDeviceSettingsTable(anonymous), QUERY_ALL_DEVICE_SETTINGS(anonymous),
            devicesSettingsParams
        ) { reader: ResultSetReader -> rowMapper.mapUserDeviceSettingsRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<DevicesData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query all device settings for user $uid", e)
                } else {
                    logger.info(
                        "Fetched all device settings records: {} for user {} took {}ms",
                        r?.map { it.deviceId + " - " + it.key } ?: listOf<String>(),
                        uid,
                        sw.stop().elapsed().toMillis()
                    )
                }
            }
    }

    private fun fetchScenarioData(
        uid: String,
        scenarios: Set<String>,
        anonymous: Boolean,
    ): CompletableFuture<List<ScenarioData>> {
        if (scenarios.isEmpty()) {
            return CompletableFuture.completedFuture(emptyList())
        }

        logger.info("fetching scenario data for user: {}, scenarios: {}", uid, scenarios)
        val userSettingsQueryParams = scenarioQueryParams(uid, scenarios)
        return ydbClient.readFirstResultSetAsync(
            "query_" + userScenarioDataTable(anonymous), QUERY_SCENARIO_DATA(anonymous),
            userSettingsQueryParams
        ) { reader: ResultSetReader -> rowMapper.mapScenarioDataRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<ScenarioData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query scenario data for user $uid", e)
                } else {
                    logger.info(
                        "Fetched scenario data records: {} for user {}",
                        r?.map { it.scenarioName } ?: listOf<String>(),
                        uid
                    )
                }
            }
    }

    private fun fetchAllScenariosData(uid: String, anonymous: Boolean): CompletableFuture<List<ScenarioData>> {
        logger.info("fetching all scenarios data for user: {}", uid)
        val userSettingsQueryParams = allScenarioQueryParams(uid)
        val sw = Stopwatch.createStarted()
        return ydbClient.readFirstResultSetAsync(
            "query_all_" + userScenarioDataTable(anonymous), QUERY_ALL_SCENARIOS_DATA(anonymous),
            userSettingsQueryParams
        ) { reader: ResultSetReader -> rowMapper.mapScenarioDataRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<ScenarioData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query all scenarios data for user $uid", e)
                } else {
                    logger.info(
                        "Fetched all scenarios data records: {} for user {} took {}ms",
                        r?.map { it.scenarioName } ?: listOf<String>(),
                        uid,
                        sw.stop().elapsed().toMillis()
                    )
                }
            }
    }

    private fun fetchSurfaceScenarioData(
        uid: String,
        surfaceScenarios: Map<String, Set<String>>,
        anonymous: Boolean,
    ): CompletableFuture<List<SurfaceScenarioData>> {
        if (surfaceScenarios.isEmpty()) {
            return CompletableFuture.completedFuture(emptyList())
        }

        logger.info(
            "fetching surface scenario data for user: {}, devices and scenario_names: {}",
            uid, surfaceScenarios
        )
        val devicesSettingsParams = surfaceScenarioQueryParams(uid, surfaceScenarios)
        return ydbClient.readFirstResultSetAsync(
            "query_" + userSurfaceScenarioDataTable(anonymous), QUERY_SURFACE_SCENARIO_DATA(anonymous),
            devicesSettingsParams
        ) { reader: ResultSetReader -> rowMapper.mapSurfaceScenarioDataRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<SurfaceScenarioData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query surface scenarios data for user $uid", e)
                } else {
                    logger.info(
                        "Fetched surface scenarios data records: {} for user {}",
                        r?.map { it.surfaceId + "-" + it.scenarioName } ?: listOf<String>(),
                        uid
                    )
                }
            }
    }

    private fun fetchAllSurfaceScenarioData(
        uid: String,
        surfaces: Set<String>,
        anonymous: Boolean,
    ): CompletableFuture<List<SurfaceScenarioData>> {
        if (surfaces.isEmpty()) {
            return CompletableFuture.completedFuture(emptyList())
        }

        logger.info(
            "fetching all surface scenario data for user: {}, surfaces: {}",
            uid, surfaces
        )
        val sw = Stopwatch.createStarted()
        val devicesSettingsParams = surfaceAllScenarioQueryParams(uid, surfaces)
        return ydbClient.readFirstResultSetAsync(
            "query_all_" + userSurfaceScenarioDataTable(anonymous), QUERY_ALL_SURFACE_SCENARIOS_DATA(anonymous),
            devicesSettingsParams
        ) { reader: ResultSetReader -> rowMapper.mapSurfaceScenarioDataRow(reader) }
            .completeOnTimeout(emptyList(), timeout, TimeUnit.MILLISECONDS)
            .whenComplete { r: List<SurfaceScenarioData>?, e: Throwable? ->
                if (e != null) {
                    logger.error("Failed to query all surface scenario data for user $uid", e)
                } else {
                    logger.info(
                        "Fetched all surface scenario data records: {} for user {} took {}ms",
                        r?.map { it.surfaceId + "-" + it.scenarioName } ?: listOf<String>(),
                        uid,
                        sw.stop().elapsed().toMillis()
                    )
                }
            }
    }

    override fun update(userId: String, changes: StoredData, anonymous: Boolean) {
        with(changes) {
            val storeUserFuture = if (userSettings.isNotEmpty()) {
                logger.info("Updating user settings: {}", userSettings.keys)
                ydbClient.executeRwAsync(
                    "store_" + userSettingsTable(anonymous), STORE_USER_SETTINGS(anonymous),
                    updateUserParams(userId, userSettings)
                )
            } else {
                CompletableFuture.completedFuture<Any?>(null)
            }

            val deviceChangesPresent = deviceSettings.values.any { it.isNotEmpty() }
            val storeDevicesFuture = if (deviceChangesPresent) {
                logger.info(
                    "Updating surface settings: {}",
                    deviceSettings.map { (key, value) -> "$key - ${value.keys}" }
                )
                ydbClient.executeRwAsync(
                    "store_" + userDeviceSettingsTable(anonymous), STORE_DEVICE_SETTINGS(anonymous),
                    updateDevicesParams(userId, deviceSettings)
                )
            } else {
                CompletableFuture.completedFuture<Any?>(null)
            }

            val storeScenarioFuture = if (scenariosData.isNotEmpty()) {
                logger.info("Updating scenario settings: {}", scenariosData.keys)
                ydbClient.executeRwAsync(
                    "store_" + userScenarioDataTable(anonymous), STORE_SCENARIO_DATA(anonymous),
                    updateScenarioParams(userId, scenariosData)
                )
            } else {
                CompletableFuture.completedFuture<Any?>(null)
            }

            val surfaceScenarioPresent = surfaceScenariosData.values.any { it.isNotEmpty() }
            val surfaceScenarioFuture = if (surfaceScenarioPresent) {
                logger.info(
                    "Updating surface settings: {}",
                    deviceSettings.map { (key, value) -> "$key - ${value.keys}" }
                )
                ydbClient.executeRwAsync(
                    "store_" + userScenarioDataTable(anonymous), STORE_SURFACE_SCENARIO_DATA(anonymous),
                    updateSurfaceScenarioParams(userId, surfaceScenariosData)
                )
            } else {
                CompletableFuture.completedFuture<Any?>(null)
            }

            CompletableFuture.allOf(storeUserFuture, storeDevicesFuture, storeScenarioFuture, surfaceScenarioFuture)
                .orTimeout(timeout, TimeUnit.MILLISECONDS)
                .whenComplete { _, e: Throwable? ->
                    if (e != null) {
                        logger.error("Failed to save states for user $userId", e)
                    } else {
                        logger.info("Storing procedure completed for user {}", userId)
                    }
                }
                .join()
        }
    }

    override fun removeAllObjects(uid: String) {
        logger.info("removing all user objects for user: {}", uid)
        ydbClient.executeRw(
            "remove_all_user_objects", DELETE_ALL_USER_OBJECTS,
            Params.of("\$user_id", PrimitiveValue.utf8(uid)), Duration.ofMillis(1000)
        )
        logger.info("Successfully removed all user objects for user {}", uid)
    }

    private fun updateDevicesParams(userId: String, devicesChanges: Map<String, Map<String, ProtoAny>>): Params {
        val structs: Array<StructValue> = devicesChanges.entries.flatMap {
            val key = it.key
            it.value.entries.map { v ->
                StructValue.of(
                    "device_id", PrimitiveValue.utf8(key),
                    "key", PrimitiveValue.utf8(v.key),
                    "data", PrimitiveValue.string(v.value.toByteArray())
                )
            }
        }.toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(userId),
            "\$changes", ListValue.of(*structs),
            "\$now", PrimitiveValue.timestamp(Instant.now())
        )
    }

    private fun updateUserParams(userId: String, keysToChange: Map<String, ProtoAny>): Params {
        val changesParam = keysToChange.map {
            StructValue.of(
                "key", PrimitiveValue.utf8(it.key),
                "data", PrimitiveValue.string(it.value.toByteArray())
            )
        }.toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(userId),
            "\$changes", ListValue.of(*changesParam),
            "\$now", PrimitiveValue.timestamp(Instant.now())
        )
    }

    private fun updateScenarioParams(uid: String, scenariosData: Map<String, ProtoAny>): Params {
        val changesParam = scenariosData.map {
            StructValue.of(
                "scenario_name", PrimitiveValue.utf8(it.key),
                "data", PrimitiveValue.string(it.value.toByteArray())
            )
        }
            .toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$changes", ListValue.of(*changesParam),
            "\$now", PrimitiveValue.timestamp(Instant.now())
        )
    }

    private fun updateSurfaceScenarioParams(
        uid: String,
        surfaceScenariosData: Map<String, Map<String, ProtoAny>>
    ): Params {
        val changes = surfaceScenariosData.flatMap {
            val key = it.key
            it.value.entries.map { v ->
                StructValue.of(
                    "surface_id", PrimitiveValue.utf8(key),
                    "scenario_name", PrimitiveValue.utf8(v.key),
                    "data", PrimitiveValue.string(v.value.toByteArray())
                )
            }
        }.toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$changes", ListValue.of(*changes),
            "\$now", PrimitiveValue.timestamp(Instant.now())
        )
    }

    private fun userQueryParams(uid: String, userKeys: Set<String>): Params {
        val keyListParam = userKeys.map { PrimitiveValue.utf8(it) }.toTypedArray()

        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$keys", ListValue.of(*keyListParam)
        )
    }

    private fun userQueryAllParams(uid: String): Params {
        return Params.of("\$user_id", PrimitiveValue.utf8(uid))
    }

    private fun devicesQueryParams(uid: String, devicesKeys: Map<String, Set<String>>): Params {
        val devicesKeysParam = devicesKeys.flatMap { entry ->
            entry.value.map {
                TupleValue.of(PrimitiveValue.utf8(entry.key), PrimitiveValue.utf8(it))
            }
        }.toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$devices_keys", ListValue.of(*devicesKeysParam)
        )
    }

    private fun devicesQueryAllParams(uid: String, devices: Set<String?>): Params {
        val devicesParam = devices.map { PrimitiveValue.utf8(it) }.toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$devices", ListValue.of(*devicesParam)
        )
    }

    private fun scenarioQueryParams(uid: String, scenarios: Set<String>): Params {
        val keyListParam = scenarios.map { PrimitiveValue.utf8(it) }.toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$scenario_names", ListValue.of(*keyListParam)
        )
    }

    private fun allScenarioQueryParams(uid: String): Params {
        return Params.of("\$user_id", PrimitiveValue.utf8(uid))
    }

    private fun surfaceScenarioQueryParams(uid: String, surfaceScenarios: Map<String, Set<String>>): Params {
        val keyListParam = surfaceScenarios.flatMap { entry ->
            entry.value.map {
                TupleValue.of(PrimitiveValue.utf8(entry.key), PrimitiveValue.utf8(it))
            }
        }.toTypedArray()
        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$surface_keys", ListValue.of(*keyListParam)
        )
    }

    private fun surfaceAllScenarioQueryParams(uid: String, surfaces: Set<String?>): Params {
        val keyListParam = surfaces.map { PrimitiveValue.utf8(it) }.toTypedArray()

        return Params.of(
            "\$user_id", PrimitiveValue.utf8(uid),
            "\$surfaces", ListValue.of(*keyListParam)
        )
    }

    private data class UserData(
        val key: String,
        val data: ProtoAny,
    )

    private data class DevicesData(
        val deviceId: String,
        val key: String,
        val data: ProtoAny,
    )

    private data class ScenarioData(
        val scenarioName: String,
        val data: ProtoAny,
    )

    private data class SurfaceScenarioData(
        val surfaceId: String,
        val scenarioName: String,
        val data: ProtoAny,
    )

    private class RowMapper {
        fun mapUserSettingsRow(reader: ResultSetReader): UserData {
            val key = reader.getColumn("key").utf8
            val serializedData = reader.getColumn("data").string
            return try {
                val protoMsg = ProtoAny.newBuilder().mergeFrom(serializedData).build()
                UserData(key, protoMsg)
            } catch (e: InvalidProtocolBufferException) {
                throw RuntimeException(e)
            }
        }

        fun mapUserDeviceSettingsRow(reader: ResultSetReader): DevicesData {
            val deviceId = reader.getColumn("device_id").utf8
            val key = reader.getColumn("key").utf8
            val serializedData = reader.getColumn("data").string
            return try {
                val protoMsg = ProtoAny.newBuilder().mergeFrom(serializedData).build()
                DevicesData(deviceId, key, protoMsg)
            } catch (e: InvalidProtocolBufferException) {
                throw RuntimeException(e)
            }
        }

        fun mapScenarioDataRow(reader: ResultSetReader): ScenarioData {
            val scenarioName = reader.getColumn("scenario_name").utf8
            val serializedData = reader.getColumn("data").string
            return try {
                val protoMsg = ProtoAny.newBuilder().mergeFrom(serializedData).build()
                ScenarioData(scenarioName, protoMsg)
            } catch (e: InvalidProtocolBufferException) {
                throw RuntimeException(e)
            }
        }

        fun mapSurfaceScenarioDataRow(reader: ResultSetReader): SurfaceScenarioData {
            val surfaceId = reader.getColumn("surface_id").utf8
            val scenarioName = reader.getColumn("scenario_name").utf8
            val serializedData = reader.getColumn("data").string
            return try {
                val protoMsg = ProtoAny.newBuilder().mergeFrom(serializedData).build()
                SurfaceScenarioData(surfaceId, scenarioName, protoMsg)
            } catch (e: InvalidProtocolBufferException) {
                throw RuntimeException(e)
            }
        }
    }
}
