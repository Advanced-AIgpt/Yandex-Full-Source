package ru.yandex.alice.kronstadt.core.directive

import com.fasterxml.jackson.annotation.JsonInclude

@Directive("mordovia_callback_directive")
@JsonInclude(JsonInclude.Include.NON_EMPTY)
data class ModroviaCallbackDirective(
    val command: String,
    val meta: Map<String, *> = mapOf<String, Any>()
) : CallbackDirective {
    companion object {
        val PROTOTYPE = ModroviaCallbackDirective("")
    }
}
