package ru.yandex.alice.kronstadt.core.scenario

import com.fasterxml.jackson.annotation.JsonIgnore
import com.google.protobuf.GeneratedMessageV3
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import kotlin.reflect.KClass

// maybe extract name to annotation
interface Scene<State, Args : Any> {
    val name: String

    @get:JsonIgnore
    val argsClass: KClass<Args>

    fun render(request: MegaMindRequest<State>, args: Args): BaseRunResponse<State>?

    // process request for the given scene
    fun selectScene(request: MegaMindRequest<State>): SelectedScene.Running<State, *>? = null
}

interface NoargScene<State> : Scene<State, Any> {
    val arg: Any
        get() = NOARG

    fun render(request: MegaMindRequest<State>): BaseRunResponse<State>?

    override fun render(request: MegaMindRequest<State>, args: Any) = render(request)

    companion object {
        private val NOARG = Any()
    }
}

interface SceneWithPrepare<State, Args : Any> : Scene<State, Args> {
    fun prepareRun(request: MegaMindRequest<State>, args: Args, responseBuilder: ScenePrepareBuilder)
}

abstract class AbstractScene<State, Args : Any>(
    override val name: String,
    override val argsClass: KClass<Args>
) : Scene<State, Args>

abstract class AbstractNoargScene<State>(name: String) : AbstractScene<State, Any>(name, Any::class),
    NoargScene<State> {

    abstract override fun render(request: MegaMindRequest<State>): BaseRunResponse<State>?

    final override fun render(request: MegaMindRequest<State>, args: Any) = render(request)
}

interface WithApplyArguments<ApplyArg : GeneratedMessageV3> {
    @get:JsonIgnore
    val applyArgsClass: KClass<ApplyArg>
}

interface ApplyingScene<State, Args : Any, ApplyArg : GeneratedMessageV3> :
    Scene<State, Args>, WithApplyArguments<ApplyArg> {

    fun processApply(request: MegaMindRequest<State>, args: Args, applyArg: ApplyArg): ScenarioResponseBody<State>
}

interface ApplyingSceneWithPrepare<State, Args : Any, ApplyArg : GeneratedMessageV3> :
    ApplyingScene<State, Args, ApplyArg>, WithApplyArguments<ApplyArg> {

    fun prepareApply(
        request: MegaMindRequest<State>,
        args: Args,
        applyArg: ApplyArg,
        responseBuilder: ScenePrepareBuilder
    )
}

interface CommittingScene<State, Args : Any, ApplyArg : GeneratedMessageV3> :
    Scene<State, Args>, WithApplyArguments<ApplyArg> {

    fun processCommit(request: MegaMindRequest<State>, args: Args, applyArg: ApplyArg)
}

interface CommittingSceneWithPrepare<State, Args : Any, ApplyArg : GeneratedMessageV3> :
    CommittingScene<State, Args, ApplyArg>, WithApplyArguments<ApplyArg> {

    fun prepareCommit(
        request: MegaMindRequest<State>,
        args: Args,
        applyArg: ApplyArg,
        responseBuilder: ScenePrepareBuilder
    )
}

interface ContinuingScene<State, Args : Any, ApplyArg : GeneratedMessageV3> :
    Scene<State, Args>, WithApplyArguments<ApplyArg> {

    fun processContinue(request: MegaMindRequest<State>, args: Args, applyArg: ApplyArg): ScenarioResponseBody<State>
}

interface ContinuingSceneWithPrepare<State, Args : Any, ApplyArg : GeneratedMessageV3> :
    ContinuingScene<State, Args, ApplyArg>, WithApplyArguments<ApplyArg> {

    fun prepareContinue(
        request: MegaMindRequest<State>,
        args: Args,
        applyArg: ApplyArg,
        responseBuilder: ScenePrepareBuilder
    )
}
