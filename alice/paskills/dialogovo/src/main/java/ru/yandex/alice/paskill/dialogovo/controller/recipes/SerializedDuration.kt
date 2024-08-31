package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.DurationToString
import java.time.Duration

internal data class SerializedDuration(val components: List<SerializedComponent>) {

    companion object {
        internal fun fromDuration(duration: Duration) = SerializedDuration(
            DurationToString.toComponents(duration, true)
                .map { component: DurationToString.Component -> SerializedComponent.fromComponent(component) }
        )
    }

    internal data class SerializedComponent internal constructor(val value: Int, val text: String) {

        companion object {
            internal fun fromComponent(component: DurationToString.Component) =
                SerializedComponent(value = component.value, text = component.pluralForm.text)
        }
    }
}
