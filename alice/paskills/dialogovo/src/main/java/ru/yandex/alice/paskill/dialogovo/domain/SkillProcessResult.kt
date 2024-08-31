package ru.yandex.alice.paskill.dialogovo.domain

import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.directive.StartMusicRecognizerDirective
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.kronstadt.core.domain.sayOrDefault
import ru.yandex.alice.kronstadt.core.layout.Button
import ru.yandex.alice.kronstadt.core.layout.ContentProperties
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.utils.append
import ru.yandex.alice.kronstadt.core.utils.prepend
import ru.yandex.alice.paskill.dialogovo.domain.Session.ProactiveSkillExitState
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequestBase
import ru.yandex.alice.paskill.dialogovo.external.v1.response.OpenYandexAuthCommand
import ru.yandex.alice.paskill.dialogovo.external.v1.response.TeaserMeta
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookMordoviaCommand
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookMordoviaShow
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WidgetGalleryMeta
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioPlayerAction
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowItemMeta
import ru.yandex.alice.paskill.dialogovo.service.proactive_exit_message.ProactiveSkillExitSuggest
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult
import java.util.Optional

data class SkillProcessResult internal constructor(
    // null if no call to webhook was performed
    val webhookRequest: WebhookRequestBase?,
    val response: WebhookRequestResult?,
    val endSession: Boolean,
    private val session: Session?,
    val layout: Layout,
    val voice: String?, // tts markup with <speaker voice=""> tags
    val rawTts: String?, // tts markup without <speaker voice=""> tags. Used in developer console.
    val skillResponseText: String?, // skill response's response.text field. Useds in developer console.
    val skill: SkillInfo,
    val viaZora: Boolean,
    val canvasShow: Optional<WebhookMordoviaShow>,
    val canvasCommand: Optional<WebhookMordoviaCommand>,
    val audioPlayerAction: Optional<AudioPlayerAction>,
    val proactiveExitSuggestResult: Optional<ProactiveSkillExitSuggest.ApplyResult>,
    val startMusicRecognizerDirective: Optional<StartMusicRecognizerDirective>,
    val showEpisode: Optional<ShowItemMeta>,
    val widgetItem: Optional<WidgetGalleryMeta>,
    val teasersItem: Optional<List<TeaserMeta>>,
    val openYandexAuthCommand: Optional<OpenYandexAuthCommand>,
    val requestedGeolocation: Optional<Boolean>,
    val serverDirectives: List<ServerDirective>,
    val nluHints: Map<String, ActionRef>,
    val floydExitNode: String?,
) {

    fun getResponseO(): Optional<WebhookRequestResult> {
        return Optional.ofNullable(response)
    }

    fun getSession(): Optional<Session> {
        val result = Optional.ofNullable(session)
        if (proactiveExitSuggestResult.isPresent) {
            val realProactiveExitSuggestResult = proactiveExitSuggestResult.get()
            val currentMessageId: Long
            val previouslySuggestedExitAt: Long
            if (result.isPresent) {
                currentMessageId = result.get().messageId
                previouslySuggestedExitAt = result.get().proactiveSkillExitState.suggestedExitAtMessageId
            } else {
                currentMessageId = 0
                previouslySuggestedExitAt = 0
            }
            val proactiveSkillExitStateUpdate = ProactiveSkillExitState.create(
                if (realProactiveExitSuggestResult.isSuggestedExit) currentMessageId else previouslySuggestedExitAt,
                realProactiveExitSuggestResult.doNotUnderstandCounter
            )
            return result.map { s: Session -> s.withProactiveSkillExitState(proactiveSkillExitStateUpdate) }
        }
        return result
    }

    // TODO: fix
    fun withSuggestButtons(newSuggestButtons: List<Button>): SkillProcessResult {
        val newLayout = layout.withSuggests(newSuggestButtons)
        return this.copy(layout = newLayout)
    }

    class Builder(
        private var response: WebhookRequestResult?,
        private var skill: SkillInfo,
        private var webhookRequest: WebhookRequestBase?,
        private var viaZora: Boolean = true,
        private var endSession: Boolean = false,
        private var session: Session? = null,
        // true is set as default for builder so as not to skip its explicit set
        private var layout: Layout.Builder = Layout.builder().contentProperties(ContentProperties.allSensitive()),
        private var voice: String? = null,
        private var rawTts: String? = null,
        private var skillResponseText: String? = null,
        private var canvasShow: Optional<WebhookMordoviaShow> = Optional.empty(),
        private var canvasCommand: Optional<WebhookMordoviaCommand> = Optional.empty(),
        private var audioPlayerAction: Optional<AudioPlayerAction> = Optional.empty(),
        private var proactiveExitSuggestResult: Optional<ProactiveSkillExitSuggest.ApplyResult> = Optional.empty(),
        private var startMusicRecognizerDirective: Optional<StartMusicRecognizerDirective> = Optional.empty(),
        private var showEpisode: Optional<ShowItemMeta> = Optional.empty(),
        private var widgetItem: Optional<WidgetGalleryMeta> = Optional.empty(),
        private var teasersItem: Optional<List<TeaserMeta>> = Optional.empty(),
        private var openYandexAuthCommand: Optional<OpenYandexAuthCommand> = Optional.empty(),
        private var requestedGeolocation: Optional<Boolean> = Optional.empty(),
        private var serverDirectives: MutableList<ServerDirective> = mutableListOf(),
        private var nluHints: MutableMap<String, ActionRef> = mutableMapOf(),
        private var floydExitNode: String? = null,
    ) {

        fun build(): SkillProcessResult {
            return SkillProcessResult(
                webhookRequest = webhookRequest,
                response = response,
                endSession = endSession,
                session = session,
                layout = layout.build(),
                voice = voice,
                rawTts = rawTts,
                skillResponseText = skillResponseText,
                skill = skill,
                viaZora = viaZora,
                canvasShow = canvasShow,
                canvasCommand = canvasCommand,
                audioPlayerAction = audioPlayerAction,
                proactiveExitSuggestResult = proactiveExitSuggestResult,
                startMusicRecognizerDirective = startMusicRecognizerDirective,
                showEpisode = showEpisode,
                widgetItem = widgetItem,
                teasersItem = teasersItem,
                openYandexAuthCommand = openYandexAuthCommand,
                requestedGeolocation = requestedGeolocation,
                serverDirectives = serverDirectives.toList(),
                nluHints = nluHints,
                floydExitNode = floydExitNode,
            )
        }

        fun skill(skill: SkillInfo): Builder {
            this.skill = skill
            return this
        }

        fun webhookRequest(webhookRequest: WebhookRequestBase?): Builder {
            this.webhookRequest = webhookRequest
            return this
        }

        fun response(response: WebhookRequestResult): Builder {
            this.response = response
            return this
        }

        fun viaZora(viaZora: Boolean): Builder {
            this.viaZora = viaZora
            return this
        }

        fun endSession(endSession: Boolean): Builder {
            this.endSession = endSession
            return this
        }

        fun session(session: Session?): Builder {
            this.session = session
            return this
        }

        fun setTts(tts: String?, voice: Voice? = null): Builder {
            this.rawTts = tts
            this.voice = voice.sayOrDefault(tts)
            return this
        }

        fun appendTts(suffix: String?, voice: Voice? = null, separator: String = " "): Builder {
            this.rawTts = this.rawTts.append(suffix, separator)
            this.voice = this.voice.append(voice.sayOrDefault(suffix), separator)
            return this
        }

        fun prependTts(prefix: String?, voice: Voice? = null, separator: String = " "): Builder {
            this.rawTts = this.rawTts.prepend(prefix, separator, true)
            this.voice = this.voice.prepend(voice.sayOrDefault(prefix), separator, true)
            return this
        }

        fun canvasShow(canvasShow: Optional<WebhookMordoviaShow>): Builder {
            this.canvasShow = canvasShow
            return this
        }

        fun canvasCommand(canvasCommand: Optional<WebhookMordoviaCommand>): Builder {
            this.canvasCommand = canvasCommand
            return this
        }

        fun audioPlayerAction(audioPlayerAction: Optional<AudioPlayerAction>): Builder {
            this.audioPlayerAction = audioPlayerAction
            return this
        }

        fun proactiveExitSuggestResult(proactiveExitSuggestResult: Optional<ProactiveSkillExitSuggest.ApplyResult>): Builder {
            this.proactiveExitSuggestResult = proactiveExitSuggestResult
            return this
        }

        fun startMusicRecognizerDirective(startMusicRecognizerDirective: Optional<StartMusicRecognizerDirective>): Builder {
            this.startMusicRecognizerDirective = startMusicRecognizerDirective
            return this
        }

        fun showEpisode(showEpisode: Optional<ShowItemMeta>): Builder {
            this.showEpisode = showEpisode
            return this
        }

        fun widgetItem(widgetItem: Optional<WidgetGalleryMeta>): Builder {
            this.widgetItem = widgetItem
            return this
        }

        fun teasersItem(teasersItem: Optional<List<TeaserMeta>>): Builder {
            this.teasersItem = teasersItem
            return this
        }

        fun openYandexAuthCommand(openYandexAuthCommand: Optional<OpenYandexAuthCommand>): Builder {
            this.openYandexAuthCommand = openYandexAuthCommand
            return this
        }

        fun requestedGeolocation(requestedGeolocation: Optional<Boolean>): Builder {
            this.requestedGeolocation = requestedGeolocation
            return this
        }

        fun getLayout(): Layout.Builder {
            return this.layout
        }

        fun skillResponseText(skillResponseText: String?): Builder {
            this.skillResponseText = skillResponseText
            return this
        }

        fun serverDirective(serverDirective: ServerDirective): Builder {
            this.serverDirectives.add(serverDirective)
            return this
        }

        fun nluHint(key: String, action: ActionRef): Builder {
            this.nluHints[key] = action
            return this
        }

        fun floydExitNode(floydExitNode: String?): Builder {
            this.floydExitNode = floydExitNode
            return this
        }

        fun sensitiveData(sensitiveData: Boolean): Builder {
            if (sensitiveData) {
                this.layout.contentProperties(ContentProperties.allSensitive())
            } else {
                this.layout.contentProperties(null)
            }
            return this
        }
    }

    companion object {
        @JvmStatic
        fun builder(
            response: WebhookRequestResult?,
            skill: SkillInfo,
            webhookRequest: WebhookRequestBase?
        ): SkillProcessResult.Builder {
            return Builder(response, skill, webhookRequest)
        }
    }
}
