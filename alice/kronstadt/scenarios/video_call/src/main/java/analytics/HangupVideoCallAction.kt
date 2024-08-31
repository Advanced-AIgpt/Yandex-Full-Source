package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

object HangupVideoCallAction: AnalyticsInfoAction(
    "call.hangup_video_call",
    "hangup call from telegram",
    "Завершается звонок из телеграма"
)