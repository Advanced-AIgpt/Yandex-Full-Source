package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.stream.Collectors;

import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession.UserSkillProducts;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;

@Component
public class EnrichmentDataProvider {

    private final NluRequestEnricher nluRequestEnricher;
    private final SkillStateRequestEnricher skillStateRequestEnricher;
    private final SocialTokenRequestEnricher socialTokenRequestEnricher;
    private final WizardRequestEnricher wizardRequestEnricher;
    private final UserSkillProductsEnricher userSkillProductsEnricher;

    EnrichmentDataProvider(NluRequestEnricher nluRequestEnricher,
                           SkillStateRequestEnricher skillStateRequestEnricher,
                           SocialTokenRequestEnricher socialTokenRequestEnricher,
                           WizardRequestEnricher wizardRequestEnricher,
                           UserSkillProductsEnricher userSkillProductsEnricher) {
        this.nluRequestEnricher = nluRequestEnricher;
        this.skillStateRequestEnricher = skillStateRequestEnricher;
        this.socialTokenRequestEnricher = socialTokenRequestEnricher;
        this.wizardRequestEnricher = wizardRequestEnricher;
        this.userSkillProductsEnricher = userSkillProductsEnricher;
    }

    public RequestEnrichmentData enrichRequest(SkillProcessRequest req, Session session, SourceType source) {
        var nluFuture = nluRequestEnricher.enrichAsync(req, session, source);
        var skillStateFuture = skillStateRequestEnricher.enrichAsync(req, session, source);
        var socialTokenFuture = socialTokenRequestEnricher.enrichAsync(req, session, source);
        var wizardFuture = wizardRequestEnricher.enrichAsync(req, session, source);
        var userSkillProductsFuture = userSkillProductsEnricher.enrichAsync(req, session, source);
        return new RequestEnrichmentData(
                nluFuture.join(),
                skillStateFuture.join(),
                socialTokenFuture.join(),
                wizardFuture.join(),
                userSkillProductsFuture.join().getSkillProducts().stream()
                                .map(product -> new UserSkillProducts(product.getUuid(), product.getName()))
                                .collect(Collectors.toList())
        );
    }
}
