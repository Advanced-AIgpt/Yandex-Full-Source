package ru.yandex.alice.paskill.dialogovo

import org.springframework.boot.env.YamlPropertySourceLoader

open class JsonPropertySourceLoader : YamlPropertySourceLoader() {
    override fun getFileExtensions(): Array<String> {
        return arrayOf("json")
    }
}
