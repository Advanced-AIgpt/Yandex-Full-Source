package ru.yandex.alice.paskill.dialogovo.scenarios

import org.apache.logging.log4j.util.Strings
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.StateConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.Session
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TNewsState
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TNewsState.TNewsByFeed
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TSkillFeedbackState
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters.GeolocationSharingStateConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters.ProductActivationStateConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters.SessionConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.RecipeStateConverter
import ru.yandex.alice.paskill.dialogovo.utils.UniqueList
import java.time.Duration
import java.time.Instant
import java.util.Optional

@Component
open class DialogovoStateConverter(
    private val sessionConverter: SessionConverter,
    private val recipeStateConverter: RecipeStateConverter,
    private val productActivationStateConverter: ProductActivationStateConverter,
    private val geolocationSharingStateConverter: GeolocationSharingStateConverter
) : StateConverter<DialogovoState> {

    private val DIALOG_TIMEOUT = Duration.ofMinutes(5L)
    private val CENTAUR_DIALOG_TIMEOUT = Duration.ofDays(1L)

    override fun convert(src: DialogovoState, ctx: ToProtoContext): DialogovoStateProto.State {
        return DialogovoStateProto.State.newBuilder().apply {
            currentSkillId = src.currentSkillId ?: ""
            isSessionInBackground = src.sessionInBackground
            resumeSessionAfterPlayerStopRequests = src.resumeSessionAfterPlayerStopRequests
            src.session?.also { session = sessionConverter.convert(it, ctx) }
            prevResponseTimestamp = src.prevResponseTimestamp ?: 0L

            if (src.feedbackRequestedForSkillId != null) {
                skillFeedbackState = TSkillFeedbackState.newBuilder()
                    .setSkillId(src.feedbackRequestedForSkillId)
                    .build()
            }
            if (src.newsState != null) {
                val protoNewsStateBuilder = TNewsState.newBuilder()
                    .addAllPostrolledProviders(src.newsState.postrolledProviders)
                    .putAllLastFeedNewsBySkillRead(
                        src.newsState.lastRead.mapValues { (_, value) ->
                            TNewsByFeed
                                .newBuilder()
                                .putAllFeedNews(value)
                                .build()
                        }
                    )
                protoNewsStateBuilder.lastPostrolledProviderTimestamp =
                    src.newsState.lastPostrolledTime?.toEpochMilli() ?: 0L

                newsState = protoNewsStateBuilder.build()
            }
            src.recipesState?.also { recipeState = recipeStateConverter.convert(it, ctx) }
            src.productActivationState?.also {
                productActivationState = productActivationStateConverter.convert(it, ctx)
            }

            src.geolocationSharingState?.also {
                geolocationSharingState = geolocationSharingStateConverter.convert(it, ctx)
            }
        }.build()
    }

    override fun convert(baseRequest: RequestProto.TScenarioBaseRequest): DialogovoState? {
        // TODO: use separate state TTL and IsNewSession handler for each state
        return if (baseRequest.hasState() &&
            baseRequest.state.serializedSize > 0 &&
            baseRequest.state.`is`(DialogovoStateProto.State::class.java)
        ) {
            val unpacked: DialogovoStateProto.State = baseRequest.state.unpack(DialogovoStateProto.State::class.java)

            // check timeout for smart-speakers and similar devices with no tabs (no dialog_id)
            // valid for all surfaces
            val sinceLastAnswer = if (unpacked.prevResponseTimestamp != 0L) {
                Duration.ofMillis(baseRequest.serverTimeMs - unpacked.prevResponseTimestamp)
            } else {
                Duration.ZERO
            }
            val state = getState(unpacked)
            if (state.recipesState != null) {
                return state
            }
            if (state.sessionInBackground) {
                return state
            }
            if (baseRequest.isNewSession) {
                // save state if new mm session and news scenario exists
                if (state.newsState == null) {
                    return null
                }
            }
            if (state.currentSkillId != null) {
                if ("" != baseRequest.dialogId) {
                    // check if tab is the same skill as in state
                    if (state.currentSkillId == baseRequest.dialogId) state else null
                } else {
                    // for non-tab (speakers) check timeout
                    // todo: replace "isCentaur" with new supported feature
                    val sessionTimeout = if (baseRequest.clientInfo.appId == "ru.yandex.centaur") CENTAUR_DIALOG_TIMEOUT
                        else DIALOG_TIMEOUT
                    if (sinceLastAnswer <= sessionTimeout) state else null
                }
            } else {
                state
            }
        } else {
            null
        }
    }

    private fun getState(src: DialogovoStateProto.State): DialogovoState {
        val skillId = Strings.trimToNull(src.currentSkillId)
        val session = if (src.hasSession()) convertSession(src.session) else null
        val feedbackRequestedForSkill = src.skillFeedbackState.skillId.takeIf { it.isNotEmpty() }
        val newsState = if (src.hasNewsState()) {
            convertNewsState(src.newsState)
        } else null
        val productActivationState = if (src.hasProductActivationState()) {
            convertProductActivationState(src.productActivationState)
        } else null
        val geolocationSharingState = if (src.hasGeolocationSharingState()) {
            convertGeolocationSharingState(src.geolocationSharingState)
        } else null
        val recipeState = if (src.recipeState != DialogovoStateProto.State.TRecipeState.getDefaultInstance()) {
            convertRecipeState(src.recipeState)
        } else null
        return DialogovoState(
            currentSkillId = skillId,
            session = session,
            prevResponseTimestamp = src.prevResponseTimestamp.takeIf { it > 0L },
            feedbackRequestedForSkillId = feedbackRequestedForSkill,
            newsState = newsState,
            recipesState = recipeState,
            sessionInBackground = src.isSessionInBackground,
            resumeSessionAfterPlayerStopRequests = src.resumeSessionAfterPlayerStopRequests,
            productActivationState = productActivationState,
            geolocationSharingState = geolocationSharingState
        )
    }

    private fun convertNewsState(newsState: TNewsState): DialogovoState.NewsState {
        return DialogovoState.NewsState(
            lastRead = newsState.lastFeedNewsBySkillReadMap.mapValues { (_, newsFeed) ->
                newsFeed.feedNewsMap.toMutableMap()
            }.toMutableMap(),
            postrolledProviders = newsState.postrolledProvidersList.toMutableSet(),
            lastPostrolledTime = newsState.lastPostrolledProviderTimestamp.takeIf { it > 0L }?.let { Instant.ofEpochMilli(it) }

        )
    }

    private fun convertProductActivationState(proto: DialogovoStateProto.State.TProductActivationState): DialogovoState.ProductActivationState? {
        return when (proto.activationType) {
            DialogovoStateProto.State.TProductActivationState.TActivationType.MUSIC -> DialogovoState.ProductActivationState(
                proto.musicAttemptCount,
                DialogovoState.ActivationType.MUSIC
            )
            else -> null
        }
    }

    private fun convertRecipeState(proto: DialogovoStateProto.State.TRecipeState): RecipeState {
        val stateType: RecipeState.StateType = when (proto.stateType) {
            DialogovoStateProto.State.TRecipeState.TStateType.SELECT_RECIPE -> RecipeState.StateType.SELECT_RECIPE
            DialogovoStateProto.State.TRecipeState.TStateType.RECIPE_STEP -> RecipeState.StateType.RECIPE_STEP
            DialogovoStateProto.State.TRecipeState.TStateType.RECIPE_STEP_AWAITS_TIMER -> RecipeState.StateType.RECIPE_STEP_AWAITS_TIMER
            DialogovoStateProto.State.TRecipeState.TStateType.WAITING_FOR_FEEDBACK -> RecipeState.StateType.WAITING_FOR_FEEDBACK
            else -> RecipeState.StateType.SELECT_RECIPE
        }
        val timers = proto.timersList
            .map { protoTimer: DialogovoStateProto.State.TRecipeState.TTimer ->
                RecipeState.TimerState(
                    protoTimer.id,
                    protoTimer.text,
                    protoTimer.tts,
                    protoTimer.shouldRingAtEpochMs,
                    protoTimer.durationSeconds
                )
            }
        return RecipeState(
            protoStringToOptional(proto.sessionId),
            stateType,
            protoStringToOptional(proto.recipeId),
            if (proto.cookingStarted) Optional.of(proto.currentStepId) else Optional.empty(),
            timers,
            protoStringToOptional(proto.previousIntent),
            UniqueList.from(proto.completedStepsList),
            proto.createdTimerIdsList.toSet(),
            proto.onboardingSeenIdsList.toSet()
        )
    }

    private fun protoStringToOptional(s: String): Optional<String> {
        return if (Strings.isEmpty(s)) Optional.empty() else Optional.of(s)
    }

    private fun convertSession(session: DialogovoStateProto.State.SkillSession): Session {
        val proactiveSkillExitState = if (session.hasProactiveSkillExitState()) {
            Session.ProactiveSkillExitState.create(
                session.proactiveSkillExitState.suggestedExitAtMessageId,
                session.proactiveSkillExitState.doNotUnderstandReplyCounter
            )
        } else Session.ProactiveSkillExitState.createEmpty()
        return Session.create(
            Strings.trimToNull(session.sessionId),
            session.messageId,
            if (session.startTimestamp != 0L) Instant.ofEpochMilli(session.startTimestamp) else Instant.now(),
            session.isEnded,
            proactiveSkillExitState,
            ActivationSourceType.R.fromValueOrDefault(session.activationSourceType, ActivationSourceType.UNDETECTED),
            session.appMetricaEventCounter,
            session.failCounter
        )
    }

    private fun convertGeolocationSharingState(proto: DialogovoStateProto.State.TGeolocationSharingState): DialogovoState.GeolocationSharingState? {
        val allowedSharingUntilTime = proto.allowedSharingUntilTime.takeIf { it != 0L }?.let { Instant.ofEpochMilli(it) }
        return DialogovoState.GeolocationSharingState(proto.isRequested, allowedSharingUntilTime)
    }
}
