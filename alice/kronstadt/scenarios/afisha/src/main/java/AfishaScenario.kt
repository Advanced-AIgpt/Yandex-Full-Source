package ru.yandex.alice.kronstadt.scenarios.afisha

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.scenarios.afisha.scene.AfishaTeaserPreviewScene

import ru.yandex.alice.kronstadt.scenarios.afisha.scene.AfishaTeaserScene

@Component
class AfishaScenario : AbstractNoStateScenario(ScenarioMeta("afisha", "Afisha", "afisha", useDivRenderer = true)) {
    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        onFrame(ALICE_CENTAUR_COLLECT_CARDS) {
            scene(AfishaTeaserScene::class)
        }
        onFrame(ALICE_CENTAUR_COLLECT_TEASERS_PREVIEW) {
            scene(AfishaTeaserPreviewScene::class)
        }
    }

    companion object {
        private const val ALICE_CENTAUR_COLLECT_CARDS = "alice.centaur.collect_cards"
        private const val ALICE_CENTAUR_COLLECT_TEASERS_PREVIEW = "alice.centaur.collect_teasers_preview"
    }
}


