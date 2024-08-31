package ru.yandex.alice.paskills.dialogovo.ammo

import com.google.protobuf.util.JsonFormat
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.paskill.dialogovo.proto.ApplyArgsProto
import ru.yandex.alice.paskills.ammo.generateToFile

fun main() {
    // патроны будут сохранены в файл в корне
    generateToFile("generate_example_req_proto_ammo.txt") {

        // RUN
        val runRequestBuilder = RequestProto.TScenarioRunRequest.newBuilder()
        JsonFormat.parser()
            .usingTypeRegistry(
                JsonFormat.TypeRegistry.newBuilder()
                    .add(RequestProto.TScenarioRunRequest.getDescriptor())
                    .build()
            ).merge(
                javaClass.classLoader.getResourceAsStream("load_run_request_example.json")
                    .reader(),
                runRequestBuilder
            )
        val runReq = runRequestBuilder.build()

        // prod coeff between run and apply rps
        val runToApplyCoeff = 5
        for (i in 0..runToApplyCoeff) {
            post(path = "/dialogovo-priemka/megamind/run", tag = "run_request_example") {
                header("X-Uid", "1")
                header("HOST", "paskills-common-testing.alice.yandex.net")
                header("Content-Type", "application/protobuf")
                header("Accept", "application/protobuf")
                header("X-Developer-Trusted-Token", "TRUSTED")
                header("X-Trusted-Service-Tvm-Client-Id", "2021570")
                body(runReq)
            }
        }

        // APPLY
        val applyRequestBuilder = RequestProto.TScenarioApplyRequest.newBuilder()
        JsonFormat.parser()
            .usingTypeRegistry(
                JsonFormat.TypeRegistry.newBuilder()
                    .add(RequestProto.TScenarioApplyRequest.getDescriptor())
                    .add(ApplyArgsProto.ApplyArgumentsWrapper.getDescriptor())
                    .build()
            ).merge(
                javaClass.classLoader.getResourceAsStream("load_apply_request_example.json")
                    .reader(),
                applyRequestBuilder
            )
        val applyReq = applyRequestBuilder.build()

        post(path = "/dialogovo-priemka/megamind/apply", tag = "apply_request_example") {
            header("X-Uid", "1")
            header("HOST", "paskills-common-testing.alice.yandex.net")
            header("Content-Type", "application/protobuf")
            header("Accept", "application/protobuf")
            header("X-Developer-Trusted-Token", "TRUSTED")
            header("X-Trusted-Service-Tvm-Client-Id", "2021570")
            body(applyReq)
        }
    }
}
