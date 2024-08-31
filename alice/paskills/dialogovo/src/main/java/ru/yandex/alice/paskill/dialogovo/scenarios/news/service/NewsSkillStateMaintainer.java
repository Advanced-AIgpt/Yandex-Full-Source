package ru.yandex.alice.paskill.dialogovo.scenarios.news.service;

import java.time.Duration;
import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotEntityTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;

@Component
public class NewsSkillStateMaintainer {

    // Duration window after each we reset newsState.postrolledProviders
    private static final Duration POSTROLL_NEXT_AFTER_DURATION = Duration.ofHours(2);

    public DialogovoState.NewsState getNewsState(MegaMindRequest<DialogovoState> request) {
        return request.getStateO()
                .map(DialogovoState::getNewsState)
                .orElseGet(() -> new DialogovoState.NewsState(new HashMap<>(), new HashSet<>(), null));
    }

    public DialogovoState generateDialogovoState(Optional<DialogovoState> originalStateO,
                                                 DialogovoState.NewsState currNewsState,
                                                 ActivationSourceType activationSourceType,
                                                 Instant requestTime) {
        Session session;
        // сontinue session on postroll - otherwise each news activation is new session
        if (activationSourceType == ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL) {
            Optional<Session> sessionO = originalStateO.map(DialogovoState::getSession);
            session = sessionO.map(sess -> sess.getNext(activationSourceType))
                    .orElseGet(() -> Session.create(activationSourceType, 1, requestTime));
        } else {
            session = Session.create(activationSourceType, 1, requestTime);
        }

        return DialogovoState.newsSkillState(session, currNewsState);
    }

    public DialogovoState generateDialogovoState(Optional<DialogovoState> originalStateO,
                                                 DialogovoState.NewsState currNewsState,
                                                 ActivationSourceType activationSourceType,
                                                 Instant requestTime,
                                                 long appmetricaEventCounter) {
        Session session;
        // сontinue session on postroll - otherwise each news activation is new session
        if (activationSourceType == ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL) {
            Optional<Session> sessionO = originalStateO.map(DialogovoState::getSession);
            session = sessionO.map(sess -> sess.getNext(activationSourceType, appmetricaEventCounter))
                    .orElseGet(() -> Session.create(activationSourceType, 1, requestTime, appmetricaEventCounter));
        } else {
            session = Session.create(activationSourceType, 1, requestTime, appmetricaEventCounter);
        }

        return DialogovoState.newsSkillState(session, currNewsState);
    }

    public void resetLastPostrolledTimeAfterTtl(DialogovoState.NewsState currNewsState, Instant requestTime) {
        Optional<Instant> lastPostrolledTimeO = currNewsState.getLastPostrolledTimeO();
        if (lastPostrolledTimeO.isPresent()) {
            Instant lastPostrolled = lastPostrolledTimeO.get();
            if (Math.abs(requestTime.toEpochMilli() - lastPostrolled.toEpochMilli())
                    > POSTROLL_NEXT_AFTER_DURATION.toMillis()) {
                currNewsState.setLastPostrolledTime(null);
                currNewsState.getPostrolledProviders().clear();
            }
        }
    }

    public void updateNewsStateWithContent(NewsSkillInfo skillInfo,
                                           NewsFeed newsFeed,
                                           DialogovoState.NewsState currNewsState,
                                           Optional<NewsContent> newsContent) {
        newsContent.ifPresent(content ->
                currNewsState.getLastRead()
                        .computeIfAbsent(skillInfo.getId(), empty -> new HashMap<>())
                        .put(newsFeed.getId(), content.getId()));
    }

    public void updateNewsStateWithNextPostrollingProvider(DialogovoState.NewsState currNewsState,
                                                           Optional<NewsSkillInfo> postrollingNextProviderO,
                                                           Instant requestTime) {
        postrollingNextProviderO.ifPresent(suggestedProvider -> {
            currNewsState.getPostrolledProviders().add(suggestedProvider.getId());
            currNewsState.setLastPostrolledTime(requestTime);
        });
    }

    public ActivationSourceType getActivationSourceTypeOnActivate(SemanticFrame semanticFrame,
                                                                  MegaMindRequest<DialogovoState> request) {
        return semanticFrame.getTypedEntityValueO(
                        SkillsSemanticSlotTypes.ACTIVATION_SOURCE_TYPE,
                        SkillsSemanticSlotEntityTypes.ACTIVATION_SOURCE_TYPE)
                .flatMap(ActivationSourceType.R::fromValueO)
                .orElse((request.isVoiceSession() || request.getClientInfo().isYaSmartDevice()) ?
                        ActivationSourceType.DIRECT :
                        ActivationSourceType.UNDETECTED);
    }

    public ActivationSourceType getActivationSourceTypeFromSession(Optional<DialogovoState> stateO) {
        return stateO
                .map(DialogovoState::getSession)
                .map(Session::getActivationSourceType)
                .orElse(ActivationSourceType.UNDETECTED);
    }
}
