package ru.yandex.alice.divkit

import com.yandex.div.dsl.context.CardContext
import com.yandex.div.dsl.model.DivExtension
import com.yandex.div.dsl.model.divExtension
import ru.yandex.alice.library.protobufutils.FromMessageConverter
import ru.yandex.alice.protos.api.voicecontrol.TUnstructuredVoiceControl
import ru.yandex.alice.protos.api.voicecontrol.TVoiceControl

@Suppress("UNCHECKED_CAST")
fun CardContext.voiceControlExtension(control: TVoiceControl): DivExtension = divExtension(
    id = "voice_control",
    params = FromMessageConverter.DEFAULT_CONVERTER.convertToMap(control)
        .filterValues { it != null } as Map<String, Any>
)

fun TVoiceControl.toUnstructuredVoiceControl(): TUnstructuredVoiceControl =
    TUnstructuredVoiceControl.newBuilder()
        .addAllActivationConditions(this.activationConditionsList)
        .addAllActions(
            this.actionsList.map { FromMessageConverter.DEFAULT_CONVERTER.convertToStruct(it) }
        )
        .addAllExternalEntitiesDescriptions(
            this.externalEntitiesDescriptionsList.map { FromMessageConverter.DEFAULT_CONVERTER.convertToStruct(it) }
        )
        .build()


