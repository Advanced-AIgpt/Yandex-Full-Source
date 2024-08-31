package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

object UnmuteMicAction: AnalyticsInfoAction(
    "call.mute_mic",
    "mute mic in telegram call",
    "Выключение микрофона в телеграм звонке"
)