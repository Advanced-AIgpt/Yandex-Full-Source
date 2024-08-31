package ru.yandex.alice.kronstadt.core.scenario

import com.google.protobuf.GeneratedMessageV3
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder

sealed class SelectedScene<State, Args : Any> {
    abstract val scene: Scene<State, Args>
    val name: String
        get() = scene.name

    abstract val args: Args

    class Running<State, Args : Any>(override val scene: Scene<State, Args>, override val args: Args) :
        SelectedScene<State, Args>() {

        fun prepareRun(request: MegaMindRequest<State>, responseBuilder: ScenePrepareBuilder) {
            if (scene is SceneWithPrepare<State, Args>) {
                scene.prepareRun(request, args, responseBuilder)
            }
        }

        fun render(request: MegaMindRequest<State>): BaseRunResponse<State>? = scene.render(request, args)

        infix fun withArgs(newArgs: Args) = Running(scene, newArgs)
    }

    class Committing<State, Args : Any, ApplyArgs : GeneratedMessageV3>(
        override val scene: CommittingScene<State, Args, ApplyArgs>,
        override val args: Args,
        val applyArgs: ApplyArgs
    ) : SelectedScene<State, Args>() {

        fun prepareCommit(request: MegaMindRequest<State>, responseBuilder: ScenePrepareBuilder) {
            if (scene is CommittingSceneWithPrepare<State, Args, ApplyArgs>) {
                scene.prepareCommit(request, args, applyArgs, responseBuilder)
            }
        }

        fun commit(request: MegaMindRequest<State>): Unit = scene.processCommit(request, args, applyArgs)
    }

    class Applying<State, Args : Any, ApplyArgs : GeneratedMessageV3>(
        override val scene: ApplyingScene<State, Args, ApplyArgs>,
        override val args: Args,
        val applyArgs: ApplyArgs
    ) : SelectedScene<State, Args>() {

        fun prepareApply(request: MegaMindRequest<State>, responseBuilder: ScenePrepareBuilder) {
            if (scene is ApplyingSceneWithPrepare<State, Args, ApplyArgs>) {
                scene.prepareApply(request, args, applyArgs, responseBuilder)
            }
        }

        fun renderApply(request: MegaMindRequest<State>): ScenarioResponseBody<State> =
            scene.processApply(request, args, applyArgs)
    }

    class Continuing<State, Args : Any, ApplyArgs : GeneratedMessageV3>(
        override val scene: ContinuingScene<State, Args, ApplyArgs>,
        override val args: Args,
        val applyArgs: ApplyArgs
    ) : SelectedScene<State, Args>() {

        fun prepareContinue(request: MegaMindRequest<State>, responseBuilder: ScenePrepareBuilder) {
            if (scene is ContinuingSceneWithPrepare<State, Args, ApplyArgs>) {
                scene.prepareContinue(request, args, applyArgs, responseBuilder)
            }
        }

        fun renderContinue(request: MegaMindRequest<State>): ScenarioResponseBody<State> =
            scene.processContinue(request, args, applyArgs)
    }
}
