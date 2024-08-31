package ru.yandex.alice.memento.settings

import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import com.google.protobuf.Any as ProtoAny

interface SettingsStorage {
    fun getSettings(request: SettingsRequest): Settings
    fun getAllObjects(uid: String, surfaces: Set<String>, anonymous: Boolean): AllSettings
    fun updateUserSettings(userId: String, keysToChange: Map<EConfigKey, ProtoAny>, anonymous: Boolean = false) {
        updateUserSettings(userId, Settings(keysToChange, emptyMap(), emptyMap(), emptyMap()), anonymous)
    }

    fun updateUserSettings(userId: String, settings: Settings, anonymous: Boolean)
    fun removeAllObjects(uid: String)
}
