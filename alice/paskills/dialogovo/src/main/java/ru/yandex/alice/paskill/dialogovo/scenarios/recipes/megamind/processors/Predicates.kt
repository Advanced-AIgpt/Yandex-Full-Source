package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import java.util.function.Predicate

class Predicates private constructor() {
    init {
        throw UnsupportedOperationException()
    }

    companion object {
        // use interfaces to check capabilities
        @JvmField
        val SURFACE_IS_SUPPORTED = Predicate { req: MegaMindRequest<DialogovoState> ->
            val clientInfo = req.clientInfo
            val isSmartSpeaker = clientInfo.isSmartSpeaker
            val isSearchAppWithFlag = (clientInfo.isSearchApp || clientInfo.isYaBrowser) &&
                req.hasExperiment(Experiments.RECIPES_ENABLE_ON_SEARCHAPP_AND_BROWSER)
            isSmartSpeaker || isSearchAppWithFlag
        }

        @JvmField
        val SUPPORTS_NATIVE_TIMERS: Predicate<ClientInfo> = Predicate { clientInfo -> clientInfo.isSmartSpeaker }

        // TODO: check for plus subscription
        @JvmField
        val SUPPORTS_MUSIC: Predicate<ClientInfo> = Predicate { clientInfo -> clientInfo.isSmartSpeaker }

        @JvmField
        val RENDER_INGREDIENTS_AS_LIST: Predicate<ClientInfo> =
            Predicate { clientInfo: ClientInfo -> clientInfo.isSearchApp || clientInfo.isYaBrowser }
    }
}
