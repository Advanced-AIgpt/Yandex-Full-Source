package ru.yandex.alice.paskill.dialogovo.scenarios;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Random;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.directive.CloseDialogDirective;
import ru.yandex.alice.kronstadt.core.directive.EndDialogSessionDirective;
import ru.yandex.alice.kronstadt.core.directive.GoBackNativeDirective;
import ru.yandex.alice.kronstadt.core.directive.HideViewDirective;
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective;
import ru.yandex.alice.kronstadt.core.directive.OpenDialogDirective;
import ru.yandex.alice.kronstadt.core.directive.TypeTextSilentDirective;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.FiltrationLevel;
import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.layout.Emoji;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.megamind.protos.common.FrameProto;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.processor.DialogovoDirectiveFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.RequestFeedbackFormDirective;
import ru.yandex.alice.paskill.dialogovo.service.api.ApiException;
import ru.yandex.alice.paskill.dialogovo.service.api.ApiService;

@Component
public class SkillLayoutsFactory {

    private static final Logger logger = LogManager.getLogger();

    private final DialogovoDirectiveFactory directiveFactory;
    private final Phrases phrases;
    private final RequestContext requestContext;
    private final ApiService apiService;

    public SkillLayoutsFactory(DialogovoDirectiveFactory directiveFactory,
                               Phrases phrases,
                               RequestContext requestContext,
                               ApiService apiService
    ) {
        this.directiveFactory = directiveFactory;
        this.phrases = phrases;
        this.requestContext = requestContext;
        this.apiService = apiService;
    }

    // TODO: implement feedback, see: https://paste.yandex-team.ru/839666
    public Layout createDeactivateSkillLayout(
            String skillId,
            String text,
            ClientInfo clientInfo,
            boolean shouldCloseWebView,
            boolean shouldStopPlayer,
            boolean silentResponse
    ) {
        List<MegaMindDirective> directives = new ArrayList<>();
        directives.add(new EndDialogSessionDirective(skillId));
        directives.add(new CloseDialogDirective(skillId));
        if (shouldCloseWebView) {
            directives.add(GoBackNativeDirective.INSTANCE);
        }
        if (shouldStopPlayer) {
            directives.addAll(directiveFactory.audioStopSequence());
            if (clientInfo.getInterfaces().getSupportsShowView()) {
                directives.add(new HideViewDirective(HideViewDirective.Layer.Companion.getCONTENT()));
            }
        }

        var builder = Layout.builder()
                .shouldListen(false)
                .directives(directives);

        if (!silentResponse) {
            builder.outputSpeech(text)
                    .textCard(text);
        }

        if (clientInfo.isHasScreenBASSimpl() && !silentResponse) {
            var thumbUp = Button.simpleText(Emoji.THUMBS_UP, TypeTextSilentDirective.THUMBS_UP);
            var thumbDown = Button.simpleText(Emoji.THUMBS_DOWN, TypeTextSilentDirective.THUMBS_DOWN);

            builder.suggests(List.of(thumbUp, thumbDown));
        }

        return builder.build();
    }

    private boolean shouldShowRateButton(ClientInfo clientInfo, SkillInfo skill) {
        if (skill.isHideInStore()) {
            logger.info("Not showing rate card: private skill");
            return false;
        }
        if (requestContext.getCurrentUserId() == null) {
            logger.info("Not showing rate card: no user id");
            return false;
        }
        if (!(clientInfo.isSearchApp() || clientInfo.isYaBrowserMobile())) {
            logger.info("Not showing rate card: surface not supported");
            return false;
        }
        @Nullable String userTicket = requestContext.getCurrentUserTicket();
        if (userTicket != null) {
            Optional<FeedbackMark> mark;
            try {
                mark = apiService.getFeedbackMarkO(userTicket, skill.getId(), clientInfo.getUuid());
            } catch (ApiException e) {
                logger.error("Not showing rate card: failed to get skill review from API", e);
                return false;
            }
            if (mark.isPresent()) {
                logger.info("Not showing rate card: review exists");
                return false;
            }
        }
        logger.info("showing rate card");
        return true;
    }

    @SuppressWarnings("ParameterNumber")
    public Layout createOpenDialogLayout(
            boolean isVoiceSession,
            SkillInfo skill,
            Optional<String> requestO,
            Optional<String> originalUtteranceO,
            Optional<ActivationSourceType> activationSourceTypeO,
            Optional<String> payload,
            FrameProto.TTypedSemanticFrame activationTypedSemanticFrame,
            Instant serverTime,
            ClientInfo clientInfo
    ) {
        OpenDialogDirective openDialogDirective = directiveFactory.openDialogDirective(
                skill,
                requestO,
                originalUtteranceO,
                activationSourceTypeO,
                payload,
                activationTypedSemanticFrame
        );

        String buttonTitle = originalUtteranceO
                .map(StringUtils::capitalize)
                .orElse(phrases.get("activate.button.title"));
        final Button startSkillButton = Button.simpleText(buttonTitle, openDialogDirective);

        final List<Button> buttons;
        if (shouldShowRateButton(clientInfo, skill)) {
            RequestFeedbackFormDirective requestFeedbackDirective = new RequestFeedbackFormDirective(
                    skill.getId(),
                    serverTime);
            final Button rateButton = Button.simpleText(phrases.get("rate.button.title"), requestFeedbackDirective);
            buttons = List.of(startSkillButton, rateButton);
        } else {
            buttons = List.of(startSkillButton);
        }

        return Layout.builder()
                .outputSpeech(isVoiceSession ? "Запускаю" : "")
                .textCard("Запускаю навык «" + skill.getName() + "»", buttons)
                .shouldListen(false)
                .directives(List.of(openDialogDirective))
                .build();
    }

    public Layout createAwaitVoiceConfirmLayout(String text, String tts) {
        return Layout
                .builder()
                .outputSpeech(tts)
                .textCard(text)
                .shouldListen(true)
                .suggests(List.of(
                        Button.simpleTextWithTextDirective(phrases.get("suggest.confirm.yes")),
                        Button.simpleTextWithTextDirective(phrases.get("suggest.confirm.no"))))
                .build();
    }

    public Layout createSuggestUserDeclineAnswerLayout(Random random) {
        String text = phrases.getRandom("station.discovery.suggest.user.declined.answer", random);
        return Layout
                .builder()
                .outputSpeech(text)
                .textCard(text)
                .shouldListen(false)
                .build();
    }

    public Layout createExplicitContentDenyLayout(FiltrationLevel filtrationLevel, Random random) {
        String text;
        if (filtrationLevel == FiltrationLevel.SAFE) {
            text = phrases.getRandom("activate.skill.explicit.content.safe.level", random);
        } else {
            text = phrases.getRandom("activate.skill.explicit.content.family.level", random);
        }
        return Layout
                .builder()
                .outputSpeech(text)
                .textCard(text)
                .shouldListen(false)
                .build();
    }
}
