package ru.yandex.alice.paskill.dialogovo.processor.request_enrichment;

import java.util.concurrent.CompletableFuture;

import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;

/**
 * Marker interface for request enrichers
 */
interface RequestEnricher<ResultType> {

    CompletableFuture<ResultType> enrichAsync(SkillProcessRequest req, Session session, SourceType source);

}
