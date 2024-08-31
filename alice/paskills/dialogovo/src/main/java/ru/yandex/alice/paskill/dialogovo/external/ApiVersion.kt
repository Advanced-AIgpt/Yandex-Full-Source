package ru.yandex.alice.paskill.dialogovo.external

import com.fasterxml.jackson.annotation.JsonValue

enum class ApiVersion(@JsonValue val code: String) {
    V1_0("1.0");
}
