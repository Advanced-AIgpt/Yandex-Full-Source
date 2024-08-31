package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.semanticframes.ParsedUtterance

data class StackEngine(
    val actions: List<StackEngineAction>
) {
    constructor(vararg action: StackEngineAction) : this(listOf(*action))
}

sealed class StackEngineAction {
    data class ResetAdd(val effects: List<StackEngineEffect>) : StackEngineAction() {
        constructor(vararg effect: StackEngineEffect) : this(listOf(*effect))
    }

    object NewSession : StackEngineAction()
}

sealed class StackEngineEffect {

    abstract val forceShouldListen: Boolean

    data class CallbackEffect(
        val directive: CallbackDirective,
        override val forceShouldListen: Boolean = false,
    ) : StackEngineEffect()

    data class ParsedUtteranceEffect(
        val parsedUtterance: ParsedUtterance,
        override val forceShouldListen: Boolean = false,
    ) : StackEngineEffect()
}

@JvmOverloads
fun CallbackDirective.toStackEngineEffect(forceShouldListen: Boolean = false) =
    StackEngineEffect.CallbackEffect(this, forceShouldListen)

// convert callback directive to stack engine
@JvmOverloads
fun CallbackDirective.toStackEngine(forceShouldListen: Boolean = false) =
    StackEngine(
        StackEngineAction.ResetAdd(
            StackEngineEffect.CallbackEffect(this, forceShouldListen)
        )
    )

fun ParsedUtterance.toStackEngineEffect(forceShouldListen: Boolean = false) =
    StackEngineEffect.ParsedUtteranceEffect(this, forceShouldListen)

