package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import NMatrix.NApi.Delivery
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.megamind.protos.common.Atm
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.INTERFACE_TO_REQUIRED_FEATURES
import ru.yandex.alice.social.sharing.apphost.handlers.*
import ru.yandex.alice.social.sharing.document.ExternalSkillDocumentParams
import ru.yandex.alice.social.sharing.document.StatelessDocumentContext
import ru.yandex.alice.social.sharing.jsonBodyParser
import ru.yandex.alice.social.sharing.proto.WebApi
import ru.yandex.alice.social.sharing.proto.WebPageProto
import ru.yandex.alice.social.sharing.proto.context.ListDevicesProto
import ru.yandex.alice.social.sharing.proto.context.SendToDevice
import ru.yandex.alice.social.sharing.proto.context.StatelessDocument
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class PrepareStatelessDocumentWebPageData(
    private val objectMapper: ObjectMapper,
) : ApphostHandler {
    override val path = "/stateless_document/document_from_request"

    private fun buildWebPageData(
        params: ExternalSkillDocumentParams,
        skill: SkillInfo,
        avatarsImage: StatelessDocument.TAvatarsImage,
        devices: List<WebApi.TWebPageTemplateData.TDevice>,
        activatedDevice: WebApi.TWebPageTemplateData.TDevice?,
        sendPushStatus: WebPageProto.TExternalSkillTemplate.ESendPushStatus,
    ): WebApi.TWebPageTemplateData {
        val frame = FrameProto.TTypedSemanticFrame.newBuilder()
            .setExternalSkillFixedActivateSemanticFrame(
                FrameProto.TExternalSkillFixedActivateSemanticFrame.newBuilder()
                    .setFixedSkillId(
                        FrameProto.TStringSlot.newBuilder()
                            .setStringValue(skill.id)
                    )
                    .setPayload(
                        FrameProto.TStringSlot.newBuilder()
                            .setStringValue(params.payload ?: "")
                    )
            ).build()
        val builder = WebApi.TWebPageTemplateData.newBuilder()
            .setDocument(
                WebPageProto.TScenarioSharePage.newBuilder()
                    .setSocialNetworkMarkup(
                        WebPageProto.TScenarioSharePage.TSocialNetworkMarkup.newBuilder()
                            .setTitle("${params.titleText} в навыке Алисы «${skill.name}»")
                            .setType("article")
                            .setDescription(params.subtitleText ?: "")
                            .setImage(
                                WebPageProto.TScenarioSharePage.TSocialNetworkMarkup.TImage.newBuilder()
                                    .setUrl(avatarsImage.url)
                            )
                    )
                    .setExternalSkill(
                        WebPageProto.TExternalSkillTemplate.newBuilder()
                            .setSkill(
                                WebPageProto.TExternalSkillTemplate.TSkill.newBuilder()
                                    .setId(skill.id)
                                    .setName(skill.name)
                                    .setRating(skill.averageRating)
                                    .setLogoAvatarsId(skill.logo.avatarId)
                            )
                            .setActionButton(
                                WebPageProto.TExternalSkillTemplate.TActionButton.newBuilder()
                                    .setText(params.buttonText ?: "")
                                    .setBackgroundColor("#FF5E1A")
                                    .setTextColor("#FFFFFF")
                            )
                            .setPageContent(
                                WebPageProto.TExternalSkillTemplate.TPageContent.newBuilder()
                                    .setBackgroundColor("#FFFFFF")
                                    .setCardColor("#F2F3F5")
                                    .setTitle(params.titleText)
                                    .setSubtitle(params.subtitleText ?: "")
                                    .setImage(
                                        WebPageProto.TExternalSkillTemplate.TImage.newBuilder()
                                            .setUrl(avatarsImage.url)
                                            .setAvatarsId(avatarsImage.avatarsId)
                                    )
                            )
                            .setSendPushStatus(sendPushStatus)
                    )
                    .setFrame(frame)
                    // TODO: remove deprecated setAction
                    .setAction(
                        FrameProto.TSemanticFrameRequestData.newBuilder()
                            .setAnalytics(
                                Atm.TAnalyticsTrackingModule.newBuilder()
                                    .setOrigin(Atm.TAnalyticsTrackingModule.EOrigin.Web)
                                    .setOriginInfo("social_sharing")
                                    .setPurpose("start_scenario")
                            )
                            .setTypedSemanticFrame(frame)
                    )
                    .addAllRequiredFeatures(
                        params.requiredInterfaces
                            .flatMap { INTERFACE_TO_REQUIRED_FEATURES[it] ?: emptyList() }
                            .toSet()
                    )
            )
            .addAllDevices(devices)
        if (activatedDevice != null) {
            builder.setActivatedDevice(activatedDevice)
        }
        return builder.build()
    }

    override fun handle(context: RequestContext) {
        val avatarsImage = context
            .getSingleRequestItem(AVATARS_IMAGE)
            .getProtobufData(StatelessDocument.TAvatarsImage.getDefaultInstance())
        val skill = parseHttpResponse(
            context,
            "get_skill_info_http_response",
            objectMapper.jsonBodyParser(SkillInfo::class.java)
        )
        val devices = context
            .getSingleRequestItemO(LIST_DEVICES_RESPONSE)
            .map { it.getProtobufData(ListDevicesProto.TListDevicesResponse.getDefaultInstance()).devicesList }
            .orElse(emptyList())
        val params = StatelessDocumentContext.fromContext(context).urlParams
        val sendPushResponse = context.getSingleRequestItemO(SEND_PUSH_RESPONSE)
            .map { item -> item.getProtobufData(SendToDevice.TSendPushResponse.getDefaultInstance()) }
            .orElse(null)
        val sendPushResponseOk = sendPushResponse?.notificatorResponse?.code == Delivery.TDeliveryResponse.EResponseCode.OK
        val sendPushRequest = context.getSingleRequestItemO(SEND_PUSH_REQUEST)
            .map { item -> item.getProtobufData(SendToDevice.TSendPushRequest.getDefaultInstance()) }
            .orElse(null)
        val activatedDevice = if (params.autostart && sendPushResponseOk)
            devices.first { d -> d.deviceId == sendPushRequest.deviceId }
            else null
        val sendPushStatus = if (activatedDevice != null)
            WebPageProto.TExternalSkillTemplate.ESendPushStatus.SENT
            else WebPageProto.TExternalSkillTemplate.ESendPushStatus.NOT_SENT
        val webPageData = buildWebPageData(params, skill, avatarsImage, devices, activatedDevice, sendPushStatus)
        logger.info("Web page data: {}", webPageData)
        context.addProtobufItem(WEB_PAGE_DATA, webPageData)
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
