package ru.yandex.alice.memento

import com.google.protobuf.Any
import org.springframework.stereotype.Service
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.TConfigKeyAnyPair
import ru.yandex.alice.memento.proto.MementoApiProto.TDeviceConfigs
import ru.yandex.alice.memento.proto.MementoApiProto.TDeviceConfigsKeyAnyPair
import ru.yandex.alice.memento.proto.MementoApiProto.TDeviceKeys
import ru.yandex.alice.memento.proto.MementoApiProto.TReqChangeUserObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetAllObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetUserObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetAllObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetUserObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceScenarioData
import ru.yandex.alice.memento.proto.MementoApiProto.VersionInfo
import ru.yandex.alice.memento.settings.Settings
import ru.yandex.alice.memento.settings.SettingsRequest
import ru.yandex.alice.memento.settings.SettingsStorage
import java.util.EnumSet

@Service
class MementoService(private val settingsStorage: SettingsStorage, private val version: VersionInfo) {

    fun getUserSettings(
        userId: String, anonymous: Boolean, reqUserSettings: TReqGetUserObjects
    ): TRespGetUserObjects {
        val keys: Set<EConfigKey> = enumSet(reqUserSettings.keysList, EConfigKey::class.java)

        val devices: Map<String, Set<EDeviceConfigKey>> = reqUserSettings.devicesKeysList
            .filter { item: TDeviceKeys -> item.deviceId.isNotEmpty() }
            .associate { it.deviceId to enumSet(it.keysList, EDeviceConfigKey::class.java) }

        val scenarioKeys = reqUserSettings.scenarioKeysList.toHashSet()

        val surfaceScenarios = reqUserSettings.surfaceScenarioNamesMap
            .mapValues { (_, keys) -> keys.scenarioNameList.toHashSet() }

        val request = SettingsRequest(userId, anonymous, keys, devices, scenarioKeys, surfaceScenarios)
        val (userSettings, deviceSettings, scenariosData, surfaceScenarioData) = settingsStorage.getSettings(request)

        val results: List<TConfigKeyAnyPair> = getUserSettings(userSettings)
        val devicesSettings: List<TDeviceConfigs> = getDevicesConfigs(deviceSettings)
        val surfaceScenarioDataList: Map<String, TSurfaceScenarioData> = getSurfaceScenarioData(surfaceScenarioData)
        return TRespGetUserObjects.newBuilder()
            .addAllUserConfigs(results)
            .addAllDevicesConfigs(devicesSettings)
            .putAllScenarioData(scenariosData)
            .putAllSurfaceScenarioData(surfaceScenarioDataList)
            .setVersion(version)
            .build()
    }

    fun getAllObjects(
        userId: String, anonymous: Boolean, reqGetAllObjects: TReqGetAllObjects
    ): TRespGetAllObjects {
        val surfaces: Set<String> = reqGetAllObjects.surfaceIdList.toHashSet()

        val settings = settingsStorage.getAllObjects(userId, surfaces, anonymous)

        val surfaceScenarioData = getSurfaceScenarioData(settings.surfaceScenarioData)
        return TRespGetAllObjects.newBuilder()
            .setUserConfigs(settings.userSettings)
            .putAllSurfaceConfigs(settings.surfacesSettings)
            .putAllScenarioData(settings.scenariosData)
            .putAllSurfaceScenarioData(surfaceScenarioData)
            .setVersion(version)
            .build()
    }

    fun removeAllUserData(uid: String) {
        settingsStorage.removeAllObjects(uid)
    }

    private fun <T : Enum<T>?> enumSet(collection: Collection<T>, clazz: Class<T>): EnumSet<T> {
        return if (collection.isEmpty()) EnumSet.noneOf(clazz) else EnumSet.copyOf(collection)
    }

    fun updateObjects(userId: String, anonymous: Boolean, req: TReqChangeUserObjects) {
        val keysToChange: Map<EConfigKey, Any> = req.userConfigsList.associate {
            it.key to it.value
        }
        val deviceKeysToChange: Map<String, Map<EDeviceConfigKey, Any>> = req.devicesConfigsList.associate {
            it.deviceId to (it.deviceConfigsList.associate { p -> p.key to p.value })
        }
        val surfaceScenarioData: Map<String, Map<String, Any>> = req.surfaceScenarioDataMap.mapValues { (_, value) ->
            value.scenarioDataMap
        }
        settingsStorage.updateUserSettings(
            userId,
            Settings(keysToChange, deviceKeysToChange, req.scenarioDataMap, surfaceScenarioData),
            anonymous
        )
    }

    private fun getUserSettings(settings: Map<EConfigKey, Any>): List<TConfigKeyAnyPair> {
        return settings.map { (key, value) ->
            TConfigKeyAnyPair.newBuilder()
                .setKey(key)
                .setValue(value)
                .build()
        }
    }

    private fun getDevicesConfigs(devicesSettings: Map<String, Map<EDeviceConfigKey, Any>>): List<TDeviceConfigs> {
        return devicesSettings
            .map { (deviceId, deviceConfig) ->
                TDeviceConfigs.newBuilder()
                    .setDeviceId(deviceId)
                    .addAllDeviceConfigs(
                        deviceConfig.map { (key, value) ->
                            TDeviceConfigsKeyAnyPair.newBuilder()
                                .setKey(key)
                                .setValue(value)
                                .build()
                        }
                    )
                    .build()
            }
    }

    private fun getSurfaceScenarioData(data: Map<String, Map<String, Any>>): Map<String, TSurfaceScenarioData> {
        return data.mapValues { (_, scenarioMap) ->
            TSurfaceScenarioData.newBuilder()
                .putAllScenarioData(scenarioMap)
                .build()
        }
    }
}
