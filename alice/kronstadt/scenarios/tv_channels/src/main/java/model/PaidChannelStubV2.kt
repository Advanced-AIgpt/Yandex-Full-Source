package ru.yandex.alice.kronstadt.scenarios.tv_channels.model

data class PaidChannelStubV2(
    val header: String,
    val text: String,
    val deeplink: String,
    val background: String,
    val placeholders: Map<String, String>,
)
