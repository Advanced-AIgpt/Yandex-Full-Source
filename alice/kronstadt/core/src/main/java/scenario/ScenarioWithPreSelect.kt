package ru.yandex.alice.kronstadt.core.scenario

import ru.yandex.alice.kronstadt.core.MegaMindRequest

interface ScenarioWithPreSelect<State> {
    fun preSelect(request: MegaMindRequest<State>): Any
}
