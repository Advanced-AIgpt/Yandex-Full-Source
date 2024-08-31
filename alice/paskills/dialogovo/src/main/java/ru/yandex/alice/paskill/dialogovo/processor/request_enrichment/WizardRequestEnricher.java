package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;

import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;

import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardResponse;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardService;

@Component
class WizardRequestEnricher implements RequestEnricher<WizardResponse> {

    private final WizardService wizardService;

    WizardRequestEnricher(WizardService wizardService) {
        this.wizardService = wizardService;
    }

    @Override
    public CompletableFuture<WizardResponse> enrichAsync(SkillProcessRequest req, Session session, SourceType source) {
        SkillInfo skill = req.getSkill();
        Optional<String> originalUtterance = req.getOriginalUtterance();
        Optional<String> normalizedUtterance = req.getNormalizedUtterance();
        Optional<String> wizardRequest = originalUtterance
                .filter(utterance -> !StringUtils.isEmpty(utterance))
                .or(() -> normalizedUtterance);

        boolean hasGrammar = skill.getGrammarsBase64().isPresent() && source.isByUser();
        boolean hasProactiveExitFeature = req.getClientInfo().isYaSmartDevice()
                && source == SourceType.USER;
        boolean shouldAskWizard = hasGrammar || hasProactiveExitFeature;

        return shouldAskWizard
                ? wizardService.requestWizardAsync(wizardRequest, skill.getGrammarsBase64())
                : CompletableFuture.<WizardResponse>completedFuture(WizardResponse.EMPTY);
    }
}
