package ru.yandex.alice.paskill.dialogovo.service.normalizer;

import java.time.Duration;

import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.CacheStats;
import com.google.common.cache.LoadingCache;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.nlu.libs.fstnormalizer.FstNormalizer;
import ru.yandex.alice.nlu.libs.fstnormalizer.Lang;

public class NormalizationServiceImpl implements NormalizationService {
    private static final Logger logger = LogManager.getLogger(NormalizationServiceImpl.class);
    private static final Duration CACHE_EXPIRATION = Duration.ofDays(1L);
    private static final long CACHE_SIZE = 25_000L;
    private final FstNormalizer normalizer;
    private final LoadingCache<String, String> cache;

    public NormalizationServiceImpl() {
        this.normalizer = new FstNormalizer();
        this.cache = CacheBuilder.newBuilder()
                .expireAfterWrite(CACHE_EXPIRATION)
                .maximumSize(CACHE_SIZE)
                .recordStats()
                .build(new CacheLoader<String, String>() {
                    @Override
                    public String load(String key) {
                        return normalizer.normalize(Lang.RUS, key);
                    }
                });
    }

    CacheStats getCacheStats() {
        return this.cache.stats();
    }

    @Override
    public String normalize(String text) {
        try {
            return cache.get(text);
        } catch (Exception e) {
            logger.error("Failed to normalize text: [" + text + "]", e);
            return text;
        }
    }
}
