package ru.yandex.alice.paskill.dialogovo.domain

import org.springframework.web.util.UriComponentsBuilder

object Image {
    @JvmStatic
    fun getImageUrl(imageId: String, namespace: AvatarsNamespace, alias: ImageAlias, avatarsUrl: String): String {
        return UriComponentsBuilder
            .fromHttpUrl(avatarsUrl)
            .pathSegment(
                namespace.urlPath,
                imageId,
                alias.code
            )
            .build()
            .toString()
    }

    @JvmStatic
    fun skillLogoUrl(imageId: String, avatarsUrl: String): String {
        return getImageUrl(imageId, AvatarsNamespace.DIALOGS, ImageAlias.MOBILE_LOGO_X2, avatarsUrl)
    }
}
