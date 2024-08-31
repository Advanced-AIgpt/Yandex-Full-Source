package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.paskill.dialogovo.megamind.AbstractProcessorBasedScenario
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoApplyArgumentsConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoMegaMindRequestListener
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoStateConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.Scenarios
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.DialogovoMegaMindScenarioConfiguration.ProcessorListPrototype

@Component
internal class DialogovoScenario(
    @Qualifier("baseDialogovoProcessors") baseProcessors: ProcessorListPrototype,
    dialogovoStateConverter: DialogovoStateConverter,
    applyArgumentsConverter: DialogovoApplyArgumentsConverter,
    listener: DialogovoMegaMindRequestListener,
) : AbstractProcessorBasedScenario<DialogovoState>(
    scenarioMeta = Scenarios.DIALOGOVO,
    runRequestProcessors = baseProcessors.processors.filter { processor -> processor.javaClass !in classesToFilterOut },
    stateConverter = dialogovoStateConverter,
    applyArgumentsConverter = applyArgumentsConverter,
    megamindRequestListeners = listOf(listener)
) {

    companion object {
        private val classesToFilterOut: Set<Class<out RunRequestProcessor<DialogovoState>>> =
            setOf(IntentActivateRunProcessor::class.java)
    }
}
