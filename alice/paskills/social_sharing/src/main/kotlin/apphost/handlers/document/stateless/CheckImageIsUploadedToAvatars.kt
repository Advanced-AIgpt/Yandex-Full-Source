package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import NAppHostHttp.Http
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.AVATARS_IMAGE
import ru.yandex.alice.social.sharing.document.ImageProvider
import ru.yandex.alice.social.sharing.document.StatelessDocumentContext
import ru.yandex.alice.social.sharing.document.md5Generator
import ru.yandex.alice.social.sharing.proto.context.StatelessDocument
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class CheckImageIsUploadedToAvatars(
        private val imageProvider: ImageProvider,
        @Value("\${avatars.namespace}") private val namespace: String,
        @Value("\${avatars.skill_card.alias}") private val alias: String,
) : ApphostHandler {

    override val path = "/stateless_document/check_image_is_uploaded_to_avatars"

    override fun handle(context: RequestContext) {
        val params = StatelessDocumentContext.fromContext(context).urlParams
        val externalUrl = params.imageUrl
        val image = imageProvider.get(externalUrl)
        if (image == null) {
            val imageName = md5Generator.get(externalUrl)
            val path = UriComponentsBuilder.newInstance()
                .path("/put-$namespace/$imageName")
                .queryParam("url", externalUrl)
            val request: Http.THttpRequest = Http.THttpRequest.newBuilder()
                .setMethod(Http.THttpRequest.EMethod.Get)
                .setPath(path.toUriString())
                .build()
            context.addProtobufItem("avatars_http_request", request)
        } else {
            context.addProtobufItem(
                    AVATARS_IMAGE,
                StatelessDocument.TAvatarsImage.newBuilder()
                    .setUrl(image.avatarsUrl(namespace, alias).toString())
                    .setAvatarsId(image.avatarsId)
                    .build()
            )
        }
    }

}
