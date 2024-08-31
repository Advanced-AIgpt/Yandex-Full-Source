package ru.yandex.alice.memento.controller

import org.apache.logging.log4j.LogManager
import org.springframework.http.ResponseEntity
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestAttribute
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RequestMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.memento.MementoService
import ru.yandex.alice.memento.proto.MementoApiProto.TClearUserData
import ru.yandex.alice.memento.proto.MementoApiProto.TReqChangeUserObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetAllObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetUserObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetAllObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetUserObjects
import ru.yandex.alice.memento.proto.MementoApiProto.VersionInfo
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmAuthorizationException
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmRequired

const val MEMENTO_VERSION_HEADER = "X-Memento-Version"

@RestController
@RequestMapping(
    consumes = ["application/protobuf", "application/x-protobuf", "application/json"],
    produces = ["application/protobuf", "application/x-protobuf"],
    path = ["/", "/memento/"]
)
@TvmRequired
internal class Controller(private val mementoService: MementoService, private val version: VersionInfo) {
    private val vsnVersion = version.svnVersion
    private val logger = LogManager.getLogger()

    @PostMapping("get_objects")
    fun getUserSettings(
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: Long?,
        @RequestBody reqUserSettings: TReqGetUserObjects,
    ): ResponseEntity<TRespGetUserObjects> {
        val (userId, anonymous) = parseUserId(uid, reqUserSettings.currentSurfaceId)

        val response = mementoService.getUserSettings(userId, anonymous, reqUserSettings)

        return ResponseEntity.ok()
            .header(MEMENTO_VERSION_HEADER, vsnVersion)
            .body(response)
    }

    @PostMapping("get_all_objects")
    fun getUserSettings(
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: Long?,
        @RequestBody reqGetAllObjects: TReqGetAllObjects,
    ): ResponseEntity<TRespGetAllObjects> {
        val (userId, anonymous) = parseUserId(uid, reqGetAllObjects.currentSurfaceId)

        val response = mementoService.getAllObjects(userId, anonymous, reqGetAllObjects)

        return ResponseEntity.ok()
            .header(MEMENTO_VERSION_HEADER, vsnVersion)
            .body(response)
    }

    @TvmRequired(userRequired = false)
    @PostMapping("clear_user_data")
    fun removeAllUserData(
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: Long?,
        @RequestBody body: TClearUserData,
    ): ResponseEntity<TRespGetUserObjects> {
        if (uid != null) {
            if (body.puid != 0L) {
                logger.error("clear_user_data called with inconsistent user_ticket=$uid and body.puid=${body.puid}")
                return ResponseEntity.badRequest().build()
            }
            mementoService.removeAllUserData(uid.toString())
        } else {
            if (body.puid == 0L) {
                logger.error("clear_user_data called with inconsistent empty body.puid")
                return ResponseEntity.badRequest().build()
            }
            mementoService.removeAllUserData(body.puid.toString())
        }
        return ResponseEntity.ok().header(MEMENTO_VERSION_HEADER, vsnVersion).build()
    }

    @PostMapping("update_objects")
    fun updateObjects(
        @RequestAttribute(value = SecurityRequestAttributes.UID_REQUEST_ATTR, required = false) uid: Long?,
        @RequestBody req: TReqChangeUserObjects,
    ): ResponseEntity<String> {
        val (userId, anonymous) = parseUserId(uid, req.currentSurfaceId)

        mementoService.updateObjects(userId, anonymous, req)

        return ResponseEntity.ok().header(MEMENTO_VERSION_HEADER, vsnVersion).build()
    }

    private fun parseUserId(uid: Long?, deviceId: String): Pair<String, Boolean> {
        return uid?.toString()?.let { Pair(it, false) }
            ?: deviceId.takeIf { it.isNotEmpty() }?.let { Pair(it, true) }
            ?: throw TvmAuthorizationException("Not TVM user ticket or X-Ya-Device-Id header provided")
    }
}
