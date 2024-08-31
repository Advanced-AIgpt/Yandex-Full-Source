package ru.yandex.alice.paskill.dialogovo.processor;

import java.util.List;
import java.util.Optional;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowItemMeta;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;
import ru.yandex.alice.paskill.dialogovo.service.abuse.AbuseApplier;

@Component
public class ShowPullResponseHandler implements WebhookResponseHandler {
    private final AbuseApplier abuseApplier;

    @Value("${abuseConfig.enabled}")
    private boolean abuseEnabled;

    public ShowPullResponseHandler(AbuseApplier abuseApplier) {
        this.abuseApplier = abuseApplier;
    }

    @Override
    public SkillProcessResult.Builder handleResponse(
            SkillProcessResult.Builder builder,
            SkillProcessRequest req,
            Context context,
            RequestEnrichmentData enrichmentData,
            WebhookResponse webhookResponse
    ) {
        if (abuseEnabled && !req.hasExperiment(Experiments.DONT_APPLY_ABUSE)) {
            abuseApplier.apply(webhookResponse);
        }

        var skill = req.getSkill();
        var response = webhookResponse.getResponse().get();

        Optional<ShowItemMeta> show = webhookResponse.getResponse()
                .flatMap(Response::getShowItemMeta);
        builder.getLayout().setCards(List.of(new TextCard(response.getText())));
        builder.showEpisode(show)
                .skill(skill)
                .setTts(response.getTts(), skill.getVoice())
                .showEpisode(show)
                .session(req.getSession().orElse(null));
        return builder;
    }
}
