package ru.yandex.alice.social.sharing.document

import com.fasterxml.jackson.databind.ObjectMapper
import com.google.protobuf.util.JsonFormat
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.paskills.common.ydb.YdbClient
import ru.yandex.alice.social.sharing.proto.WebPageProto
import java.time.Duration

private val testDocuments = listOf(
    Document(
        "123",
        WebPageProto.TScenarioSharePage.newBuilder()
            .setSocialNetworkMarkup(
                WebPageProto.TScenarioSharePage.TSocialNetworkMarkup.newBuilder()
                    .setTitle("Тестовая страница")
                    .build()
            )
            .setFrame(
                FrameProto.TTypedSemanticFrame.newBuilder()
                    .setExternalSkillFixedActivateSemanticFrame(
                        FrameProto.TExternalSkillFixedActivateSemanticFrame.newBuilder()
                            .setFixedSkillId(
                                FrameProto.TStringSlot.newBuilder()
                                    .setStringValue("672f7477-d3f0-443d-9bd5-2487ab0b6a4c")
                                    .build()
                            )
                    )
            )
            .setExternalSkill(
                WebPageProto.TExternalSkillTemplate.newBuilder()
                    .setSkillId("672f7477-d3f0-443d-9bd5-2487ab0b6a4c")
            )
            .build()
    )
)

@Configuration
open class DocumentProviderConfiguration {

    @Bean
    open fun documentProvider(
        ydbClient: YdbClient,
        objectMapper: ObjectMapper,
        @Value("\${document.get_by_id.timeout.ms}") getDocumentByIdTimeout: Long,
        @Value("\${document.upsert.timeout.ms}") upsertDocumentTimeout: Long,
    ): DocumentProvider {
        val provider = YdbDocumentProvider(
            ydbClient,
            objectMapper,
            Duration.ofMillis(getDocumentByIdTimeout),
            Duration.ofMillis(upsertDocumentTimeout),
        )
        for (document in testDocuments) {
            var upserted = false
            while (!upserted) {
                try {
                    provider.createCandidate(document)
                    provider.commitCandidate(document.id)
                    upserted = true
                } catch (e: Exception) {
                    logger.error(e)
                }

                logger.info("Start sleep between attempts")
                Thread.sleep(2000)
                logger.info("Sleep between attempts finished")
            }
            val document2 = provider.get(document.id)
            logger.info("document:\n{}", JsonFormat.printer().print(document2!!.page))
        }
        return provider
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
