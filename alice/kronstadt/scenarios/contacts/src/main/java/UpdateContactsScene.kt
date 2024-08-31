package ru.yandex.alice.kronstadt.scenarios.contacts

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.server.MementoChangeUserObjectsDirective
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.kronstadt.core.directive.server.SurfaceScenarioData
import ru.yandex.alice.kronstadt.core.layout.ContentProperties
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.scenario.ApplyingSceneWithPrepare
import ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.MssngrApiService
import ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain.RESPONSE_STATUS_OK
import ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain.UploadContactsResponse
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.kronstadt.alice.scenarios.contacts.proto.TContactsDeviceData
import ru.yandex.kronstadt.alice.scenarios.contacts.proto.TContactsScenarioData
import ru.yandex.kronstadt.alice.scenarios.contacts.proto.TUploadContactsApplyArgs
import kotlin.reflect.KClass

data class UploadContactsArgs(
    val fullUpload: Boolean
)

const val MSSGR_HTTP_REQUEST_ITEM = "mssngr_api_registry_http_request"
const val MSSGR_HTTP_RESPONSE_ITEM = "mssngr_api_registry_http_response"

@Component
internal class UploadContactsScene(
    private val requestContext: RequestContext,
    private val mssngrApiService: MssngrApiService
) : AbstractScene<Any, UploadContactsArgs>("upload_contacts_scene", UploadContactsArgs::class),
    ApplyingSceneWithPrepare<Any, UploadContactsArgs, TUploadContactsApplyArgs> {

    private val logger = LogManager.getLogger(UploadContactsScene::class.java)

    override val applyArgsClass: KClass<TUploadContactsApplyArgs>
        get() = TUploadContactsApplyArgs::class

    override fun render(request: MegaMindRequest<Any>, args: UploadContactsArgs): RelevantResponse<Any> {
        if (requestContext.oauthToken == null) {
            throw IllegalStateException("Request oauth token must be not empty")
        }
        if (request.clientInfo.uuid.isEmpty() || request.clientInfo.deviceId == null) {
            throw IllegalStateException("Updating contacts request with empty uuid or deviceId")
        }
        if (args.fullUpload) {
            logger.info("Full uploading contacts to messenger for uuid: {}", request.clientInfo.uuid)
        } else {
            logger.info("Partial updating contacts to messenger for uuid: {}", request.clientInfo.uuid)
        }
        return ApplyNeededResponse(
            arguments = UploadContactsApplyArgs
        )
    }

    override fun prepareApply(
        request: MegaMindRequest<Any>,
        args: UploadContactsArgs,
        applyArg: TUploadContactsApplyArgs,
        responseBuilder: ScenePrepareBuilder,
    ) {
        val uploadContactsRequest = if (args.fullUpload) {
            request.getSemanticFrame(UPLOAD_CONTACTS_REQUEST_FRAME)!!.typedSemanticFrame!!
                .uploadContactsRequestSemanticFrame.uploadRequest.requestValue
        } else {
            request.getSemanticFrame(UPDATE_CONTACTS_REQUEST_FRAME)!!.typedSemanticFrame!!
                .updateContactsRequestSemanticFrame.updateRequest.requestValue
        }
        logger.info(
            "Contacts count for create: {}, for update: {}, for remove: {}",
            uploadContactsRequest.createdContactsCount,
            uploadContactsRequest.updatedContactsCount,
            uploadContactsRequest.removedContactsCount
        )

        val contactsDeviceData = getContactsMementoData(request.mementoData)
            .getContactsDeviceDataOrDefault(
                request.clientInfo.uuid,
                TContactsDeviceData.getDefaultInstance()
            )
        val mssngrRequest = mssngrApiService.buildTHttpMssngrRequest(
            uploadContactsRequest,
            contactsDeviceData, args.fullUpload, request.clientInfo.uuid
        )
        responseBuilder.addHttpRequest(MSSGR_HTTP_REQUEST_ITEM, mssngrRequest)
    }

    override fun processApply(
        request: MegaMindRequest<Any>,
        args: UploadContactsArgs,
        applyArg: TUploadContactsApplyArgs
    ): ScenarioResponseBody<Any> {

        val mssngrResponse = request.additionalSources
            .getSingleHttpResponse<UploadContactsResponse>(MSSGR_HTTP_RESPONSE_ITEM)
            ?.takeIf { it.is2xxSuccessful }
            ?.content
            ?: throw UploadContactsError(request.clientInfo.uuid, "Empty response")

        if (mssngrResponse.status != RESPONSE_STATUS_OK
            && mssngrResponse.data.status != RESPONSE_STATUS_OK
        ) {
            throw UploadContactsError(request.clientInfo.uuid, "Got response: $mssngrResponse")
        }
        logger.info("Successful uploading contacts to messenger for uuid: {}", request.clientInfo.uuid)
        return ScenarioResponseBody(
            layout = Layout(
                outputSpeech = null,
                shouldListen = false,
                contentProperties = ContentProperties.sensitiveRequest()
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = if (args.fullUpload) UPLOAD_CONTACTS_INTENT else UPDATE_CONTACTS_INTENT),
            serverDirectives = listOf(getMementoChangeUserObjectServerDirective(request)),
            isExpectsRequest = false
        )
    }

    private fun getMementoChangeUserObjectServerDirective(request: MegaMindRequest<Any>): ServerDirective {
        val oldMementoData = getContactsMementoData(request.mementoData)
        val newMementoDataBuilder = oldMementoData.toBuilder()
        newMementoDataBuilder.putContactsDeviceData(
            request.clientInfo.uuid,
            TContactsDeviceData.newBuilder()
                .setLastSyncKey(
                    oldMementoData.getContactsDeviceDataOrDefault(
                        request.clientInfo.uuid, TContactsDeviceData.getDefaultInstance()
                    ).lastSyncKey + 1
                )
                .build()
        )
        // todo switch to surfaceScenarioData when it is working. Using scenarioData for now
        return MementoChangeUserObjectsDirective(
            scenarioData = com.google.protobuf.Any.pack(newMementoDataBuilder.build()),
            surfaceScenarioData = SurfaceScenarioData(
                mapOf(
                    CONTACTS.megamindName to
                        com.google.protobuf.Any.pack(newMementoDataBuilder.build())
                )
            )
        )
    }

    private fun getContactsMementoData(mementoData: RequestProto.TMementoData): TContactsScenarioData =
        if (mementoData.scenarioData != null && mementoData.scenarioData.`is`(TContactsScenarioData::class.java)) {
            mementoData.scenarioData.unpack(TContactsScenarioData::class.java)
        } else TContactsScenarioData.getDefaultInstance()
}
