package ru.yandex.alice.kronstadt.scenarios.tv_channels

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.scenarios.tv_channels.model.PaidChannelStub
import ru.yandex.alice.kronstadt.scenarios.tv_channels.model.PaidChannelStubV2
import ru.yandex.alice.kronstadt.scenarios.tv_channels.model.ThinCardDetail
import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.VhClient
import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response.OttParams

@Component
class CheckLicenseService(private val vhClient: VhClient) {

    fun getContentDetail(contentId: String): ThinCardDetail {

        val data = vhClient.contentDetail(contentId)

        return data.content.run {
            ThinCardDetail(
                title = title,
                years = years,
                genres = genres?.takeIf { it.isNotEmpty() }?.joinToString(", "),
                duration = duration.takeIf { it != 0 },  // а можно не выводить ноль в рендере просто?
                series = series,
                yaPlus = yaPlus,
                ottParams = ottParams,
                contentId = contentId,
                releaseYear = releaseYear,
                restrictionAge = restrictionAge,
                contentType = contentTypeName,
                monetizationModel = ottParams?.monetizationModel,
                thumbnail = thumbnail?.takeIf { it.isNotEmpty() }?.let { fixSchema(it) },
                poster = ontoPoster?.takeIf { it.isNotEmpty() }?.let { fixSchema(it) },
                playerId = PlayerDetector.forCardDetail(this),
                paidChannelStub = preparePaidChannelStub(),
                paidChannelStubV2 = preparePaidChannelStubV2(ottParams),
            )
        }
    }

    private fun preparePaidChannelStubV2(ottParams: OttParams?): PaidChannelStubV2? {
        // заглушка платного канала с рекламой нужной подписки (если нужна)
        return PaidChannelStubBuilder.getStub(ottParams)
    }

    private fun preparePaidChannelStub(): PaidChannelStub {
        return PaidChannelStub(
            bodyText = listOf(
                "Тысячи фильмов и музыка\nбез рекламы и ограничений",
                "Кэшбэк баллами, которые\nможно тратить на поездки\nи не только",
                "Множество преимуществ\nна сервисах Яндекса"
            ),
            headerImage = "https://androidtv.s3.yandex.net/other/yandex_plus_monochrome.png"
        )
    }

    private fun fixSchema(url: String): String {
        if (url.startsWith("//")) {
            return "https:$url"
        }
        return url
    }
}


