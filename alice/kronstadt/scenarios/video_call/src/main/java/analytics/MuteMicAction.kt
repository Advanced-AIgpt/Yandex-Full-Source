package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

object MuteMicAction: AnalyticsInfoAction(
    "call.unmute_mic",
    "unmute mic in telegram call",
    "Включение микрофона в телеграм звонке"
)