package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.service.SocialService;

@Component
class SocialTokenRequestEnricher implements RequestEnricher<Optional<String>> {

    private final RequestContext requestContext;
    private final SocialService socialService;

    SocialTokenRequestEnricher(RequestContext requestContext, SocialService socialService) {
        this.requestContext = requestContext;
        this.socialService = socialService;
    }

    @Override
    public CompletableFuture<Optional<String>> enrichAsync(
            SkillProcessRequest req,
            Session session,
            SourceType source
    ) {
        var userId = requestContext.getCurrentUserId();
        SkillInfo skill = req.getSkill();
        Optional<String> socialAppName = skill.getSocialAppName();
        return (userId != null && socialAppName.isPresent() && source.isByUser())
                ? socialService.getSocialTokenAsync(userId, socialAppName.get())
                : CompletableFuture.completedFuture(Optional.<String>empty());
    }
}
