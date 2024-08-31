package ru.yandex.alice.paskill.dialogovo.processor.discovery.activation.converter

import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey

// Пока решено не поддерживать позицию(begin, end) слота
const val UNKNOWN_POSITION_VALUE = -1

interface ProvidableToSkillFrameConverter {
    fun canConvert(frame: TTypedSemanticFrame): Boolean
    fun toIntent(typedSemanticFrame: TTypedSemanticFrame): Intent
    // Frame will be provided only to skills with this tag
    val skillTag: SkillTagsKey
}
