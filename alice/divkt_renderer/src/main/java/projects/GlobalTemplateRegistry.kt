package ru.yandex.alice.divktrenderer.projects

import com.google.protobuf.Message
import com.yandex.div.dsl.context.CardWithTemplates
import org.springframework.stereotype.Component
import ru.yandex.alice.divkit.*
import ru.yandex.alice.divktrenderer.grpc.TDivktRendererRequest
import ru.yandex.alice.divktrenderer.projects.example.ExampleDiv
import ru.yandex.alice.library.client.protos.ClientInfoProto.TClientInfoProto
import ru.yandex.alice.protos.data.language.Language.ELang
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData.DataCase
import java.time.Instant

@Component
class GlobalTemplateRegistry {
    private val exampleDiv = ExampleDiv()
    private val templateMap = mapOf<DataCase, DivKitTemplate<*>>(
        DataCase.EXAMPLESCENARIODATA to exampleDiv,
    )
    private val caseToFieldMap = DataCase.values()
        .filterNot { it == DataCase.DATA_NOT_SET }
        .associateWith { TScenarioData.getDescriptor().findFieldByNumber(it.number) ?: throw IllegalStateException("Can't find field for ") }

    fun selectTemplateAndRender(scenarioData: TScenarioData, request: TDivktRendererRequest) : CardWithTemplates? {
        val templateId = scenarioData.dataCase
        return when(val template = templateMap[templateId]!!) {
            is IdempotentDivKitTemplate -> {
                template.doRender(
                    ctx = createContext(scenarioData, request),
                    scenarioData = scenarioData,
                    case = templateId,
                )
            }
            is NonIdempotentDivKitTemplate -> {
                template.doRender(
                    ctx = createContext(scenarioData, request),
                    scenarioData = scenarioData,
                    case = templateId,
                )
            }
        }
    }

    private fun <T: Message> IdempotentDivKitTemplate<T>.doRender(ctx: StaticContext, scenarioData: TScenarioData, case: DataCase): CardWithTemplates {
        val arg = scenarioData.getField(scenarioData.descriptorForType.findFieldByNumber(case.number)) as T
        return this.render(ctx, arg)
    }
    private fun <T: Message> NonIdempotentDivKitTemplate<T>.doRender(ctx: DynamicContext, scenarioData: TScenarioData, case: DataCase): CardWithTemplates {
        val arg = scenarioData.getField(scenarioData.descriptorForType.findFieldByNumber(case.number)) as T
        return this.render(ctx, arg)
    }

    private fun createContext(scenarioData: TScenarioData, request: TDivktRendererRequest): DynamicContextImpl {
        val baseRequest = if (request.hasRequest()) {
            request.request.baseRequest
        } else if (request.hasRunRequest()) {
            request.runRequest.baseRequest
        } else if (request.hasApplyRequest()) {
            request.applyRequest.baseRequest
        } else {
            null
        }
        val combinatorBaseRequest = if (request.hasCombinatorRequest()) request.combinatorRequest.baseRequest else null

        val clientInfo: TClientInfoProto
        val lang: ELang
        val experiments: Set<String>
        val serverTime: Instant
        val randomSeed: Long
        if (baseRequest != null) {
            clientInfo = baseRequest.clientInfo
            lang = baseRequest.userLanguage
            experiments = baseRequest.experiments.fieldsMap.map { it.key }.toSet()
            serverTime = Instant.ofEpochMilli(baseRequest.serverTimeMs)
            randomSeed = baseRequest.randomSeed
        } else if (combinatorBaseRequest != null) {
            clientInfo = combinatorBaseRequest.clientInfo
            lang = combinatorBaseRequest.userLanguage
            experiments = combinatorBaseRequest.experiments.fieldsMap.map { it.key }.toSet()
            serverTime = Instant.ofEpochMilli(combinatorBaseRequest.serverTimeMs)
            randomSeed = combinatorBaseRequest.randomSeed
        } else {
            clientInfo = TClientInfoProto.getDefaultInstance()
            lang = ELang.L_RUS
            experiments = setOf()
            serverTime = Instant.now()
            randomSeed = 0
        }

        return DynamicContextImpl(clientInfo, lang, experiments, randomSeed, serverTime)
    }
}
