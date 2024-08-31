package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.IrrelevantRecipeIntent
import java.util.Optional

class RecipeResponse private constructor(
    val text: String,
    val tts: String,
    val state: RecipeState,
    @get:JvmName("isEndSession") val endSession: Boolean,
    @get:JvmName("isShouldListen") val shouldListen: Boolean,
    @get:JvmName("isRelevant") val relevant: Boolean,
    val analytics: RecipeAnalytics,
    val pushDirective: Optional<SendPushMessageDirective>,
) {

    constructor(
        text: String,
        tts: String,
        state: RecipeState,
        flags: ResponseFlags,
        analytics: RecipeAnalytics,
        pushDirective: Optional<SendPushMessageDirective>
    ) : this(
        text = text,
        tts = tts,
        state = state,
        endSession = flags.getEndSession(),
        shouldListen = flags.getShouldListen(),
        relevant = flags.getIsRelevant(),
        analytics = analytics,
        pushDirective = pushDirective,
    )

    enum class ResponseFlags {
        NOT_LISTENING, LISTENING, END_SESSION, IRRELEVANT;

        fun getIsRelevant(): Boolean {
            return this != IRRELEVANT
        }

        fun getEndSession(): Boolean {
            return this == END_SESSION
        }

        fun getShouldListen(): Boolean {
            // listen after irrelevant requests in skill
            return this == LISTENING || this == IRRELEVANT
        }
    }

    class RecipeAnalytics(
        val recipe: Optional<Recipe>,
        val intent: RecipeIntent
    ) {
        companion object {
            @JvmStatic
            fun create(recipe: Recipe, intent: RecipeIntent): RecipeAnalytics {
                return RecipeAnalytics(Optional.of(recipe), intent)
            }

            @JvmStatic
            fun createWithoutRecipe(intent: RecipeIntent): RecipeAnalytics {
                return RecipeAnalytics(Optional.empty(), intent)
            }

            @JvmStatic
            fun fallback(recipe: Optional<Recipe>): RecipeAnalytics {
                return RecipeAnalytics(recipe, IrrelevantRecipeIntent)
            }

            @JvmStatic
            fun fallback(recipe: Recipe?): RecipeAnalytics {
                return RecipeAnalytics(Optional.ofNullable(recipe), IrrelevantRecipeIntent)
            }
        }
    }

    companion object {

        @JvmStatic
        @JvmOverloads
        fun create(
            textWithTts: TextWithTts,
            state: RecipeState,
            analytics: RecipeAnalytics,
            pushDirective: SendPushMessageDirective? = null
        ): RecipeResponse {
            return RecipeResponse(
                text = textWithTts.text,
                tts = textWithTts.tts,
                state = state,
                flags = ResponseFlags.NOT_LISTENING,
                analytics = analytics,
                pushDirective = Optional.ofNullable(pushDirective),
            )
        }

        @JvmStatic
        fun createListening(
            textWithTts: TextWithTts,
            state: RecipeState,
            analytics: RecipeAnalytics
        ): RecipeResponse {
            return RecipeResponse(
                textWithTts.text,
                textWithTts.tts,
                state,
                ResponseFlags.LISTENING,
                analytics,
                Optional.empty()
            )
        }

        @JvmStatic
        fun endSession(textWithTts: TextWithTts, analytics: RecipeAnalytics): RecipeResponse {
            return RecipeResponse(
                textWithTts.text,
                textWithTts.tts,
                RecipeState.EMPTY,
                ResponseFlags.END_SESSION,
                analytics,
                Optional.empty()
            )
        }

        @JvmStatic
        fun fallback(
            textWithTts: TextWithTts,
            state: RecipeState,
            analytics: RecipeAnalytics
        ): RecipeResponse {
            return RecipeResponse(
                textWithTts.text,
                textWithTts.tts,
                state,
                ResponseFlags.IRRELEVANT,
                analytics,
                Optional.empty()
            )
        }
    }
}
