package ru.yandex.alice.paskill.dialogovo.scenarios.news.service;

import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.ReadNewsApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;

@Component
public class NewsCommitService {
    private static final Logger logger = LogManager.getLogger();

    private final NewsSkillProvider newsSkillProvider;
    private final AppMetricaEventSender appMetricaEventSender;

    public NewsCommitService(NewsSkillProvider newsSkillProvider,
                             AppMetricaEventSender appMetricaEventSender) {
        this.newsSkillProvider = newsSkillProvider;
        this.appMetricaEventSender = appMetricaEventSender;
    }

    public CommitResult commit(MegaMindRequest<DialogovoState> request, String skillId, String eventName,
                               ReadNewsApplyArguments args) {
        logger.info("Process commit with skillId [{}] event [{}]", skillId, eventName);

        Optional<NewsSkillInfo> skillO = newsSkillProvider.getSkill(skillId);

        if (skillO.isEmpty()) {
            logger.warn("News skill [{}] not found", skillId);
            return CommitResult.Success;
        }

        NewsSkillInfo skill = skillO.get();

        Session session = request.getStateO()
                .map(DialogovoState::getSession)
                .map(Session::getNext)
                .orElseGet(() -> Session.create(ActivationSourceType.UNDETECTED, request.getServerTime()));

        appMetricaEventSender.sendClientEvents(request.getClientInfo().getUuid(), args.getSkillId(),
                args.getApplyArgs().getApiKeyEncrypted(), args.getApplyArgs().getUri(),
                args.getApplyArgs().getEventEpochTime(), args.getApplyArgs().getReportMessage(),
                skill.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER));

        return CommitResult.Success;
    }
}
