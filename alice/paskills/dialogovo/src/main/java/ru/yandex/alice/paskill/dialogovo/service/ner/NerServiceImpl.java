package ru.yandex.alice.paskill.dialogovo.service.ner;

import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import javax.annotation.Nullable;

import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.CacheStats;
import com.google.common.cache.LoadingCache;
import lombok.AllArgsConstructor;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.NerApiConfig;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;

import static ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService.TIMEOUT_DELTA;

class NerServiceImpl implements NerService {
    private static final Logger logger = LogManager.getLogger();

    private final RestTemplate restTemplate;
    private final DialogovoInstrumentedExecutorService executor;
    private final LoadingCache<String, Nlu> cache;
    private final NerApiConfig nerConfig;
    private final long retryCount;

    NerServiceImpl(NerApiConfig nerConfig,
                   RestTemplate restTemplate,
                   DialogovoInstrumentedExecutorService executor,
                   long retryCount) {
        this.executor = executor;
        this.restTemplate = restTemplate;
        this.nerConfig = nerConfig;
        this.cache = CacheBuilder.newBuilder()
                .expireAfterWrite(Duration.ofSeconds(nerConfig.getCacheTtlSeconds()))
                .maximumSize(nerConfig.getCacheSize())
                .recordStats()
                .build(new CacheLoader<String, Nlu>() {
                    @Override
                    public Nlu load(String key) {
                        return fetchFromWizard(key);
                    }
                });
        this.retryCount = retryCount;
    }

    CacheStats getCacheStats() {
        return this.cache.stats();
    }

    @Override
    public Nlu getNlu(@Nullable String utterance, String skillId) {
        if (utterance == null || utterance.isEmpty()) {
            return Nlu.EMPTY;
        }
        try {
            return cache.get(utterance);
        } catch (ExecutionException e) {
            logger.error("Unable to get nlu: utterance=" + utterance + " skillId=" + skillId, e.getCause() != null ?
                    e.getCause() : e);
            return Nlu.EMPTY;
        } catch (Exception e) {
            logger.error("Unable to get nlu: utterance=" + utterance + " skillId=" + skillId, e);
            return Nlu.EMPTY;
        }
    }

    private Nlu fetchFromWizard(String utterance) {

        var result = restTemplate.postForObject(nerConfig.getUrl(), new NluRequest(utterance), NluResponse.class);

        return result != null ? result.nlu : Nlu.EMPTY;
    }

    @Override
    public CompletableFuture<Nlu> getNluAsync(@Nullable String utterance, String skillId) {
        return executor.supplyAsyncInstrumented(
                () -> getNlu(utterance, skillId),
                Duration.ofMillis(retryCount * nerConfig.getTimeout() + TIMEOUT_DELTA)
        );
    }

    @Data
    @AllArgsConstructor
    private static class NluRequest {
        private String utterance;
    }

    @Data
    @AllArgsConstructor
    private static class NluResponse {
        private Nlu nlu;
    }
}
