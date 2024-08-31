package ru.yandex.alice.kronstadt.scenarios.skills_discovery

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

@Component
class SkillDiscoveryScenario(
    private val skillDiscoveryStationScene: SkillDiscoveryStationScene,
    private val gcWithDivCardsScene: GcWithDivCardsScene,
    @Qualifier("dialogovoMegaMindRequestListener") dialogovoMegaMindRequestListener: MegaMindRequestListener,
) : AbstractNoStateScenario(
    ScenarioMeta("discovery", "SkillsDiscovery", "dialogovo"),
    listOf(dialogovoMegaMindRequestListener)
) {

    override val irrelevantResponseFactory: IrrelevantResponse.Factory<Any> =
        DefaultIrrelevantResponse.Factory("external_skill_discovery.irrelevant")

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        if (request.dialogId != null) return@handle null

        onExperiment(VOICE_DISCOVERY_SUGGEST) {
            onClient(ClientInfo::isYaSmartDevice) {
                onFrameWithSlot("alice.external_skill_discovery", ACTIVATION_PHRASE_SLOT) { _, slot ->
                    skillDiscoveryStationScene.searchSkills(slot.value, request).firstOrNull()
                        ?.let { skillInfo -> sceneWithArgs(SkillDiscoveryStationScene::class, skillInfo) }
                }
            }
        }

        onFrame("alice.external_skill_discovery.gc") {
            onClient(ClientInfo::supportsDivCards) {
                onCondition({ !skillDiscoveryGcCandidates?.candidates.isNullOrEmpty() }) {
                    val recommendations =
                        GcWithDivCardsScene.Args(
                            gcWithDivCardsScene.filterCandidates(
                                skillDiscoveryGcCandidates?.candidates!!,
                                request
                            )
                        )
                    onCondition({ recommendations.items.isNotEmpty() }) {
                        sceneWithArgs(GcWithDivCardsScene::class, recommendations)
                    }
                }
            }
        }
    }

    companion object {
        private const val ACTIVATION_PHRASE_SLOT = "activation_phrase"
    }
}
