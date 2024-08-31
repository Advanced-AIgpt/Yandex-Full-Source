package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor

import ru.yandex.metrika.appmetrica.proto.AppMetricaProto

data class AppmetricaCommitArgs(
    val apiKeyEncrypted: String,
    val uri: String,
    val eventEpochTime: Long,
    val reportMessage: AppMetricaProto.ReportMessage
)
