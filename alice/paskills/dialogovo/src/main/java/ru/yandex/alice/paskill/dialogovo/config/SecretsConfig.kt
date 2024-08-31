package ru.yandex.alice.paskill.dialogovo.config

data class SecretsConfig(
    val pgDatabaseMultiHost: String,
    val pgDatabaseName: String,
    val pgUser: String,
    val pgPassword: String,
    val tvmToken: String,
    val xivaToken: String,
    val developerTrustedToken: String,
    val appMetricaEncryptionSecret: String,
    val ydbEndpoint: String,
    val ydbDatabase: String,
    val ytToken: String?,
)
