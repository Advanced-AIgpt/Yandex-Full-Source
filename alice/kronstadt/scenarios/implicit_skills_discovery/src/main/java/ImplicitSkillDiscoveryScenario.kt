package ru.yandex.alice.kronstadt.scenarios.discovery.implicit

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse
import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.MegaMindRequestListener
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.paskill.dialogovo.domain.Experiments.VOICE_DISCOVERY_SUGGEST
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey
import ru.yandex.alice.paskill.dialogovo.scenarios.discovery.SkillRecommendationMapper

private const val PUT_MONEY_ON_PHONE_FRAME_NAME = "alice.put_money_on_phone"

@Component
open class ImplicitSkillDiscoveryScenario(
    private val implicitSkillDiscoveryStationScene: ImplicitSkillDiscoveryStationScene,
    @Qualifier("dialogovoMegaMindRequestListener") dialogovoMegaMindRequestListener: MegaMindRequestListener,
    private val skillRecommendationMapper: SkillRecommendationMapper
) : AbstractNoStateScenario(
    ScenarioMeta("implicitDiscovery", "ImplicitSkillsDiscovery", "dialogovo"),
    listOf(dialogovoMegaMindRequestListener)
) {

    override val irrelevantResponseFactory: IrrelevantResponse.Factory<Any> =
        DefaultIrrelevantResponse.Factory("implicit_skill_discovery.irrelevant")

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        if (request.dialogId != null) return@handle null

        onFrame(PUT_MONEY_ON_PHONE_FRAME_NAME) {
            onExperiment(VOICE_DISCOVERY_SUGGEST) {
                onClient(ClientInfo::isYaSmartDevice) {
                    implicitSkillDiscoveryStationScene.getImplicitRecommendationSkills(
                        request,
                        SkillTagsKey.TOP_UP_PHONE_PROVIDER
                    ).firstOrNull()
                        ?.let { skillInfo ->
                            sceneWithArgs(
                                ImplicitSkillDiscoveryStationScene::class,
                                skillInfo
                            )
                        }
                }
            }
            onClient(ClientInfo::supportsDivCards) {
                val implicitRecommendations =
                    GcWithDivCardsImplicitSkillsDiscoveryScene.Args(
                        skillRecommendationMapper.mapToSkillRecommendationList(
                            implicitSkillDiscoveryStationScene.getImplicitRecommendationSkills(
                                request,
                                SkillTagsKey.TOP_UP_PHONE_PROVIDER
                            ),
                            request
                        )
                    )
                onCondition({ implicitRecommendations.items.isNotEmpty() }) {
                    sceneWithArgs(GcWithDivCardsImplicitSkillsDiscoveryScene::class, implicitRecommendations)
                }
            }
        }
    }
}
