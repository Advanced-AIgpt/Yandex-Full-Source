package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.ActionRef.Companion.withTypedSemanticFrame
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective
import ru.yandex.alice.kronstadt.core.directive.TtsPlayPlaceholderDirective.Companion.TTS_PLAY_PLACEHOLDER_DIRECTIVE_DIALOG
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.scenarios.video_call.CALL_TO_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.MatchedContactsObject
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ContactChoosingScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ProviderContactData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.VideoCallToSemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.VideoCallScenarioDataConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.NluHints

const val CHOOSE_CONTACT_CARD_ID = "video_call.choose_contact.card.id"
const val CALL_TO_PREFIX = "call_to"

@Component
class SelectContactScene(
    private val scenarioDataConverter: VideoCallScenarioDataConverter,
    private val nluHints: NluHints
) : AbstractScene<Any, SelectContactScene.Args>(
    name = "select_contact_scene",
    argsClass = Args::class
) {
    class Args(val matchedContacts: List<Contact>)

    override fun render(request: MegaMindRequest<Any>, args: Args): RelevantResponse<Any> {
        val contactNames = args.matchedContacts.map { it.displayName }.joinToString()
        val actions = args.matchedContacts.mapIndexed { index, contact ->
            "${CALL_TO_PREFIX}_$index" to chooseContactVoiceButton(contact, index)
        }.toMap()
        return RunOnlyResponse(
            layout = Layout.textWithOutputSpeech(
                textWithTts = TextWithTts("Выберите, кому звонить: $contactNames"),
                shouldListen = true,
                directives = listOf(
                        ShowViewDirective(
                            layer = ShowViewDirective.Layer.DIALOG,
                            card = ShowViewDirective.CardId(CHOOSE_CONTACT_CARD_ID),
                            inactivityTimeout = ShowViewDirective.InactivityTimeout.LONG
                        ),
                        TTS_PLAY_PLACEHOLDER_DIRECTIVE_DIALOG
                    )
            ),
            state = null,
            analyticsInfo = createAnalyticsInfo(args),
            isExpectsRequest = false,
            actions = actions,
            renderData = listOf(
                    DivRenderData(
                        cardId = CHOOSE_CONTACT_CARD_ID,
                        scenarioData = getScenarioData(args.matchedContacts)
                    )
                )
        )
    }

    private fun createAnalyticsInfo(args: Args) = AnalyticsInfo(
        intent = CALL_TO_INTENT,
        objects = listOf(MatchedContactsObject(args.matchedContacts)),
    )

    private fun chooseContactVoiceButton(contact: Contact, index: Int): ActionRef {
        return withTypedSemanticFrame(
            utterance = contact.displayName!!,
            frame = VideoCallToSemanticFrame(contact),
            nluHint = nluHints.makeChooseContactNluHint(
                frameName = "${CALL_TO_PREFIX}_${contact.getFilledLookupKey()}",
                contactName = contact.displayName!!,
                displayPosition = index,
            ),
            purpose = CALL_TO_INTENT
        )
    }

    private fun getScenarioData(
        matchedContacts: List<Contact>,
    ) = scenarioDataConverter.convert(
        ContactChoosingScenarioData(
            contactData = matchedContacts.map {
                ProviderContactData(
                    displayName = it.displayName!!,
                    userId = it.contactId.toString()
                )
            },
        ),
        ToProtoContext()
    )
}
