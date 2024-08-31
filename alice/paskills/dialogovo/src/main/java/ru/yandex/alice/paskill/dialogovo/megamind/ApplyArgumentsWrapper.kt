package ru.yandex.alice.paskill.dialogovo.megamind

import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType

data class ApplyArgumentsWrapper(
    val processorType: ProcessorType,
    val arguments: ApplyArguments
) : ApplyArguments
