package ru.yandex.alice.kronstadt.runner

import org.springframework.boot.SpringApplication
import org.springframework.boot.autoconfigure.SpringBootApplication
import org.springframework.boot.autoconfigure.data.web.SpringDataWebAutoConfiguration
import org.springframework.boot.autoconfigure.gson.GsonAutoConfiguration
import org.springframework.boot.autoconfigure.web.servlet.error.ErrorMvcAutoConfiguration

@SpringBootApplication(
    scanBasePackages = [
        "ru.yandex.alice.kronstadt.core",
        "ru.yandex.alice.kronstadt.server",
        "ru.yandex.alice.kronstadt.runner",
        "ru.yandex.alice"
    ], exclude = [
        GsonAutoConfiguration::class,
        SpringDataWebAutoConfiguration::class,
        // remove /error page
        ErrorMvcAutoConfiguration::class,
    ]
)
open class KronstadtApplication {

    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            SpringApplication.run(KronstadtApplication::class.java, *args)
        }
    }
}
