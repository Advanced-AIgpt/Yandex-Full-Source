package ru.yandex.alice.kronstadt.core.scenario

import com.google.protobuf.Any
import com.google.protobuf.GeneratedMessageV3
import ru.yandex.alice.kronstadt.core.AdditionalSources
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioCompositeResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.rpc.RpcHandler
import ru.yandex.alice.kronstadt.proto.ApplyArgsProto.TSceneArguments
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder

interface IScenario<State> {
    val scenarioMeta: ScenarioMeta
    val scenes: List<Scene<State, *>>
    val grpcHandlers: List<RpcHandler<*, *>>

    val name: String
        get() = scenarioMeta.name

    fun doPrepareSelectScene(request: MegaMindRequest<State>, responseBuilder: ApphostResponseBuilder)

    fun doSelectScene(request: MegaMindRequest<State>): SelectedScene.Running<State, *>?

    fun prepareRun(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Running<State, *>
    )

    fun renderRunResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Running<State, *>?
    ): ScenarioCompositeResponse<ResponseProto.TScenarioRunResponse>

    fun convertRunRequest(
        req: RequestProto.TScenarioRunRequest,
        uid: String?,
        additionalSources: AdditionalSources = AdditionalSources.EMPTY,
    ): MegaMindRequest<State>

    fun convertApplyRequest(
        req: RequestProto.TScenarioApplyRequest,
        uid: String?,
        additionalSources: AdditionalSources = AdditionalSources.EMPTY,
    ):
        Pair<MegaMindRequest<State>, SelectedScene.Applying<State, out kotlin.Any, out GeneratedMessageV3>>

    fun convertCommitRequest(
        req: RequestProto.TScenarioApplyRequest,
        uid: String?,
        additionalSources: AdditionalSources = AdditionalSources.EMPTY,
    ):
        Pair<MegaMindRequest<State>, SelectedScene.Committing<State, out kotlin.Any, out GeneratedMessageV3>>

    fun convertContinueRequest(
        req: RequestProto.TScenarioApplyRequest,
        uid: String?,
        additionalSources: AdditionalSources = AdditionalSources.EMPTY,
    ):
        Pair<MegaMindRequest<State>, SelectedScene.Continuing<State, out kotlin.Any, out GeneratedMessageV3>>

    fun convertMegaMindMRequest(
        userId: String?,
        baseRequest: RequestProto.TScenarioBaseRequest,
        input: RequestProto.TInput,
        dataSourcesMap: Map<Int, RequestProto.TDataSource>,
        additionalSources: AdditionalSources = AdditionalSources.EMPTY,
    ): MegaMindRequest<State>

    fun chooseRunningScene(sceneArguments: TSceneArguments): SelectedScene.Running<State, *>
    fun chooseApplyingScene(sceneArguments: TSceneArguments, applyArgs: Any): SelectedScene.Applying<State, *, *>
    fun chooseCommittingScene(sceneArguments: TSceneArguments, applyArgs: Any): SelectedScene.Committing<State, *, *>
    fun chooseContinuingScene(sceneArguments: TSceneArguments, applyArgs: Any): SelectedScene.Continuing<State, *, *>

    fun prepareApply(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Applying<State, *, *>
    )

    fun renderApplyResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Applying<State, *, *>
    ): ScenarioCompositeResponse<ResponseProto.TScenarioApplyResponse>

    fun prepareCommit(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Committing<State, *, *>
    )

    fun renderCommitResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Committing<State, *, *>
    ): ResponseProto.TScenarioCommitResponse

    fun prepareContinue(
        request: MegaMindRequest<State>,
        responseBuilder: ApphostResponseBuilder,
        selectedScene: SelectedScene.Continuing<State, *, *>
    )

    fun renderContinueResponse(
        request: MegaMindRequest<State>,
        selectedScene: SelectedScene.Continuing<State, *, *>
    ): ScenarioCompositeResponse<ResponseProto.TScenarioContinueResponse>

    fun packSceneArguments(selectedScene: SelectedScene<State, *>): TSceneArguments
}
