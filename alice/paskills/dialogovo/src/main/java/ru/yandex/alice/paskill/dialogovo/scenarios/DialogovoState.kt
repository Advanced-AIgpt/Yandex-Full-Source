package ru.yandex.alice.paskill.dialogovo.scenarios

import com.fasterxml.jackson.annotation.JsonIgnore
import ru.yandex.alice.paskill.dialogovo.domain.Session
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import java.time.Instant
import java.util.Optional

data class DialogovoState(
    val currentSkillId: String? = null,
    val session: Session? = null,
    val prevResponseTimestamp: Long? = null,
    val feedbackRequestedForSkillId: String? = null,
    val newsState: NewsState? = null,
    val recipesState: RecipeState? = null,
    val sessionInBackground: Boolean = false,
    val resumeSessionAfterPlayerStopRequests: Int = 0,
    val productActivationState: ProductActivationState? = null,
    val geolocationSharingState: GeolocationSharingState? = null
) {

    @JsonIgnore
    fun getCurrentSkillIdO() = Optional.ofNullable(currentSkillId)

    @JsonIgnore
    fun getSessionO() = Optional.ofNullable(session)

    @JsonIgnore
    fun getPrevResponseTimestampO() = Optional.ofNullable(prevResponseTimestamp)

    @JsonIgnore
    fun getFeedbackRequestedForSkillIdO() = Optional.ofNullable(feedbackRequestedForSkillId)

    @JsonIgnore
    fun getNewsStateO() = Optional.ofNullable(newsState)

    @JsonIgnore
    fun getRecipesStateO() = Optional.ofNullable(recipesState)

    @JsonIgnore
    fun getProductActivationStateO() = Optional.ofNullable(productActivationState)

    @JsonIgnore
    fun getGeolocationSharingStateO() = Optional.ofNullable(geolocationSharingState)

    @JsonIgnore
    fun isSessionInBackground() = sessionInBackground

    fun withFeedbackRequest(skillId: String): DialogovoState =
        this.copy(feedbackRequestedForSkillId = skillId, newsState = null, recipesState = null, productActivationState = null)

    fun withProductActivationState(productActivationState: ProductActivationState): DialogovoState =
        this.copy(newsState = null, recipesState = null, productActivationState = productActivationState)

    fun withGeolocationSharingState(geolocationSharingState: GeolocationSharingState?): DialogovoState =
        this.copy(newsState = null, recipesState = null, productActivationState = null, geolocationSharingState = geolocationSharingState)

    data class ThereminState(
        val isInternal: Boolean,
        // skillId for external modes
        val modeId: String? = null
    )

    // Not immutable - all fields can be modified after creation - but inside NewsSkillStateMaintainer only
    data class NewsState(
        // map<skill, map<feed, news>>
        val lastRead: MutableMap<String, Map<String, String>> = mutableMapOf(),
        val postrolledProviders: MutableSet<String> = mutableSetOf(),
        var lastPostrolledTime: Instant? = null,
    ) {

        @JsonIgnore
        fun getLastPostrolledTimeO() = Optional.ofNullable(lastPostrolledTime)

        @JsonIgnore
        fun getLastNewsReadBySkillFeed(skillId: String, feedId: String): Optional<String> =
            Optional.ofNullable(lastRead[skillId]?.get(feedId))
    }

    data class ProductActivationState(
        val musicAttemptCount: Int?,
        val activationType: ActivationType,
    )

    data class GeolocationSharingState(
        val isRequested: Boolean,
        val allowedSharingUntilTime: Instant?
    ) {
        fun getAllowedSharingUntilTimeO() = Optional.ofNullable(allowedSharingUntilTime)
    }

    enum class ActivationType {
        MUSIC
    }

    companion object {
        @JvmStatic
        fun createSkillState(
            skillId: String,
            session: Session?,
            prevResponseTimestamp: Long,
            feedbackRequestedForSkillId: Optional<String>,
            geolocationSharingState: Optional<GeolocationSharingState>
        ): DialogovoState {
            return DialogovoState(
                currentSkillId = skillId,
                session = session,
                prevResponseTimestamp = prevResponseTimestamp,
                feedbackRequestedForSkillId = feedbackRequestedForSkillId.orElse(null),
                geolocationSharingState = geolocationSharingState.orElse(null)
            )
        }

        @JvmStatic
        fun createSkillState(
            skillId: String,
            session: Session?,
            prevResponseTimestamp: Long,
            feedbackRequestedForSkillId: Optional<String>,
            resumeSessionAfterPlayerStopRequests: Int,
            geolocationSharingState: Optional<GeolocationSharingState>
        ): DialogovoState {
            return DialogovoState(
                currentSkillId = skillId,
                session = session,
                // Optional.empty(),
                prevResponseTimestamp = prevResponseTimestamp,
                feedbackRequestedForSkillId = feedbackRequestedForSkillId.orElse(null),
                // Optional.empty(),
                // Optional.empty(),
                resumeSessionAfterPlayerStopRequests = resumeSessionAfterPlayerStopRequests,
                geolocationSharingState = geolocationSharingState.orElse(null),
            )
        }

        @JvmStatic
        fun createSkillState(
            skillId: String,
            session: Session?,
            prevResponseTimestamp: Long,
            feedbackRequestedForSkillId: Optional<String>,
            outOfSkillSession: Boolean,
            geolocationSharingState: Optional<GeolocationSharingState>
        ): DialogovoState {
            return DialogovoState(
                currentSkillId = skillId,
                session = session,
                prevResponseTimestamp = prevResponseTimestamp,
                feedbackRequestedForSkillId = feedbackRequestedForSkillId.orElse(null),
                sessionInBackground = outOfSkillSession,
                resumeSessionAfterPlayerStopRequests = 0,
                geolocationSharingState = geolocationSharingState.orElse(null)
            )
        }

        @JvmStatic
        fun newsSkillState(
            session: Session,
            newsState: NewsState
        ) =
            DialogovoState(session = session, newsState = newsState)

        @JvmStatic
        fun createSkillFeedbackRequest(skillId: String) =
            DialogovoState(feedbackRequestedForSkillId = skillId)

        @JvmStatic
        fun createRecipeState(src: RecipeState, currentTime: Instant) =
            DialogovoState(prevResponseTimestamp = currentTime.toEpochMilli(), recipesState = src)
    }
}
