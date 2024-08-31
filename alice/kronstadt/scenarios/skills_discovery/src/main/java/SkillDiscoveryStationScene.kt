package ru.yandex.alice.kronstadt.scenarios.skills_discovery

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Button.Companion.simpleTextWithTextDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSuggestVoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderCardName
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderRequestAttributes
import ru.yandex.alice.paskill.dialogovo.service.recommender.RecommenderService

@Component
open class SkillDiscoveryStationScene(
    private val skillProvider: SkillProvider,
    private val recommenderService: RecommenderService,
    private val phrases: Phrases,
    private val voiceButtonFactory: SkillsSuggestVoiceButtonFactory,
    private val surfaceChecker: SurfaceChecker,
) : AbstractScene<Any, SkillInfo>("EXTERNAL_SKILL_DISCOVERY_STATION", SkillInfo::class) {

    private val logger = LogManager.getLogger()

    private val RECOMMENDER_EXP_PREFIX = "skill_recommendation_card_discovery_threshold__discovery_bass_search__"

    override fun render(request: MegaMindRequest<Any>, skillInfo: SkillInfo): RelevantResponse<Any> {

        val nlg = phrases.getRandomTextWithTts(
            "station.discovery.suggest",
            request.random,
            TextWithTts(skillInfo.name, skillInfo.getNameTts())
        )
        return RunOnlyResponse(
            layout = Layout.textLayout(
                text = nlg.text,
                outputSpeech = nlg.tts,
                shouldListen = true,
                suggests = listOf(
                    simpleTextWithTextDirective(phrases["suggest.confirm.yes"]),
                    simpleTextWithTextDirective(phrases["suggest.confirm.no"])
                )
            ),
            state = null,
            analyticsInfo = analyticsInfo(intent = "external_skill_discovery.suggest") {
                action(skillSuggestAction(skillInfo))
                obj(SkillAnalyticsInfoObject(skillInfo))
            },
            actions = mapOf(
                "confirm" to voiceButtonFactory.createConfirmSkillActivationVoiceButton(
                    skillInfo,
                    ActivationSourceType.DISCOVERY
                ),
                "decline" to voiceButtonFactory.createDeclineDoNothingButton()
            )
        )
    }

    fun searchSkills(
        activationPhrase: String,
        request: MegaMindRequest<Any>
    ): List<SkillInfo> = recommenderService.search(
        RecommenderCardName.DISCOVERY_BASS_SEARCH,
        activationPhrase,
        convertRequestToRecommenderAttributes(request)
    )
        .items
        .mapNotNull { item -> skillProvider.getSkill(item.skillId).orElse(null) }
        .filterValidForRecommendations(request)

    private fun List<SkillInfo>.filterValidForRecommendations(request: MegaMindRequest<Any>) =
        filter { skillInfo ->
            SkillFilters.VALID_FOR_RECOMMENDATIONS.test(skillInfo) &&
                surfaceChecker.isSkillSupported(request.clientInfo, skillInfo.surfaces) &&
                SkillFilters.EXPLICIT_CONTENT_RECOMMENDATION_FILTER.test(request, skillInfo)
        }

    private fun skillSuggestAction(skill: SkillInfo) = AnalyticsInfoAction(
        "external_skill_discovery.suggest",
        "external_skill_discovery.suggest",
        "Предложение запустить навык «${skill.name}»"
    )

    private fun convertRequestToRecommenderAttributes(request: MegaMindRequest<Any>): RecommenderRequestAttributes {
        val experiments = request.experiments.toMutableSet()

        request.getExperimentStartWith(Experiments.VOICE_DISCOVERY_SUGGEST_THRESHOLD_PREFIX)
            ?.let { extractThreshold(it) }
            ?.let { voice_discovery_threshold_value ->

                experiments.add(RECOMMENDER_EXP_PREFIX + voice_discovery_threshold_value)
            }
        return RecommenderRequestAttributes(experiments, null)
    }

    private fun extractThreshold(voiceDiscoveryThresholdExperiment: String): Long? {
        val pos = voiceDiscoveryThresholdExperiment.indexOf("=")
        return if (pos == -1) {
            null
        } else try {
            voiceDiscoveryThresholdExperiment.substring(pos + 1).toLong()
        } catch (e: Exception) {
            logger.warn(
                "Cannot extract discovery bass search threshold value from experiment value = [{}]",
                voiceDiscoveryThresholdExperiment
            )
            null
        }
    }
}
