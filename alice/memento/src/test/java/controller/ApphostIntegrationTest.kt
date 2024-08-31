package ru.yandex.alice.memento.controller

import NAppHostTvmUserTicket.TvmUserTicket.TTvmUserTicket
import com.google.protobuf.Any
import com.google.protobuf.Message
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.http.HttpHeaders
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.TClearUserData
import ru.yandex.alice.memento.proto.MementoApiProto.TConfigKeyAnyPair
import ru.yandex.alice.memento.proto.MementoApiProto.TDeviceConfigs
import ru.yandex.alice.memento.proto.MementoApiProto.TDeviceConfigsKeyAnyPair
import ru.yandex.alice.memento.proto.MementoApiProto.TReqChangeUserObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetAllObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TRespGetAllObjects
import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceScenarioData
import ru.yandex.alice.memento.tvm.UnitTestTvmClient
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders
import ru.yandex.web.apphost.api.AppHostService
import ru.yandex.web.apphost.grpc.Client

internal class ApphostIntegrationTest : AbstractIntegrationTest() {

    @Autowired
    private lateinit var apphost: AppHostService
    private lateinit var client: Client
    private val DUMMY_SERVICE_TICKET = "test-ticket"

    @BeforeEach
    public override fun setUp() {
        client = Client("localhost", apphost.port)
        super.setUp()
    }

    @AfterEach
    public override fun tearDown() {
        super.tearDown()
    }

    override fun getAllSettings(anonymous: Boolean): TRespGetAllObjects {
        val builder = TReqGetAllObjects.newBuilder()
            .addSurfaceId(DUMMY_DEVICE_ID)
            .setCurrentSurfaceId(DUMMY_DEVICE_ID)
        val headers = HttpHeaders()
        if (!anonymous) {
            headers.add("X-Uid", DUMMY_USER_ID)
        }

        val grpcResponse = client.call("/get_all_objects") {
            this.addProtobufItem("get_all_objects_request", builder.build())
            if (!anonymous) {
                this.addProtobufItem(
                    "tvm_user_ticket",
                    TTvmUserTicket.newBuilder().setUserTicket(DUMMY_SERVICE_TICKET).build()
                )
            }
        }

        val response = grpcResponse.items.firstOrNull { it.type == "memento_user_objects" }
            ?.getProtobufData(TRespGetAllObjects.getDefaultInstance())
            ?: TRespGetAllObjects.getDefaultInstance()

        return response
    }

    override fun clearUserData() {

        val builder = TClearUserData.newBuilder()
            .setPuid(DUMMY_USER_ID.toLong())

        val grpcResponse = client.call("/clear_all_data") {
            this.addProtobufItem("clear_all_data_request", builder.build())
        }

        Assertions.assertEquals(listOf("!nothing"), grpcResponse.items.map { it.type })
    }

    override fun storeUserAndDeviceSettings(
        anonymous: Boolean,
        userChanges: Map<EConfigKey, Message>, deviceChanges: Map<EDeviceConfigKey, Message>,
        scenariosData: Map<String, Message>, surfaceScenariosData: Map<String, Message>
    ) {
        val headers = HttpHeaders()
        if (!anonymous) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, "test-ticket")
        }
        headers.add(HttpHeaders.ACCEPT, "application/protobuf")
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, UnitTestTvmClient.SERVICE_TICKET)
        val changeBody = TReqChangeUserObjects.newBuilder()
            .setCurrentSurfaceId(DUMMY_DEVICE_ID)
            .addAllUserConfigs(
                userChanges.entries
                    .map { (key, value) ->
                        TConfigKeyAnyPair.newBuilder()
                            .setKey(key)
                            .setValue(Any.pack(value))
                            .build()
                    }
            )
            .addDevicesConfigs(
                if (deviceChanges.isEmpty()) TDeviceConfigs.newBuilder() else TDeviceConfigs.newBuilder()
                    .setDeviceId(DUMMY_DEVICE_ID)
                    .addAllDeviceConfigs(deviceChanges.entries
                        .map { (key, value) ->
                            TDeviceConfigsKeyAnyPair.newBuilder()
                                .setKey(key)
                                .setValue(Any.pack(value))
                                .build()
                        })
            )
            .putAllScenarioData(
                scenariosData.mapValues { (_, value) -> Any.pack(value) }
            )
            .putAllSurfaceScenarioData(
                if (surfaceScenariosData.isEmpty()) emptyMap() else java.util.Map.of(
                    DUMMY_DEVICE_ID,
                    TSurfaceScenarioData.newBuilder()
                        .putAllScenarioData(
                            surfaceScenariosData.mapValues { (_, value) -> Any.pack(value) }
                        )
                        .build()
                )
            )
            .build()

        val grpcResponse = client.call("/update_objects") {
            this.addProtobufItem("update_objects_request", changeBody)
            if (!anonymous) {
                this.addProtobufItem(
                    "tvm_user_ticket",
                    TTvmUserTicket.newBuilder().setUserTicket(DUMMY_SERVICE_TICKET).build()
                )
            }
        }

        Assertions.assertEquals(listOf("!nothing"), grpcResponse.items.map { it.type })
    }
}
