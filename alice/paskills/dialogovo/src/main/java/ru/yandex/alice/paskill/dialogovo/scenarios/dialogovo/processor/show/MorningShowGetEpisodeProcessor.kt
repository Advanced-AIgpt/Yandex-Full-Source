package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.show

import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.ContinueNeededResponse
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ContinuingRunProcessor
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames.ALICE_SHOW_GET_SKILL_EPISODE
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.CollectSkillShowEpisodeApplyArguments
import ru.yandex.alice.paskill.dialogovo.service.show.ShowService

@Component
@ConditionalOnProperty(name = ["scheduler.controllers.enable"], havingValue = "true")
class MorningShowGetEpisodeProcessor(
    private val showService: ShowService
) : ContinuingRunProcessor<DialogovoState, CollectSkillShowEpisodeApplyArguments> {

    private val nothingNewTextWithTts =
        TextWithTts("Ничего нового от навыка сегодня", "Ничего нового от навыка сегодня")

    private val defaultIrrelevantResponseFactory =
        DefaultIrrelevantResponse.Factory<DialogovoState>(ALICE_SHOW_GET_SKILL_EPISODE)

    override val applyArgsType = CollectSkillShowEpisodeApplyArguments::class.java
    override val type: ProcessorType = RunRequestProcessorType.ALICE_SHOW_SKILL_EPISODE_COLLECTOR

    override fun processContinue(
        request: MegaMindRequest<DialogovoState>,
        context: Context,
        applyArguments: CollectSkillShowEpisodeApplyArguments
    ): ScenarioResponseBody<DialogovoState> {
        val skillId = applyArguments.skillId
        val showEpisodeEntityO = showService.getRelevantEpisode(ShowType.MORNING, skillId, request.clientInfo)

        return ScenarioResponseBody(
            layout = Layout.textWithOutputSpeech(
                if (showEpisodeEntityO.isPresent) TextWithTts(
                    showEpisodeEntityO.get().text, showEpisodeEntityO.get().tts
                )
                else nothingNewTextWithTts
            ),
            state = null,
            analyticsInfo = AnalyticsInfo("get_skill_episode")
        )
    }

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean =
        hasFrame(ALICE_SHOW_GET_SKILL_EPISODE).test(request)

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val skillId = request.getSemanticFrame(ALICE_SHOW_GET_SKILL_EPISODE)
            ?.typedSemanticFrame?.externalSkillEpisodeForShowRequestSemanticFrame?.skillId?.stringValue
            ?: return defaultIrrelevantResponseFactory.create(request)
        return ContinueNeededResponse(
            CollectSkillShowEpisodeApplyArguments(skillId, ShowType.MORNING)
        )
    }
}
