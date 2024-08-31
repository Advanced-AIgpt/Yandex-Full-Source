package ru.yandex.alice.paskill.dialogovo.vins

import com.fasterxml.jackson.core.JsonProcessingException
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.domain.Interfaces
import ru.yandex.alice.kronstadt.core.layout.Button
import ru.yandex.alice.kronstadt.core.layout.TextCard
import ru.yandex.alice.kronstadt.core.layout.div.DivBody
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.AvatarsNamespace
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias
import ru.yandex.alice.paskill.dialogovo.domain.RequestGeolocationButton
import ru.yandex.alice.paskill.dialogovo.domain.Session
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.domain.StartAccountLinkingButton
import ru.yandex.alice.paskill.dialogovo.domain.StartPurchaseButton
import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerEventRequest
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerFailedRequest
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerState
import ru.yandex.alice.paskill.dialogovo.external.v1.response.BigImageCard
import ru.yandex.alice.paskill.dialogovo.external.v1.response.BigImageListCard
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Card
import ru.yandex.alice.paskill.dialogovo.external.v1.response.CardType
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListCard
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListItem
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.service.normalizer.NormalizationService
import ru.yandex.alice.paskill.dialogovo.utils.parseClientInfo
import ru.yandex.alice.paskill.dialogovo.vins.Card.DivCard
import ru.yandex.alice.paskill.dialogovo.vins.Card.TextWithButtons
import ru.yandex.alice.paskill.dialogovo.vins.ErrorVinsBlock.ErrorBlockData
import ru.yandex.alice.paskill.dialogovo.vins.ItemsListVinsCard.ItemsListVinsItem
import ru.yandex.alice.paskill.dialogovo.vins.MarkupVinsSlot.VinsMarkup
import ru.yandex.alice.paskill.dialogovo.vins.ResponseVinsSlot.ResponseVinsSlotData
import ru.yandex.alice.paskill.dialogovo.vins.SessionVinsSlot.VinsSession
import ru.yandex.alice.paskill.dialogovo.vins.SkillMetaSlot.SkillMeta
import ru.yandex.alice.paskill.dialogovo.vins.VinsRequest.VinsAction
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult
import java.net.URI
import java.time.Instant
import java.util.Optional
import java.util.Random
import java.util.stream.Collectors

typealias Handler = (SkillProcessRequest.SkillProcessRequestBuilder, VinsSlot<*>) -> Unit

@Component
class VinsConverter(
    @Value("\${avatarsUrl}") private val avatarsUrl: String,
    private val objectMapper: ObjectMapper,
    private val skillProvider: SkillProvider,
    private val normalizer: NormalizationService,
    private val overrideCardButtonUrlDivBlockVisitor: OverrideCardButtonUrlDivBlockVisitor
) {
    private val logger = LogManager.getLogger()
    private val vinsSlotMapping: Map<Class<out VinsSlot<out Any>>, Handler> = mapOf(
        SessionVinsSlot::class.java to this::handleSession,
        RequestVinsSlot::class.java to this::handleRequest,
        MarkupVinsSlot::class.java to this::handleMarkup,
        SkillMetaSlot::class.java to this::handleSkillMeta,
        RandomSeedSlot::class.java to this::handleRandomSeed,
        AudioPlayerStateSlot::class.java to this::handleAudioPlayerState,
    )

    fun convert(req: VinsRequest): SkillProcessRequest {
        val builder = SkillProcessRequest.builder()
            .requestTime(Instant.now())
            .session(Optional.empty())
            .viewState(Optional.empty())
            .activationSourceType(ActivationSourceType.DEV_CONSOLE)
            // random can be overridden with "random_seed" slot, see handleRandomSeed()
            .random(Random())
            .mementoData(RequestProto.TMementoData.getDefaultInstance())
        handleForm(req, builder)
        handleMeta(req, builder)
        handleActions(req, builder)

        if (req.location != null) {
            builder.locationInfo(Optional.of(req.location))
            builder.needToShareGeolocation(true)
        }
        return builder.build()
    }

    private fun handleSession(builder: SkillProcessRequest.SkillProcessRequestBuilder, slot: VinsSlot<*>) {
        val session = slot.value as VinsSession?
        if (session != null) {
            builder.session(
                Optional.of(
                    Session.create(
                        session.id,
                        session.seq,
                        Instant.now(),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(),
                        ActivationSourceType.DEV_CONSOLE
                    )
                )
            )
        }
    }

    private fun handleRequest(builder: SkillProcessRequest.SkillProcessRequestBuilder, slot: VinsSlot<*>) {
        val reqSlot = slot as RequestVinsSlot
        if (reqSlot.type == "button") {
            val node = reqSlot.value["payload"]
            builder.isButtonPress(true)
                .buttonPayload(if (node.isTextual) node.asText() else node.toString())
        } else if (reqSlot.type == "audio_player_event") {
            try {
                val audioPlayerVinsEvent = objectMapper.treeToValue(
                    reqSlot.value,
                    AudioPlayerVinsEvent::class.java
                )
                if (audioPlayerVinsEvent.type != InputType.AUDIO_PLAYER_PLAYBACK_FAILED) {
                    builder.audioPlayerEvent(
                        AudioPlayerEventRequest(audioPlayerVinsEvent.type)
                    )
                } else {
                    val audioPlayerFailedVinsEvent =
                        objectMapper.treeToValue(reqSlot.value, AudioPlayerFailedVinsEvent::class.java)
                    builder.audioPlayerEvent(AudioPlayerFailedRequest(audioPlayerFailedVinsEvent.error))
                }
            } catch (e: JsonProcessingException) {
                logger.error("Error while converting request vins slot with audio_player_event [$reqSlot]", e)
            }
        } else {
            builder.normalizedUtterance(normalizer.normalize(reqSlot.value.asText()))
        }
    }

    private fun handleMarkup(builder: SkillProcessRequest.SkillProcessRequestBuilder, slot: VinsSlot<*>) {
        val markup = slot.value as VinsMarkup
        builder.isDangerousContext(markup.isDangerousContext)
    }

    private fun handleSkillMeta(builder: SkillProcessRequest.SkillProcessRequestBuilder, slot: VinsSlot<*>) {
        val skillMeta = slot.value as SkillMeta
        val skill: SkillInfo = if (skillMeta.isDraft) {
            skillProvider.getSkillDraft(skillMeta.skillId)
                .orElseThrow { RuntimeException("Skill draft not found by id " + skillMeta.skillId) }
        } else {
            skillProvider.getSkill(skillMeta.skillId)
                .orElseThrow { RuntimeException("Skill not found by id " + skillMeta.skillId) }
        }
        builder.skill(skill)
    }

    private fun handleRandomSeed(builder: SkillProcessRequest.SkillProcessRequestBuilder, slot: VinsSlot<*>) {
        val seed = slot.value as Int
        builder.random(Random(seed.toLong()))
    }

    private fun handleAudioPlayerState(builder: SkillProcessRequest.SkillProcessRequestBuilder, slot: VinsSlot<*>) {
        val audioPlayerState = slot.value as AudioPlayerState
        builder.audioPlayerState(audioPlayerState)
    }

    private fun handleForm(req: VinsRequest, builder: SkillProcessRequest.SkillProcessRequestBuilder) {
        val form = req.form
        val slots = form.slots
        for (slot in slots) {
            val cls: Class<out VinsSlot<*>> = slot.javaClass
            if (vinsSlotMapping.containsKey(cls)) {
                vinsSlotMapping[cls]!!.invoke(builder, slot)
            }
        }
    }

    private fun handleMeta(req: VinsRequest, builder: SkillProcessRequest.SkillProcessRequestBuilder) {
        val meta = req.meta
        val interfaces = Interfaces(hasAudioClient = true)
        builder.originalUtterance(meta.utterance)
            .clientInfo(parseClientInfo(meta.clientId, meta.uuid, interfaces))
            .locationInfo(Optional.empty())
    }

    private fun handleActions(req: VinsRequest, builder: SkillProcessRequest.SkillProcessRequestBuilder) {
        val action = req.action
        if (action != null) {
            builder.accountLinkingCompleteEvent(action.name == VinsAction.ACCOUNT_LINKING_COMPLETE_EVENT_NAME)
        }
    }

    fun convert(vinsReq: VinsRequest, res: SkillProcessResult, errors: List<ExternalDevError>): VinsResponse {
        val blocks: List<VinsBlock<*>>?
        val formName: String
        if (!res.endSession) {
            blocks = getBlocks(vinsReq, res, errors)
            formName = "personal_assistant.scenarios.external_skill"
        } else {
            // can play audio on end session
            blocks = getAudioPlayerVinsBlock(res)?.let { listOf(it) }
            formName = "personal_assistant.scenarios.external_skill__deactivate"
        }
        val form = VinsForm(formName, getSlots(res))
        return VinsResponse(
            blocks = blocks,
            form = form,
            layout = res.layout.toConsoleLayout(),
            endSession = res.endSession,
        )
    }

    private fun ru.yandex.alice.kronstadt.core.layout.Layout.toConsoleLayout(): Layout = Layout(
        cards = this.cards.map { it.toConsoleCard() },
        outputSpeech = outputSpeech,
        shouldListen = shouldListen,
        suggests = this.suggests.map { it.toButton() }
    )

    private fun ru.yandex.alice.kronstadt.core.layout.Card.toConsoleCard(): ru.yandex.alice.paskill.dialogovo.vins.Card =
        when (this) {
            is TextCard -> TextWithButtons(this.text, this.buttons.map { it.toButton() })
            is DivBody -> {
                val divCard = DivCard(this)
                DivCardWalker.walk(overrideCardButtonUrlDivBlockVisitor, divCard.body)
                divCard
            }
            else -> throw RuntimeException("unknown card type: ${this.javaClass}")
        }

    private fun getSlots(res: SkillProcessResult): List<VinsSlot<*>> {
        val slots = ArrayList<VinsSlot<*>>()
        slots.add(SkillIdVinsSlot(res.skill.id))
        if (res.getResponseO().isPresent) {
            slots.add(
                DebugVinsSlot(
                    DebugVinsSlot.DebugInfo(
                        res.getResponseO().flatMap { it.rawResponse }.orElse(null),
                        res.webhookRequest
                    )
                )
            )
        }
        // if no skill response text is available (i.e. for account linking and other directive responses),
        // take text card's text
        val responseText =
            res.skillResponseText ?: if (res.layout.joinTextCards() != null) res.layout.joinTextCards()!!.text else null
        slots.add(
            ResponseVinsSlot(
                ResponseVinsSlotData(
                    responseText,
                    res.rawTts
                )
            )
        )
        if (res.getSession().isPresent) {
            slots.add(
                SessionVinsSlot(
                    VinsSession(
                        res.getSession().get().sessionId,
                        res.getSession().get().messageId, res.endSession
                    )
                )
            )
        }
        return slots
    }

    private fun hasScreen(clientId: String?): Boolean {
        return clientId != null && clientId.startsWith("ru.yandex.searchplugin")
    }

    private fun getBlocks(
        vinsReq: VinsRequest,
        resp: SkillProcessResult,
        errors: List<ExternalDevError>
    ): List<VinsBlock<*>> {
        val blocks = ArrayList<VinsBlock<*>>()
        if (errors.isNotEmpty()) {
            blocks.add(ErrorVinsBlock(ErrorBlockData(errors)))
            return blocks
        }
        blocks.add(ExternalSkillDeactivateVinsBlock)

        val joinedTextCard = resp.layout.joinTextCards()
        val buttons = joinedTextCard?.buttons ?: emptyList()
        for (btn in buttons) {
            if (btn is StartAccountLinkingButton) {
                blocks.add(StartAccountLinkingBlock(btn))
            } else {
                blocks.add(ButtonBlock(btn))
            }
        }
        for (btn in resp.layout.suggests) {
            blocks.add(ButtonBlock(btn))
        }
        val cardO = resp
            .getResponseO()
            .flatMap { obj: WebhookRequestResult -> obj.response }
            .flatMap { obj: WebhookResponse -> obj.response }
            .flatMap { obj: Response -> obj.card }
        if (cardO.isPresent && hasScreen(vinsReq.meta.clientId)) {
            val card = cardO.get()
            if (card.type != CardType.INVALID_CARD) {
                blocks.add(DivCardVinsSlot(convert(card)))
            }
        }
        getAudioPlayerVinsBlock(resp)?.let { blocks.add(it) }
        blocks.add(ClientFeaturesVinsBlock)
        return blocks
    }

    private fun getAudioPlayerVinsBlock(resp: SkillProcessResult): AudioPlayerVinsBlock? {
        return resp.audioPlayerAction
            .map { data -> AudioPlayerVinsBlock(data) }
            .orElse(null)
    }

    private fun convert(card: Card): VinsCard {
        return when (card.type) {
            CardType.BIG_IMAGE -> {
                val bigImageCard = card as BigImageCard
                val namespace = bigImageCard.mdsNamespace
                    .map { name -> AvatarsNamespace.createCustom(name) }
                    .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD)
                val imageUrl = getImageUrl(bigImageCard.imageId, namespace, ImageAlias.ONE_X2)
                BigImageVinsCard(
                    imageUrl,
                    bigImageCard.title.orElse(null),
                    bigImageCard.description.orElse(null),
                    bigImageCard.button.orElse(null)
                )
            }
            CardType.ITEMS_LIST -> {
                val itemsListCard = card as ItemsListCard
                ItemsListVinsCard(
                    itemsListCard.items
                        .stream()
                        .map { itemsListItem: ItemsListItem -> this.convert(itemsListItem) }
                        .collect(Collectors.toList()),
                    itemsListCard.header.orElse(null),
                    itemsListCard.footer.orElse(null),
                )
            }
            CardType.BIG_IMAGE_LIST, CardType.IMAGE_GALLERY -> {
                val imageGalleryCard = card as BigImageListCard
                ImageGalleryVinsCard(
                    imageGalleryCard.items.map { item: BigImageListCard.Item -> this.convert(item) },
                    imageGalleryCard.header
                        .map { header -> ImageGalleryVinsCard.ItemsListCardHeader(header.text.orElse(null)) }
                        .orElse(null)
                )
            }
            CardType.INVALID_CARD -> throw RuntimeException("Invalid card type: ${card.type}")
        }
    }

    private fun convert(itemsListItem: ItemsListItem): ItemsListVinsItem {
        val imageUrl = itemsListItem.imageId.map { imageId: String ->
            val namespace = itemsListItem.mdsNamespace
                .map { name -> AvatarsNamespace.createCustom(name) }
                .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD)

            getImageUrl(imageId, namespace, ImageAlias.MENU_LIST_X2)
        }
        return ItemsListVinsItem(
            title = itemsListItem.title.orElse(null),
            imageId = imageUrl.orElse(null),
            description = itemsListItem.description.orElse(null),
            button = itemsListItem.button.orElse(null)
        )
    }

    private fun convert(item: BigImageListCard.Item): ImageGalleryVinsCard.Item {
        val imageId = item.imageId
        val namespace = item.mdsNamespace
            .map { name: String -> AvatarsNamespace.createCustom(name) }
            .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD)
        val imageUrl = getImageUrl(imageId, namespace, ImageAlias.ORIG)
        return ImageGalleryVinsCard.Item(
            imageUrl = imageUrl,
            title = item.title.orElse(null),
            description = item.description.orElse(null),
            button = item.button.orElse(null)
        )
    }

    private fun getImageUrl(imageId: String, namespace: AvatarsNamespace, alias: ImageAlias): String {
        return UriComponentsBuilder
            .fromHttpUrl(avatarsUrl)
            .pathSegment(namespace.urlPath, imageId, alias.code)
            .build()
            .toString()
    }
}

private fun Button.toButton(): ru.yandex.alice.paskill.dialogovo.vins.Button = when (this) {
    is StartAccountLinkingButton -> Button(
        title = this.text,
        directives = listOf(ConsoleDirective.StartAccountLinking(this.authPageUrl))
    )
    is StartPurchaseButton -> Button(
        title = this.text,
        directives = listOf(ConsoleDirective.StartPurchase(this.offerUuid, this.offerUrl))
    )
    is RequestGeolocationButton -> Button(
        title = this.text,
        directives = listOf(ConsoleDirective.RequestGeosharing(this.period.toMinutes()))
    )
    else -> Button(this.text, this.url?.let { URI.create(it) }, this.payload)
}
