package ru.yandex.alice.divktrenderer

import org.springframework.boot.SpringApplication
import org.springframework.boot.autoconfigure.SpringBootApplication
import org.springframework.context.annotation.Bean
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace

@SpringBootApplication
open class Application {

    @Bean
    open fun setrace() = Setrace("divkt-renderer")

    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            SpringApplication.run(Application::class.java, *args)
        }
    }
}
