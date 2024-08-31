package ru.yandex.alice.paskills.ammo.example

import com.google.protobuf.Struct
import com.google.protobuf.Value
import ru.yandex.alice.paskills.ammo.generateToFile
import ru.yandex.alice.paskills.ammo.generateToStdOut

// Для запуска нужно сделать ya make && ./run.sh ru.yandex.alice.paskills.ammo.example.ScriptKt
fun main() {
    // патроны будут распечатаны в StdOut
    generateToStdOut {
        get(path = "/the_path?foo=bar") {
            header("X-Uid", "123")
        }
    }

    // патроны будут сохранены в файл в корне
    generateToFile("ammo.txt") {
        post(path = "/", tag = "simple_post") {
            headers(
                "Host" to "example.com",
                "Accept" to "*/*",
                "Content-Type" to "application/x-www-form-urlencoded"
            )
            body("foo=bar")
        }
    }

    generateToFile("ammo_proto.txt") {
        post(path = "/", tag = "simple_proto_post") {
            headers(
                "Host" to "example.com",
                "Accept" to "*/*",
                "Content-Type" to "application/x-protobuf"
            )
            body(
                Struct.newBuilder()
                    .putFields(
                        "a", Value.newBuilder()
                            .setStringValue("test")
                            .build()
                    ).build()
            )
        }
    }
}
