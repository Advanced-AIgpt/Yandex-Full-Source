package ru.yandex.alice.kronstadt.core.directive

class InternalThereminPlayDirective(val mode: Int) : ThereminPlayDirective {
    override fun modeId(): String {
        return mode.toString()
    }

    override fun isInternal(): Boolean {
        return true
    }
}
