package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio

import ru.yandex.alice.kronstadt.core.Features
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.PlayerFeatures
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import java.time.Duration
import kotlin.math.max

internal fun MegaMindRequest<DialogovoState>.capturePlayer(): Features {
    return Features(
        PlayerFeatures(
            true,
            max(
                0,
                this.audioPlayerLastPlayTimestamp?.let { lastPlayTs ->
                    Duration.between(
                        lastPlayTs,
                        this.serverTime
                    ).seconds
                } ?: 0L
            )
        )
    )
}
