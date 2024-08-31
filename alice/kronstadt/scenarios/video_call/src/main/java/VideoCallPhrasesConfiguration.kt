package ru.yandex.alice.kronstadt.scenarios.video_call

import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.kronstadt.core.text.Phrases

@Configuration
open class VideoCallPhrasesConfiguration {
    @Bean("videoCallPhrases")
    open fun videoCallPhrases(): Phrases = Phrases(listOf("phrases/video_call"))
}