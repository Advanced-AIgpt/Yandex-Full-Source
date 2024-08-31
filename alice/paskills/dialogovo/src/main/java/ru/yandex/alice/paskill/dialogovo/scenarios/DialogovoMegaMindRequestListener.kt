package ru.yandex.alice.paskill.dialogovo.scenarios

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.scenario.MegaMindRequestListener
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext

@Component
open class DialogovoMegaMindRequestListener(
    private val dialogovoRequestContext: DialogovoRequestContext,
) : MegaMindRequestListener {

    override fun onRun(request: MegaMindRequest<*>) = prepareContext(request)
    override fun onApply(request: MegaMindRequest<*>) = prepareContext(request)
    override fun onCommit(request: MegaMindRequest<*>) = prepareContext(request)

    fun prepareContext(request: MegaMindRequest<*>) {
        dialogovoRequestContext.megaMindRequestContext = DialogovoRequestContext.MegaMindRequestContext.from(request)
        dialogovoRequestContext.scenario = request.scenarioMeta
    }
}
