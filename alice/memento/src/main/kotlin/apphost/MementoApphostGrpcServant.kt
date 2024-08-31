package ru.yandex.alice.memento.apphost

import NAppHostTvmUserTicket.TvmUserTicket.TTvmUserTicket
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Controller
import ru.yandex.alice.memento.MementoService
import ru.yandex.alice.memento.grpc.MementoGrpcServiceBase
import ru.yandex.alice.memento.grpc.TClearAllDataRequest
import ru.yandex.alice.memento.grpc.TClearAllDataResponse
import ru.yandex.alice.memento.grpc.TGetAllObjectsRequest
import ru.yandex.alice.memento.grpc.TGetAllObjectsResponse
import ru.yandex.alice.memento.grpc.TUpdateObjectsRequest
import ru.yandex.alice.memento.grpc.TUpdateObjectsResponse
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmAuthorizationException
import ru.yandex.passport.tvmauth.TvmClient
import ru.yandex.web.apphost.api.request.RequestMeta

@Controller
open class MementoApphostGrpcServant(
    private val mementoService: MementoService,
    private val tvmClient: TvmClient,
) : MementoGrpcServiceBase() {

    private val logger = LogManager.getLogger()

    override fun getAllObjects(requestMeta: RequestMeta, payload: TGetAllObjectsRequest): TGetAllObjectsResponse {
        val request = payload.request;

        val (userId, anonymous) = parseUserId(
            payload.tvmUserTicket.takeIf { payload.hasTvmUserTicket() },
            request.currentSurfaceId
        )

        return TGetAllObjectsResponse.newBuilder()
            .setMementoUserObjects(mementoService.getAllObjects(userId, anonymous, request))
            .build()
    }

    override fun updateObjects(requestMeta: RequestMeta, payload: TUpdateObjectsRequest): TUpdateObjectsResponse {

        val request = payload.request;

        val (userId, anonymous) = parseUserId(
            payload.tvmUserTicket.takeIf { payload.hasTvmUserTicket() },
            request.currentSurfaceId
        )

        mementoService.updateObjects(userId, anonymous, request)
        return TUpdateObjectsResponse.getDefaultInstance()
    }

    private fun parseUserId(tvmUserTicket: TTvmUserTicket?, deviceId: String): Pair<String, Boolean> {
        val userTicket: String? = tvmUserTicket
            ?.takeIf { it.userTicket.isNotEmpty() }
            ?.let { tvmClient.checkUserTicket(it.userTicket).defaultUid.toString() }

        return userTicket?.let { Pair(it, false) }
            ?: deviceId.takeIf { it.isNotEmpty() }?.let { Pair(it, true) }
            ?: throw TvmAuthorizationException("Not TVM user ticket or X-Ya-Device-Id provided")
    }

    override fun clearAllData(requestMeta: RequestMeta, payload: TClearAllDataRequest): TClearAllDataResponse {
        val uid: String? =
            payload.tvmUserTicket.takeIf { payload.hasTvmUserTicket() }?.userTicket?.takeIf { it.isNotEmpty() }
        if (uid != null) {
            if (payload.request.puid != 0L && payload.request.puid.toString() != uid) {
                logger.error("clear_user_data called with inconsistent user_ticket=$uid and body.puid=${payload.request.puid}")
                throw RuntimeException("clear_user_data called with inconsistent user_ticket=$uid and body.puid=${payload.request.puid}")
            }
            mementoService.removeAllUserData(uid.toString())
        } else {
            if (payload.request.puid == 0L) {
                logger.error("clear_user_data called with empty body.puid")
                throw RuntimeException("clear_user_data called with empty body.puid")
            }
            mementoService.removeAllUserData(payload.request.puid.toString())
        }
        return TClearAllDataResponse.getDefaultInstance()
    }
}
