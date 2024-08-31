package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse
import ru.yandex.alice.paskill.dialogovo.megamind.AbstractProcessorBasedScenario
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoApplyArgumentsConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoMegaMindRequestListener
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoStateConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.Scenarios

@Component
internal class FlashBriefingScenario(
    applyArgumentsConverter: DialogovoApplyArgumentsConverter,
    activateSkillNewsBySubscriptionRunProcessor: ActivateSkillNewsBySubscriptionRunProcessor,
    morningShowNewsProcessor: MorningShowNewsProcessor,
    activateSkillNewsFromProviderRunProcessor: ActivateSkillNewsFromProviderRunProcessor,
    activateFactOfTheDayRunProcessor: ActivateFactOfTheDayRunProcessor,
    readNextNewsCallbackProcessor: ReadNextNewsCallbackProcessor,
    readPrevNewsCallbackProcessor: ReadPrevNewsCallbackProcessor,
    repeatLastNewsCallbackProcessor: RepeatLastNewsCallbackProcessor,
    repeatAllNewsCallbackProcessor: RepeatAllNewsCallbackProcessor,
    newsGetDetailsCallbackProcessor: NewsGetDetailsCallbackProcessor,
    newsSendDetailsLinkCallbackProcessor: NewsSendDetailsLinkCallbackProcessor,
    radionewsOnboardingRunProcessor: RadionewsOnboardingRunProcessor,
    newsSubscriptionConfirmCallbackProcessor: NewsSubscriptionConfirmCallbackProcessor,
    newsSubscriptionDeclineCallbackProcessor: NewsSubscriptionDeclineCallbackProcessor,
    activateKidsNewsRunProcessor: ActivateKidsNewsRunProcessor,
    activateKidsNewsOnWideRequestSafeModeRunProcessor: ActivateKidsNewsOnWideRequestSafeModeRunProcessor,
    dialogovoStateConverter: DialogovoStateConverter,
    listener: DialogovoMegaMindRequestListener,
) : AbstractProcessorBasedScenario<DialogovoState>(
    Scenarios.FLASHBRIEFINGS,
    runRequestProcessors = listOf(
        morningShowNewsProcessor,
        activateSkillNewsBySubscriptionRunProcessor,
        activateFactOfTheDayRunProcessor,
        activateSkillNewsFromProviderRunProcessor,
        readNextNewsCallbackProcessor,
        readPrevNewsCallbackProcessor,
        repeatLastNewsCallbackProcessor,
        repeatAllNewsCallbackProcessor,
        newsGetDetailsCallbackProcessor,
        newsSendDetailsLinkCallbackProcessor,
        radionewsOnboardingRunProcessor,
        newsSubscriptionConfirmCallbackProcessor,
        newsSubscriptionDeclineCallbackProcessor,
        activateKidsNewsRunProcessor,
        activateKidsNewsOnWideRequestSafeModeRunProcessor
    ),
    stateConverter = dialogovoStateConverter,
    applyArgumentsConverter = applyArgumentsConverter,
    megamindRequestListeners = listOf(listener),
    irrelevantResponseFactory = DefaultIrrelevantResponse.Factory("external_skill.news.irrelevant")
)
