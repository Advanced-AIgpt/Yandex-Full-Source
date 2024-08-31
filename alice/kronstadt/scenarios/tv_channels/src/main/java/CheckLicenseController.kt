package ru.yandex.alice.kronstadt.scenarios.tv_channels

import org.springframework.web.bind.annotation.GetMapping
import org.springframework.web.bind.annotation.RequestParam
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.kronstadt.scenarios.tv_channels.model.ThinCardDetail

@RestController
class CheckLicenseController(
    private val checkLicenseService: CheckLicenseService
) {

    @GetMapping("/kronstadt/scenario/tv_channels/thin_card_detail")
    fun thinCardDetail(
        @RequestParam("content_id") contentId: String,
    ): ThinCardDetail = checkLicenseService.getContentDetail(contentId)
}
