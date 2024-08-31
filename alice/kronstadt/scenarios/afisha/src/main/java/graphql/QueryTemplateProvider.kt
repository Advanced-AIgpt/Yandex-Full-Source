package ru.yandex.alice.kronstadt.scenarios.afisha.graphql

interface QueryTemplateProvider {
    fun getQuery(eventType: EventType): String
}
