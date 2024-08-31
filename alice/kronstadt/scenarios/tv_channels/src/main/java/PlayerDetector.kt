package ru.yandex.alice.kronstadt.scenarios.tv_channels

import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response.PlayerContent

object PlayerDetector {
    fun forCardDetail(playerContent: PlayerContent): String {
        if (playerContent.contentTypeName in listOf("channel", "episode")) {
            return "vh"
        }
        // опасное место, логика тут не повторяет в точности
        // https://a.yandex-team.ru/arc_vcs/smarttv/droideka/proxy/player_detection.py?rev=ec5bf16aee83cacb407938df97836a2a8cb01249#L167
        return "web"
    }
}
