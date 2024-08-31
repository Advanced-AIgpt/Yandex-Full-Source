package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import ru.yandex.alice.paskill.dialogovo.config.SearchAppStyles
import ru.yandex.alice.paskill.dialogovo.config.UserSkillProductConfig
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.AudioPlayerIntentToSkillProxyProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.AudioPlayerModalityResumeProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.AudioPlayerPauseIntentProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.AudioPlayerReplayIntentProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.AudioPlayerRewindIntentProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.GetNextAudioPlayerItemProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.ProcessOnAudioPlayFailedCallbackProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.ProcessOnAudioPlayFinishedCallbackProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.ProcessOnAudioPlayStartedCallbackProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio.ProcessOnAudioPlayStoppedCallbackProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.feedback.LogNonsenseFeedbackRunProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.feedback.RequestFeedbackFormRunProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.feedback.SaveFeedbackRunProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.geolocation.RequestGeolocationSharingProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.SkillsOnboardingScrollNextCallbackProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.SkillsOnboardingStationRunProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment.PurchaseCompleteReturnToStationProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment.PurchaseCompleteSameDeviceProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.show.MorningShowGetEpisodeProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.teasers.TeasersCollectingProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.widgets.WidgetGalleryCollectingProcessor

@Configuration
internal open class DialogovoMegaMindScenarioConfiguration {

    @Bean
    open fun userSkillProductConfig(objectMapper: ObjectMapper): UserSkillProductConfig {
        return PathMatchingResourcePatternResolver()
            .getResource("classpath:user_skill_product_config.json")
            .inputStream
            .use {
                objectMapper.readValue(it)
            }
    }

    @Bean
    open fun searchAppStyles(objectMapper: ObjectMapper): SearchAppStyles {
        return PathMatchingResourcePatternResolver()
            .getResource("classpath:search_app_styles.json")
            .inputStream
            .use {
                objectMapper.readValue(it)
            }
    }

    @Bean("baseDialogovoProcessors")
    open fun baseDialogovoProcessors(
        fairytaleActivateRunProcessor: FairytaleActivateRunProcessor,
        skillDeactivateRunProcessor: SkillDeactivateRunProcessor,
        skillsOnboardingStationRunProcessor: SkillsOnboardingStationRunProcessor,
        skillsOnboardingScrollNextCallbackProcessor: SkillsOnboardingScrollNextCallbackProcessor,
        gamesOnboardingWithDivCardsRunProcessor: GamesOnboardingWithDivCardsRunProcessor,
        fixedSkillActivateRunProcessor: FixedSkillActivateRunProcessor,
        newDialogSessionInputProcessor: NewDialogSessionInputProcessor,
        skillActivateWithRequestRunProcessor: SkillActivateWithRequestRunProcessor,
        processSkillRunProcessor: ProcessSkillRunProcessor,
        skillActivateRunProcessor: SkillActivateRunProcessor,
        userDeclinedSuggestedSkillCallbackProcessor: UserDeclinedSuggestedSkillCallbackProcessor,
        requestFeedbackFormRunProcessor: RequestFeedbackFormRunProcessor,
        saveFeedbackRunProcessor: SaveFeedbackRunProcessor,
        logNonsenseFeedbackRunProcessor: LogNonsenseFeedbackRunProcessor,
        accountLinkingCompleteProcessor: AccountLinkingCompleteProcessor,
        userAgreementAcceptedProcessor: UserAgreementAcceptedProcessor,
        userAgreementRejectedProcessor: UserAgreementRejectedProcessor,
        purchaseCompleteReturnToStationProcessor: PurchaseCompleteReturnToStationProcessor,
        purchaseCompleteSameDeviceProcessor: PurchaseCompleteSameDeviceProcessor,
        intentActivateRunProcessor: IntentActivateRunProcessor,
        louderProcessor: LouderProcessor,
        quieterProcessor: QuieterProcessor,
        setSoundLevelProcessor: SetSoundLevelProcessor,
        musicSkillProductActivationProcessor: MusicSkillProductActivationProcessor,
        requestGeolocationSharingProcessor: RequestGeolocationSharingProcessor,
        processOnAudioPlayStartedCallbackProcessor: ProcessOnAudioPlayStartedCallbackProcessor,
        processOnAudioPlayStoppedCallbackProcessor: ProcessOnAudioPlayStoppedCallbackProcessor,
        processOnAudioPlayFinishedCallbackProcessor: ProcessOnAudioPlayFinishedCallbackProcessor,
        processOnAudioPlayFailedCallbackProcessor: ProcessOnAudioPlayFailedCallbackProcessor,
        getNextAudioPlayerItemProcessor: GetNextAudioPlayerItemProcessor,
        audioPlayerIntentToSkillProxyProcessor: AudioPlayerIntentToSkillProxyProcessor,
        audioPlayerReplayIntentProcessor: AudioPlayerReplayIntentProcessor,
        audioPlayerPauseIntentProcessor: AudioPlayerPauseIntentProcessor,
        audioPlayerRewindIntentProcessor: AudioPlayerRewindIntentProcessor,
        audioPlayerModalityResumeProcessor: AudioPlayerModalityResumeProcessor,
        widgetGalleryCollectingProcessor: WidgetGalleryCollectingProcessor,
        teasersCollectingProcessor: TeasersCollectingProcessor,
        morningShowGetEpisodeProcessor: MorningShowGetEpisodeProcessor?
    ): ProcessorListPrototype {
        return ProcessorListPrototype(
            listOfNotNull(
                accountLinkingCompleteProcessor,
                userAgreementAcceptedProcessor,
                userAgreementRejectedProcessor,
                purchaseCompleteReturnToStationProcessor,
                purchaseCompleteSameDeviceProcessor,
                newDialogSessionInputProcessor,
                fixedSkillActivateRunProcessor,
                fairytaleActivateRunProcessor,
                skillActivateRunProcessor,
                gamesOnboardingWithDivCardsRunProcessor,
                skillsOnboardingStationRunProcessor,
                skillsOnboardingScrollNextCallbackProcessor,
                skillActivateWithRequestRunProcessor,
                intentActivateRunProcessor,
                skillDeactivateRunProcessor,
                louderProcessor,
                quieterProcessor,
                requestFeedbackFormRunProcessor,
                saveFeedbackRunProcessor,
                logNonsenseFeedbackRunProcessor,
                setSoundLevelProcessor,

                // audio player
                processOnAudioPlayStartedCallbackProcessor,
                processOnAudioPlayStoppedCallbackProcessor,
                processOnAudioPlayFinishedCallbackProcessor,
                processOnAudioPlayFailedCallbackProcessor,
                getNextAudioPlayerItemProcessor,
                audioPlayerIntentToSkillProxyProcessor,
                audioPlayerReplayIntentProcessor,
                audioPlayerPauseIntentProcessor,
                audioPlayerRewindIntentProcessor,

                // user skill products
                musicSkillProductActivationProcessor,
                requestGeolocationSharingProcessor,
                processSkillRunProcessor,
                userDeclinedSuggestedSkillCallbackProcessor,

                // probably must be last processor - cause contains relatively heavy wizard request
                audioPlayerModalityResumeProcessor,

                //widget gallery
                widgetGalleryCollectingProcessor,
                //teasers
                teasersCollectingProcessor,
                // alice show
                morningShowGetEpisodeProcessor
            )
        )
    }

    data class ProcessorListPrototype(
        val processors: List<RunRequestProcessor<DialogovoState>>
    )
}
