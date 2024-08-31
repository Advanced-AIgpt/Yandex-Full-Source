package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.concurrent.CompletableFuture;

import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.service.ner.NerService;

@Component
class NluRequestEnricher implements RequestEnricher<Nlu> {
    private final NerService nerService;

    NluRequestEnricher(NerService nerService) {
        this.nerService = nerService;
    }

    @Override
    public CompletableFuture<Nlu> enrichAsync(SkillProcessRequest req, Session session, SourceType source) {
        var originalUtterance = req.getOriginalUtterance();
        SkillInfo skill = req.getSkill();
        boolean isNluEnabled = req.hasExperiment(Experiments.ENABLE_NER_FOR_SKILLS) || req.getSkill().isUseNlu();
        return (isNluEnabled && originalUtterance.isPresent() && source.isByUser())
                ? nerService.getNluAsync(originalUtterance.get(), skill.getId())
                : CompletableFuture.completedFuture(Nlu.EMPTY);
    }
}
