package ru.yandex.alice.kronstadt.core

class IrrelevantResponseException(val scenarioResponseBody: ScenarioResponseBody<*>) : RuntimeException() {

}
