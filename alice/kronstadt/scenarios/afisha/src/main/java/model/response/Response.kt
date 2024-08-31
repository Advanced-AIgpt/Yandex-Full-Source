package ru.yandex.alice.kronstadt.scenarios.afisha.model.response

data class Response(
    val status: Int,
    val data: Map<String, Items>?,
    val errors: List<Error> = emptyList()
)

data class Items(
    val items: List<AfishaEvent>
)

data class AfishaEvent(
    val event: Event,
    val scheduleInfo: ScheduleInfo?
)
