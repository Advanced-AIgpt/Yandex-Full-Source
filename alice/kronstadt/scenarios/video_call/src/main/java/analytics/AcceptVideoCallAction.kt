package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

object AcceptVideoCallAction: AnalyticsInfoAction(
    "call.accept_video_call",
    "accept call from telegram",
    "Принимается звонок из телеграма"
)