package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillState;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillStateDao;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillStateId;

@Component
class SkillStateRequestEnricher implements RequestEnricher<Optional<SkillState>> {
    private final SkillStateDao skillStateDao;
    private final RequestContext requestContext;

    SkillStateRequestEnricher(SkillStateDao skillStateDao, RequestContext requestContext) {
        this.skillStateDao = skillStateDao;
        this.requestContext = requestContext;
    }

    @Override
    public CompletableFuture<Optional<SkillState>> enrichAsync(
            SkillProcessRequest req,
            Session session,
            SourceType source
    ) {
        SkillInfo skill = req.getSkill();
        String applicationId = req.getApplicationId();
        var userId = requestContext.getCurrentUserId();
        boolean processState = skill.getUseStateStorage() && source.isByUser();
        return processState ? skillStateDao.findBySkillIdAndUserIdAndSessionIdAndApplicationIdAsync(
                new SkillStateId(skill.getId(), userId, session.getSessionId(), applicationId)
        )
                .thenApply(Optional::of)
                .orTimeout(1, TimeUnit.SECONDS)
                : CompletableFuture.completedFuture(Optional.<SkillState>empty());
    }
}
