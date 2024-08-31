package ru.yandex.alice.kronstadt.runner

import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.InitializingBean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.kronstadt.core.layout.div.DivBody
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.megamind.protos.scenarios.ScenarioRequestMeta
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TShowViewDirective
import ru.yandex.alice.paskills.common.proto.utils.ProtoWarmupper
import ru.yandex.alice.protos.api.renderer.Api
import ru.yandex.alice.protos.data.Contacts
import ru.yandex.alice.protos.div.Div2CardProto.TDiv2Card
import ru.yandex.alice.protos.endpoint.CapabilityProto
import ru.yandex.web.apphost.api.format.Format

@Configuration
internal open class ProtoWarmupApplicationListener(private val objectMapper: ObjectMapper) : InitializingBean {

    private val logger = LogManager.getLogger()

    override fun afterPropertiesSet() {
        logger.info("Loading file descriptors")
        //preinitialize Apphost format
        Format.values()
        logger.info("Perform ProtoWarmupper routines")
        ProtoWarmupper.warmup(
            objectMapper = objectMapper,
            protoMessages = hashSetOf(
                RequestProto.TScenarioRunRequest.newBuilder().build(),
                ScenarioRequestMeta.TRequestMeta.newBuilder().build(),
                ApplyArgsProto.TSelectedScene.newBuilder().build(),
                ApplyArgsProto.TSceneArguments.newBuilder().build(),
                RequestProto.TScenarioApplyRequest.newBuilder().build(),
                Api.TDivRenderData.newBuilder().build(),
                TDirective.newBuilder().build(),
                ResponseProto.TScenarioRunResponse.newBuilder().build(),
                ResponseProto.TScenarioApplyResponse.newBuilder().build(),
                ResponseProto.TScenarioCommitResponse.newBuilder().build(),
                ResponseProto.TScenarioContinueResponse.newBuilder().build(),
                CapabilityProto.TVideoCallCapability.newBuilder().build(),
                Contacts.TContactsList.newBuilder().build(),
                FrameProto.TTypedSemanticFrame.newBuilder().build(),
                TShowViewDirective.newBuilder().build(),
                TDiv2Card.newBuilder().build(),
            ),
        )
        ProtoWarmupper.warmupJackson(objectMapper, setOf(DivBody::class.java))
        logger.info("Finish warmup")
    }
}
