package ru.yandex.alice.memento.settings

import com.google.protobuf.InvalidProtocolBufferException
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceConfig
import ru.yandex.alice.memento.proto.MementoApiProto.TUserConfigs
import ru.yandex.alice.memento.scanner.KeyMappingScanner
import ru.yandex.alice.memento.storage.KeysToFetch
import ru.yandex.alice.memento.storage.StorageDao
import ru.yandex.alice.memento.storage.StoredData
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import com.google.protobuf.Any as ProtoAny

internal class SettingsStorageImpl(
    private val storageDao: StorageDao,
    private val scanner: KeyMappingScanner,
    private val metricRegistry: MetricRegistry,
) : SettingsStorage {

    private val logger = LogManager.getLogger()

    override fun getSettings(request: SettingsRequest): Settings {
        val userId = request.uid
        val userKeys: Set<String> = request.userSettingsKeys
            .filter { validConfigKey(it) }
            .map { scanner.getDbKey(it) }
            .toSet()
        val deviceKeys = request.deviceSettingsKeys.mapValues { mapToDbKey(it.value) }

        // fetching
        val keysToFetch = KeysToFetch(userKeys, deviceKeys, request.scenarios, request.surfaceScenarios)
        val fetched = storageDao.fetch(userId, keysToFetch, request.anonymous)

        val userSettings = userKeys.associate {
            scanner.userConfigEnumByKey(it) to fetchedOrDefault(it, fetched.userSettings)
        }

        val devicesSettings: Map<String, Map<EDeviceConfigKey, ProtoAny>> = deviceKeys
            .mapValues { (deviceId, keys) ->
                val deviceSettings = fetched.deviceSettings.getOrDefault(deviceId, emptyMap())
                keys.associate { key ->
                    scanner.deviceConfigEnumByKey(key) to fetchedOrDefaultForDevice(key, deviceSettings)
                }
            }

        return Settings(
            userSettings, devicesSettings, fetched.scenariosData, fetched.surfaceScenariosData
        )
    }

    override fun getAllObjects(uid: String, surfaces: Set<String>, anonymous: Boolean): AllSettings {
        val fetched: StoredData = storageDao.fetchAll(uid, surfaces, anonymous)

        val userConfigs = makeUserConfigs(fetched.userSettings)
        val surfaceConfigMap: Map<String, TSurfaceConfig> = surfaces.associateWith { surfaceId ->
            makeSurfaceConfigs(fetched.deviceSettings.getOrDefault(surfaceId, emptyMap()))
        }
        return AllSettings(userConfigs, surfaceConfigMap, fetched.scenariosData, fetched.surfaceScenariosData)
    }

    private fun makeUserConfigs(userSettings: Map<String, ProtoAny>): TUserConfigs {
        val builder = TUserConfigs.newBuilder()
        for ((dbKey, data) in userSettings) {
            val configKey = scanner.userConfigEnumByKey(dbKey)
            if (!validConfigKey(configKey)) {
                break
            }
            try {
                scanner.setValue(configKey, builder, data.unpack(scanner.getClassForKey(configKey)))
            } catch (e: InvalidProtocolBufferException) {
                logger.error("Failed to unpack Any for key ${configKey.name}", e)
            }
        }

        for ((key, value) in scanner.userConfigExplicitDefaults) {
            if (scanner.getDbKey(key) !in userSettings) {
                scanner.setValue(key, builder, value)
            }
        }

        return builder.build()
    }

    private fun makeSurfaceConfigs(surfaceSettings: Map<String, ProtoAny>): TSurfaceConfig {
        val builder = TSurfaceConfig.newBuilder()
        for ((dbKey, data) in surfaceSettings) {
            val configKey = scanner.deviceConfigEnumByKey(dbKey)
            if (!validConfigKey(configKey)) {
                break
            }
            try {
                scanner.setValue(configKey, builder, data.unpack(scanner.getClassForKey(configKey)))
            } catch (e: InvalidProtocolBufferException) {
                logger.error("Failed to unpack Any for key ${configKey.name}", e)
            }
        }

        for ((key, value) in scanner.deviceConfigExplicitDefaults) {
            if (scanner.getDbKey(key) !in surfaceSettings) {
                scanner.setValue(key, builder, value)
            }
        }

        return builder.build()
    }

    override fun updateUserSettings(userId: String, settings: Settings, anonymous: Boolean) {
        val userChanges: Map<String, ProtoAny> = settings.userSettings
            .filter { entry -> correctKeyToTypeUrlPair(entry) }
            .mapKeys { (key, _) -> scanner.getDbKey(key) }

        val devicesChanges: Map<String, Map<String, ProtoAny>> = settings.deviceSettings
            .mapValues { (_, deviceEntry) -> mapByDbKey(deviceEntry) }

        if (userChanges.isNotEmpty()) {
            userChanges.keys.forEach { key ->
                metricRegistry.rate("key_updated", Labels.of("type", "user_settings", "key", key))
            }
            logger.info("updating user config keys: {}", { userChanges.keys })
        }

        if (devicesChanges.isNotEmpty()) {
            devicesChanges.values.forEach { surfaceMap ->
                surfaceMap.keys.forEach { key ->
                    metricRegistry.rate(
                        "key_updated",
                        Labels.of("type", "device_settings", "key", key)
                    )
                }
            }
            logger.info("updating surface config keys: {}", { devicesChanges.mapValues { (_, v) -> v.keys } })
        }

        if (settings.scenariosData.isNotEmpty()) {
            settings.scenariosData.keys.forEach { key ->
                metricRegistry.rate("key_updated", Labels.of("type", "user_settings", "key", key))
            }
            logger.info("updating scenarios data: {}", { settings.scenariosData.keys })
        }

        if (settings.surfaceScenarioData.isNotEmpty()) {
            settings.surfaceScenarioData.values.forEach { surfaceMap ->
                surfaceMap.keys.forEach { key ->
                    metricRegistry.rate(
                        "key_updated",
                        Labels.of("type", "surface_scenario_data", "key", key)
                    )
                }
            }
            logger.info("updating surfaces scenarios data: {}") {
                settings.surfaceScenarioData.mapValues { (_, v) -> v.keys }
            }
        }

        val storedData = StoredData(userChanges, devicesChanges, settings.scenariosData, settings.surfaceScenarioData)
        storageDao.update(userId, storedData, anonymous)
    }

    override fun removeAllObjects(uid: String) {
        logger.info("removing all objects for user {}", uid)
        storageDao.removeAllObjects(uid)
    }

    private fun mapToDbKey(deviceEntry: Set<EDeviceConfigKey>): Set<String> {
        return deviceEntry
            .filter { key -> validConfigKey(key) }
            .map { scanner.getDbKey(it) }
            .toHashSet()
    }

    private fun mapByDbKey(deviceEntry: Map<EDeviceConfigKey, ProtoAny>): Map<String, ProtoAny> {
        return deviceEntry
            .filter { checkDeviceKeys(it) }
            .mapKeys { (key, _) -> scanner.getDbKey(key) }
    }

    private fun correctKeyToTypeUrlPair(entry: Map.Entry<EConfigKey, ProtoAny>): Boolean {
        return entry.value.typeUrl == scanner.typeUrlForEnum(entry.key)
    }

    private fun checkDeviceKeys(entry: Map.Entry<EDeviceConfigKey, ProtoAny>): Boolean {
        return entry.value.typeUrl == scanner.typeUrlForEnum(entry.key)
    }

    private fun fetchedOrDefault(key: String, userSettings: Map<String, ProtoAny>): ProtoAny {
        return userSettings.getOrElse(key, { scanner.getDefaultForKey(scanner.userConfigEnumByKey(key)) })
    }

    private fun fetchedOrDefaultForDevice(key: String, userSettings: Map<String, ProtoAny>): ProtoAny {
        return userSettings.getOrElse(key, { scanner.getDefaultForKey(scanner.deviceConfigEnumByKey(key)) })
    }

    private fun validConfigKey(key: EConfigKey) =
        key != EConfigKey.UNRECOGNIZED && key != EConfigKey.CK_UNDEFINED

    private fun validConfigKey(key: EDeviceConfigKey) =
        key != EDeviceConfigKey.UNRECOGNIZED && key != EDeviceConfigKey.DCK_UNDEFINED
}
