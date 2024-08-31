package ru.yandex.alice.paskill.dialogovo.config

interface EndpointConfig {
    val url: String
    val timeout: Int
    val connectTimeout: Int
}
