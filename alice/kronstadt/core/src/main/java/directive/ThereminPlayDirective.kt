package ru.yandex.alice.kronstadt.core.directive

interface ThereminPlayDirective : MegaMindDirective {
    fun modeId(): String
    fun isInternal(): Boolean
}
