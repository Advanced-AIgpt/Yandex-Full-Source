package ru.yandex.alice.memento.ammo

import com.google.protobuf.util.JsonFormat
import ru.yandex.alice.memento.proto.MementoApiProto
import ru.yandex.alice.memento.proto.MementoApiProto.TReqGetAllObjects
import ru.yandex.alice.memento.proto.UserConfigsProto
import ru.yandex.alice.paskills.ammo.generateToFile
import ru.yandex.alice.protos.data.ChildAge

// Для запуска нужно сделать `ya make && ./run.sh ru.yandex.alice.memento.ammo.ScriptKt`
fun main() {

    val printer = JsonFormat.printer()
    // патроны будут сохранены в файл в корне
    generateToFile("get_all_objects_with_20pcnt_anonymous.txt") {

        val req = TReqGetAllObjects.newBuilder()
            .addSurfaceId("surf_id")
            .setCurrentSurfaceId("surf_id")
            .build()

        for (i in 0..200) {
            val uid = if ((i - 2).rem(5) != 0) i else null
            val tag = when {
                (i).rem(5) == 0 -> "single_key_user"
                (i - 1).rem(5) == 0 -> "two_keys_user"
                (i - 2).rem(5) == 0 -> "anonymous_user"
                else -> "empty_user"
            }

            post(path = "/memento/get_all_objects", tag = tag) {
                if (uid != null) {
                    header("X-Uid", "$uid")
                }
                header("HOST", "localhost")
                header("Content-Type", "application/json")
                header("Accept", "application/protobuf")
                header("X-Developer-Trusted-Token", "priemka_token")
                header("X-Trusted-Service-Tvm-Client-Id", "2021570")
                body(printer.print(req))
            }
        }

    }

    generateToFile("get_all_objects_and_updates_proto.txt") {

        val getReq = TReqGetAllObjects.newBuilder()
            .addSurfaceId("surf_id")
            .setCurrentSurfaceId("surf_id")
            .build()

        val baseSetReq = MementoApiProto.TReqChangeUserObjects.newBuilder()
            .setCurrentSurfaceId("surf_id")
            .addUserConfigs(
                MementoApiProto.TConfigKeyAnyPair.newBuilder()
                    .setKey(MementoApiProto.EConfigKey.CK_CHILD_AGE)
                    .setValue(com.google.protobuf.Any.pack(ChildAge.TChildAge.newBuilder().setAge(5).build()))
            )
            .build()

        fun getAllObjects(tag: String, uid: Long?) {
            post(path = "/memento/get_all_objects", tag = tag) {
                if (uid != null) {
                    header("X-Uid", "$uid")
                }
                header("HOST", "paskills-common-testing.alice.yandex.net")
                header("Content-Type", "application/protobuf")
                header("Accept", "application/protobuf")
                header("X-Developer-Trusted-Token", "priemka_token")
                header("X-Trusted-Service-Tvm-Client-Id", "2021570")
                body(getReq)
            }
        }

        fun updateObjects(tag: String, uid: Long?, vararg configs: MementoApiProto.TConfigKeyAnyPair) {
            val req: MementoApiProto.TReqChangeUserObjects = baseSetReq.toBuilder()
                .addAllUserConfigs(configs.asIterable())
                .build()

            post(path = "/memento/update_objects", tag = tag) {
                if (uid != null) {
                    header("X-Uid", "$uid")
                }
                header("HOST", "paskills-common-testing.alice.yandex.net")
                header("Content-Type", "application/protobuf")
                header("Accept", "application/protobuf")
                header("X-Developer-Trusted-Token", "priemka_token")
                header("X-Trusted-Service-Tvm-Client-Id", "2021570")
                body(req)
            }
        }

        val childAge = MementoApiProto.TConfigKeyAnyPair.newBuilder()
            .setKey(MementoApiProto.EConfigKey.CK_CHILD_AGE)
            .setValue(com.google.protobuf.Any.pack(ChildAge.TChildAge.newBuilder().setAge(5).build()))
            .build()

        val newsConfig =
            MementoApiProto.TConfigKeyAnyPair.newBuilder()
                .setKey(MementoApiProto.EConfigKey.CK_NEWS)
                .setValue(
                    com.google.protobuf.Any.pack(
                        UserConfigsProto.TNewsConfig.newBuilder().setDefaultSource(
                            "abcdefghijklmnopqrstuvwxyz0123456789"
                        ).build()
                    )
                )
                .build()


        for (i0 in 0..10000 step 10) {
            val i: Long = 100000000L + i0

            getAllObjects("single_key_user", i)
            getAllObjects("two_keys_user", i + 1)
            getAllObjects("anonymous_user", null/*i+2*/)
            getAllObjects("empty_user", i + 3)
            getAllObjects("empty_user", i + 4)

            updateObjects("update_single_key", i, childAge)

            getAllObjects("single_key_user", i + 5)
            getAllObjects("two_keys_user", i + 6)
            getAllObjects("anonymous_user", null/*i+7*/)
            getAllObjects("empty_user", i + 8)
            getAllObjects("empty_user", i + 9)

            updateObjects("update_two_keys_user", i + 1, childAge, newsConfig)
        }

    }
}
