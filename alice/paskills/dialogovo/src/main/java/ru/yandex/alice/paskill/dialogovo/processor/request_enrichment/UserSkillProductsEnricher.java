package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.Collections;
import java.util.concurrent.CompletableFuture;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.UserFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.service.billing.BillingService;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductsResult;

@Component
class UserSkillProductsEnricher implements RequestEnricher<UserSkillProductsResult> {
    private final BillingService billingService;
    private final RequestContext requestContext;

    UserSkillProductsEnricher(
            BillingService billingService,
            RequestContext requestContext
    ) {
        this.billingService = billingService;
        this.requestContext = requestContext;
    }

    @Override
    public CompletableFuture<UserSkillProductsResult> enrichAsync(
            SkillProcessRequest req,
            Session session,
            SourceType source
    ) {
        var userId = requestContext.getCurrentUserId();
        SkillInfo skill = req.getSkill();
        return (userId != null && skill.hasUserFeatureFlag(UserFeatureFlag.USER_SKILL_PRODUCT) && source.isByUser())
                ? billingService.getUserSkillProductsAsync(skill.getId())
                : CompletableFuture.completedFuture(new UserSkillProductsResult(Collections.emptyList()));
    }
}
