package ru.yandex.alice.kronstadt.scenarios.afisha.model.request

data class QueryRequest(
    val query: String,
    val variables: Map<String, Any>,
    val lat: Double?,
    val lon: Double?,
)
