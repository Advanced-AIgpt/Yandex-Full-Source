package ru.yandex.alice.paskill.dialogovo.processor.discovery.activation.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NluEntity
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.NumberNluEntity
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.StringEntity
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey

private const val INTENT_NAME = "YANDEX.FINANCE.TOP_UP_MOBILE_PHONE"
private const val AMOUNT_SLOT_NAME = "amount"
private const val PHONE_NUMBER_SLOT_NAME = "phone_number"

@Component
class PutMoneyOnPhoneFrameConverter : ProvidableToSkillFrameConverter {

    override fun toIntent(typedSemanticFrame: FrameProto.TTypedSemanticFrame): Intent {
        val putMoneyOnPhoneSemanticFrame = typedSemanticFrame.putMoneyOnPhoneSemanticFrame
        val slotsMap = mutableMapOf<String, NluEntity>()
        if (putMoneyOnPhoneSemanticFrame.hasAmount())
            slotsMap[AMOUNT_SLOT_NAME] = NumberNluEntity(
                UNKNOWN_POSITION_VALUE,
                UNKNOWN_POSITION_VALUE,
                putMoneyOnPhoneSemanticFrame.amount.numValue.toBigDecimal()
            )
        if (putMoneyOnPhoneSemanticFrame.hasPhoneNumber())
            slotsMap[PHONE_NUMBER_SLOT_NAME] = StringEntity(
                UNKNOWN_POSITION_VALUE,
                UNKNOWN_POSITION_VALUE,
                putMoneyOnPhoneSemanticFrame.phoneNumber.stringValue
            )
        return Intent(INTENT_NAME, slotsMap)
    }

    override fun canConvert(frame: FrameProto.TTypedSemanticFrame) = frame.hasPutMoneyOnPhoneSemanticFrame()

    override val skillTag: SkillTagsKey = SkillTagsKey.TOP_UP_PHONE_PROVIDER
}
