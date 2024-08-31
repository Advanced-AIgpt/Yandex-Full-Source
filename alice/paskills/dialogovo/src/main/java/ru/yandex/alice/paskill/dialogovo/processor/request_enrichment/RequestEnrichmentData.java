package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.List;
import java.util.Optional;

import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillState;
import ru.yandex.alice.paskill.dialogovo.service.wizard.WizardResponse;

import static ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession.UserSkillProducts;

@Data
@AllArgsConstructor
public class RequestEnrichmentData {
    private final Nlu nlu;
    private final Optional<SkillState> skillState;
    private final Optional<String> socialToken;
    private final WizardResponse wizardResponse;
    private final List<UserSkillProducts> userSkillProductsResult;
}
