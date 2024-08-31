package ru.yandex.alice.paskill.dialogovo.processor;

import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData;

interface WebhookResponseHandler {
    SkillProcessResult.Builder handleResponse(SkillProcessResult.Builder builder,
                                              SkillProcessRequest request,
                                              Context context,
                                              RequestEnrichmentData requestEnrichment,
                                              WebhookResponse response);
}
