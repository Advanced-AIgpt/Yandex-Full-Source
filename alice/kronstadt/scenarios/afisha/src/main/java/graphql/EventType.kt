package ru.yandex.alice.kronstadt.scenarios.afisha.graphql

enum class EventType(val fileName: String) {
    CONCERT_RECOMMENDATION("concertRecommendation"),
    THEATRE_RECOMMENDATION("theatreRecommendation"),
    ACTUAL_EVENTS("actualEvents")
}

