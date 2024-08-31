package ru.yandex.alice.kronstadt.core.scenario

import ru.yandex.alice.kronstadt.core.MegaMindRequest

interface MegaMindRequestListener {

    fun onPrepareSelectScene(request: MegaMindRequest<*>) {
    }

    fun onSelectScene(request: MegaMindRequest<*>) {
    }

    fun onPrepareRun(request: MegaMindRequest<*>) {
    }

    fun onRun(request: MegaMindRequest<*>) {
    }

    fun onPrepareApply(request: MegaMindRequest<*>) {
    }

    fun onApply(request: MegaMindRequest<*>) {
    }

    fun onPrepareCommit(request: MegaMindRequest<*>) {
    }

    fun onCommit(request: MegaMindRequest<*>) {
    }

    fun onPrepareContinue(request: MegaMindRequest<*>) {
    }

    fun onContinue(request: MegaMindRequest<*>) {
    }
}
