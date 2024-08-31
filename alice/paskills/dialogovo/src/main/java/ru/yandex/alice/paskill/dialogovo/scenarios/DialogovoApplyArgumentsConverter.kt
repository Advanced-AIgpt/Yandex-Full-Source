package ru.yandex.alice.paskill.dialogovo.scenarios

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArgumentsConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.megamind.ApplyArgumentsWrapper
import ru.yandex.alice.paskill.dialogovo.proto.ApplyArgsProto
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.AudioPlayerEventsApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.CollectSkillShowEpisodeApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.PurchaseCompleteApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestGeolocationSharingApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.SaveFeedbackApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.TeasersApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.WidgetGalleryApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.AppmetricaCommitArgs
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.ReadNewsApplyArguments

@Component
open class DialogovoApplyArgumentsConverter : ApplyArgumentsConverter {
    override fun convert(src: ApplyArguments, ctx: ToProtoContext): ApplyArgsProto.ApplyArgumentsWrapper {

        val arguments = src as ApplyArgumentsWrapper
        val builder = ApplyArgsProto.ApplyArgumentsWrapper.newBuilder()
            .setProcessorType(arguments.processorType.name)
        val args = arguments.arguments
        builder.apply {
            when (args) {
                is RequestSkillApplyArguments -> requestSkill = convertRequestSkillApplyArgs(args)
                is SaveFeedbackApplyArguments -> saveFeedback = convertSaveFeedbackApplyArgs(args)
                is ReadNewsApplyArguments -> readNews = convertReadNewsApplyArguments(args)
                is AudioPlayerEventsApplyArguments ->
                    audioPlayerEventsApplyArgs = convertAudioPlayerEventsApplyArguments(args)
                is PurchaseCompleteApplyArguments ->
                    purchaseCompleteApplyArgs = convertPurchaseCompleteApplyArguments(args)
                is RequestGeolocationSharingApplyArguments ->
                    requestGeolocationSharingApplyArgs = convertRequestGeolocationSharingApplyArgs(args)
                is WidgetGalleryApplyArguments ->
                    collectWidgetGalleryApplyArgs = convertWidgetGalleryApplyArguments(args)
                is TeasersApplyArguments ->
                    collectTeasersApplyArgs = convertTeasersApplyArguments(args)
                is CollectSkillShowEpisodeApplyArguments ->
                    collectSkillShowEpisodeApplyArgs = convertCollectSkillShowEpisodeApplyArguments(args)
            }
        }
        return builder.build()
    }

    private fun convertRequestSkillApplyArgs(
        src: RequestSkillApplyArguments
    ): ApplyArgsProto.RelevantApplyArgs {
        return ApplyArgsProto.RelevantApplyArgs.newBuilder().apply {
            skillId = src.skillId
            src.request?.also { request = it }
            src.originalUtterance?.also { originalUtterance = it }
            src.activationSourceType?.also { sourceType -> source = sourceType.type }
            resumeSessionAfterPlayerStopRequests = src.resumeSessionAfterPlayerStopRequests
            isActivation = src.activation
            payload = src.payload ?: ""
            src.activationTypedSemanticFrameSlot?.let { activationTypedSemanticFrameSlot = it }
        }.build()
    }

    private fun convertSaveFeedbackApplyArgs(
        src: SaveFeedbackApplyArguments
    ): ApplyArgsProto.SaveFeedbackApplyArgs {
        return ApplyArgsProto.SaveFeedbackApplyArgs.newBuilder()
            .setSkillId(src.skillId)
            .setMark(src.mark.markValue)
            .build()
    }

    private fun convertReadNewsApplyArguments(
        src: ReadNewsApplyArguments
    ): ApplyArgsProto.ReadNewsApplyArgs {
        if (src.applyArgs == null) {
            return ApplyArgsProto.ReadNewsApplyArgs.newBuilder()
                .setSkillId(src.skillId)
                .build()
        } else {
            return ApplyArgsProto.ReadNewsApplyArgs.newBuilder()
                .setSkillId(src.skillId)
                .setApplyArgs(convertAppmetricaCommitArgs(src.applyArgs))
                .build()
        }
    }

    private fun convertAppmetricaCommitArgs(
        src: AppmetricaCommitArgs
    ): ApplyArgsProto.AppmetricaApplyArgs {
        return ApplyArgsProto.AppmetricaApplyArgs.newBuilder()
            .setUri(src.uri)
            .setApiKeyEncrypted(src.apiKeyEncrypted)
            .setEventEpochTime(src.eventEpochTime)
            .setReportMessage(src.reportMessage)
            .build()
    }

    private fun convertAudioPlayerEventsApplyArguments(
        src: AudioPlayerEventsApplyArguments
    ): ApplyArgsProto.TAudioPlayerEventsApplyArgs {
        return ApplyArgsProto.TAudioPlayerEventsApplyArgs.newBuilder().apply {
            skillId = src.skillId
            if (src.applyArgs != null) {
                applyArgs = convertAppmetricaCommitArgs(src.applyArgs)
            }
        }.build()
    }

    private fun convertPurchaseCompleteApplyArguments(
        src: PurchaseCompleteApplyArguments
    ): ApplyArgsProto.TPurchaseCompleteApplyArgs {
        return ApplyArgsProto.TPurchaseCompleteApplyArgs.newBuilder()
            .setSkillId(src.skillId)
            .setPurchaseOfferUuid(src.purchaseOfferUuid)
            .setInitialDeviceId(src.initialDeviceId)
            .build()
    }

    private fun convertRequestGeolocationSharingApplyArgs(
        src: RequestGeolocationSharingApplyArguments
    ): ApplyArgsProto.TRequestGeolocationSharingApplyArgs {
        return ApplyArgsProto.TRequestGeolocationSharingApplyArgs.newBuilder()
            .setSkillId(src.skillId)
            .setNormalizedUtterance(src.normalizedUtterance)
            .build()
    }

    private fun convertWidgetGalleryApplyArguments(
        src: WidgetGalleryApplyArguments
    ): ApplyArgsProto.TCollectWidgetGalleryApplyArgs {
        return ApplyArgsProto.TCollectWidgetGalleryApplyArgs.newBuilder()
            .addAllSkillIds(src.skillIds)
            .build()
    }

    private fun convertTeasersApplyArguments(
        src: TeasersApplyArguments
    ): ApplyArgsProto.TCollectTeasersApplyArgs {
        return ApplyArgsProto.TCollectTeasersApplyArgs.newBuilder()
            .addAllSkillIds(src.skillIds)
            .build()
    }

    private fun convertCollectSkillShowEpisodeApplyArguments(
        src: CollectSkillShowEpisodeApplyArguments
    ): ApplyArgsProto.TCollectSkillShowEpisodeApplyArgs {
        return ApplyArgsProto.TCollectSkillShowEpisodeApplyArgs.newBuilder()
            .setSkillId(src.skillId)
            .setShowType(src.showType.name)
            .build()
    }

    override fun convert(src: com.google.protobuf.Any): ApplyArgumentsWrapper {

        return when {
            src.`is`(ApplyArgsProto.ApplyArgumentsWrapper::class.java) ->
                parseApplyArgumentsWrapper(src.unpack(ApplyArgsProto.ApplyArgumentsWrapper::class.java))
            else -> throw RuntimeException("Failed to parse apply args proto")
        }
    }

    private fun parseApplyArgumentsWrapper(unpack: ApplyArgsProto.ApplyArgumentsWrapper): ApplyArgumentsWrapper {
        val args: ApplyArguments = unpack.run {
            when {
                hasSaveFeedback() -> parseSaveFeedbackApplyArgs(saveFeedback)
                hasRequestSkill() -> parseRequestSkillApplyArgs(requestSkill)
                hasReadNews() -> parseReadNewsApplyArgs(readNews)
                hasAudioPlayerEventsApplyArgs() -> parseAudioPlayerEventsApplyArgs(audioPlayerEventsApplyArgs)
                hasPurchaseCompleteApplyArgs() -> parsePurchaseCompleteApplyArgs(purchaseCompleteApplyArgs)
                hasRequestGeolocationSharingApplyArgs() ->
                    parseRequestGeolocationSharingApplyArgs(requestGeolocationSharingApplyArgs)
                hasCollectWidgetGalleryApplyArgs() -> parseCollectWidgetGalleryApplyArgs(collectWidgetGalleryApplyArgs)
                hasCollectTeasersApplyArgs() -> parseCollectTeasersApplyArgs(collectTeasersApplyArgs)
                hasCollectSkillShowEpisodeApplyArgs() ->
                    parseCollectSkillShowEpisodeApplyArgs(collectSkillShowEpisodeApplyArgs)
                else -> throw RuntimeException("no arguments found in apply arguments wrapper")
            }
        }
        val type = RunRequestProcessorType.byName(unpack.processorType)
            .orElseThrow { IllegalArgumentException(""""Unknown run processor type: "${unpack.processorType}"""") }
        return ApplyArgumentsWrapper(type, args)
    }

    private fun parseRequestSkillApplyArgs(src: ApplyArgsProto.RelevantApplyArgs): RequestSkillApplyArguments {
        if (src.skillId.isEmpty()) {
            throw RuntimeException("Skill id is missing")
        }
        return RequestSkillApplyArguments(
            src.skillId,
            src.request.takeIf { it.isNotEmpty() },
            src.originalUtterance.takeIf { it.isNotEmpty() },
            src.source.let { ActivationSourceType.R.fromValueO(it) }.orElse(null),
            src.resumeSessionAfterPlayerStopRequests,
            src.isActivation,
            src.payload,
        )
    }

    private fun parseSaveFeedbackApplyArgs(src: ApplyArgsProto.SaveFeedbackApplyArgs): SaveFeedbackApplyArguments {
        return SaveFeedbackApplyArguments(FeedbackMark.ofValue(src.mark), src.skillId)
    }

    private fun parseReadNewsApplyArgs(src: ApplyArgsProto.ReadNewsApplyArgs): ReadNewsApplyArguments {
        return ReadNewsApplyArguments(src.skillId, parseAppmetricaApplyArgs(src.applyArgs))
    }

    private fun parseAppmetricaApplyArgs(src: ApplyArgsProto.AppmetricaApplyArgs): AppmetricaCommitArgs {
        return AppmetricaCommitArgs(src.apiKeyEncrypted, src.uri, src.eventEpochTime, src.reportMessage)
    }

    private fun parseAudioPlayerEventsApplyArgs(
        src: ApplyArgsProto.TAudioPlayerEventsApplyArgs
    ): AudioPlayerEventsApplyArguments {
        return AudioPlayerEventsApplyArguments(src.skillId, parseAppmetricaApplyArgs(src.applyArgs))
    }

    private fun parsePurchaseCompleteApplyArgs(
        src: ApplyArgsProto.TPurchaseCompleteApplyArgs
    ): PurchaseCompleteApplyArguments {
        return PurchaseCompleteApplyArguments(
            src.skillId,
            src.purchaseOfferUuid,
            src.initialDeviceId
        )
    }

    private fun parseRequestGeolocationSharingApplyArgs(
        src: ApplyArgsProto.TRequestGeolocationSharingApplyArgs
    ): RequestGeolocationSharingApplyArguments {
        return RequestGeolocationSharingApplyArguments(
            src.skillId,
            src.normalizedUtterance
        )
    }

    private fun parseCollectWidgetGalleryApplyArgs(
        src: ApplyArgsProto.TCollectWidgetGalleryApplyArgs
    ) = WidgetGalleryApplyArguments(
        src.skillIdsList
    )

    private fun parseCollectTeasersApplyArgs(
        src: ApplyArgsProto.TCollectTeasersApplyArgs
    ) = TeasersApplyArguments(
        src.skillIdsList
    )

    private fun parseCollectSkillShowEpisodeApplyArgs(
        src: ApplyArgsProto.TCollectSkillShowEpisodeApplyArgs
    ) = CollectSkillShowEpisodeApplyArguments(
        src.skillId,
        ShowType.valueOf(src.showType)
    )
}
