package ru.yandex.alice.paskill.dialogovo.vins

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerError

data class AudioPlayerFailedVinsEvent(val error: AudioPlayerError, val type: InputType)
