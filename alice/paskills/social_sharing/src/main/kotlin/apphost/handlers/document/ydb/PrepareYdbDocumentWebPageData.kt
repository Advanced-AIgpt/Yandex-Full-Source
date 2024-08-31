package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.DOCUMENT
import ru.yandex.alice.social.sharing.apphost.handlers.LIST_DEVICES_RESPONSE
import ru.yandex.alice.social.sharing.apphost.handlers.WEB_PAGE_DATA
import ru.yandex.alice.social.sharing.proto.WebApi
import ru.yandex.alice.social.sharing.proto.context.FetchDocumentProto
import ru.yandex.alice.social.sharing.proto.context.ListDevicesProto
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class PrepareYdbDocumentWebPageData: ApphostHandler {

    override val path = "/document/prepare_web_page_data"

    override fun handle(context: RequestContext) {
        val document = context.getSingleRequestItem(DOCUMENT)
            .getProtobufData(FetchDocumentProto.TDocument.getDefaultInstance())
        val devices = context.getSingleRequestItemO(LIST_DEVICES_RESPONSE)
            .map { it.getProtobufData(ListDevicesProto.TListDevicesResponse.getDefaultInstance()) }
            .orElse(ListDevicesProto.TListDevicesResponse.getDefaultInstance())
        val webPageData = WebApi.TWebPageTemplateData.newBuilder()
            .setDocument(document.scenarioSharePage)
            .addAllDevices(devices.devicesList)
        context.addProtobufItem(WEB_PAGE_DATA, webPageData.build())
    }


}
