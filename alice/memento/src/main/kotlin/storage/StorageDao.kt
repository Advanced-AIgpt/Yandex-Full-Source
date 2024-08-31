package ru.yandex.alice.memento.storage

interface StorageDao {
    fun fetch(uid: String, keysToFetch: KeysToFetch, anonymous: Boolean): StoredData
    fun fetchAll(uid: String, surfaces: Set<String>, anonymous: Boolean): StoredData
    fun update(userId: String, storedData: StoredData, anonymous: Boolean)
    fun removeAllObjects(uid: String)
}
