package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

import ru.yandex.alice.kronstadt.core.ActionSpace
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData
import ru.yandex.alice.kronstadt.core.semanticframes.SkillSessionRequest
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames

object SpaceActionUtils {
    const val DIALOG_ACTION_SPACE_ID = "skill_dialog_layer"

    private const val skillRequestActionId = "skill_request_action"
    private const val productScenario = "Dialogovo"

    @JvmStatic
    fun getModalSkillSessionSpaceAction() : ActionSpace {
        val wildCardHint = ActionSpace.NluHint(
            skillRequestActionId,
            SemanticFrames.EXTERNAL_SKILL_WILDCARD
        )
        val effect = ActionSpace.TypedSemanticFrameEffect(
            SemanticFrameRequestData(
                SkillSessionRequest,
                SemanticFrameRequestData.AnalyticsTrackingModule(
                    productScenario,
                    SkillSessionRequest.defaultPurpose()
                )
            )
        )
        return ActionSpace(mapOf(Pair(skillRequestActionId, effect)), listOf(wildCardHint))
    }
}
