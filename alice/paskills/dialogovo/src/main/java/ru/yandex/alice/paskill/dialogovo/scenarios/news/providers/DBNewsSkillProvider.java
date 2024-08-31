package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.base.Stopwatch;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.domain.Voice;
import ru.yandex.alice.kronstadt.core.text.InflectedString;
import ru.yandex.alice.paskill.dialogovo.domain.Channel;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillIdPhrasesEntity;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.FlashBriefingType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides.NewsSkillInflectedNamesOverrides;
import ru.yandex.alice.paskill.dialogovo.utils.EnumsExt;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.GaugeInt64;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.LazyGaugeInt64;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static java.util.Objects.requireNonNullElse;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.mapping;
import static java.util.stream.Collectors.toList;
import static java.util.stream.Collectors.toUnmodifiableMap;

@Component
class DBNewsSkillProvider implements NewsSkillProvider {
    private static final Logger logger = LogManager.getLogger();

    private static final Duration SKILL_REFRESH_WARN_DURATION = Duration.ofSeconds(1);
    private static final int SKILL_CONTENTS_PER_FEED_CACHE_SIZE = 10;
    private static final int MAX_CONTENT_DELAY_GAUGE_DAYS = 10;
    private static final Set<FlashBriefingType> SUPPORTED_FLASH_BRIEFING_TYPES = Set.of(
            FlashBriefingType.RADIONEWS, FlashBriefingType.FACTS);

    private final NewsSkillsDao newsSkillsDao;
    private final NewsFeedsDao newsFeedsDao;
    private final NewsContentsDao newsContentsDao;
    private final Rate findSkillByIdCallRate;
    private final Histogram findSkillByIdDurationRate;
    private final Rate findAllActiveNewsAliceSkillRate;
    private final Histogram findAllActiveNewsAliceSkillDuration;
    private final Rate findByActivationPhrasesRate;
    private final Histogram findByActivationPhrasesDuration;
    private final Rate findAllEnabledNewsFeedsRate;
    private final Histogram findAllEnabledNewsFeedsDuration;
    private final Rate findAllNewsContentsFeedsRate;
    private final Histogram findAllNewsContentsNewsFeedsDuration;
    private final LazyGaugeInt64 onAirNotDeletedNewsSkillsGauge;
    private final GaugeInt64 newsFeedsNumberGauge;
    private final GaugeInt64 newsContentsNumberGauge;
    // Count of provider that are currently excluded from rotation cause the have stale content more than allowed
    private final GaugeInt64 staleContentProvidersGauge;
    private final GaugeInt64 actualContentProvidersGauge;
    private final NewsSkillInflectedNamesOverrides inflectedNamesOverrides;
    private final MetricRegistry metricRegistry;

    private volatile Map<String, NewsSkillInfo> skillCache = Map.of();
    private volatile boolean firstCacheWarmUpOccurred;

    DBNewsSkillProvider(
            NewsSkillsDao newsSkillsDao,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry,
            @Value("${fillCachesOnStartUp}") boolean fillCachesOnStartUp,
            NewsFeedsDao newsFeedsDao,
            NewsContentsDao newsContentsDao,
            NewsSkillInflectedNamesOverrides inflectedNamesOverrides) {
        this.newsSkillsDao = newsSkillsDao;
        this.newsFeedsDao = newsFeedsDao;
        this.newsContentsDao = newsContentsDao;

        this.findSkillByIdCallRate = metricRegistry.rate("db.postgres.query_totals", Labels.of("query_name",
                "findNewsAliceSkillById"));
        this.findSkillByIdDurationRate = metricRegistry.histogramRate("db.postgres.query_duration", Labels.of(
                "query_name", "findNewsAliceSkillById"),
                Histograms.exponential(19, 1.5d, 10.3d / 1.5d));

        this.findAllActiveNewsAliceSkillRate = metricRegistry.rate("db.postgres.query_totals", Labels.of("query_name",
                "findAllActiveNewsAliceSkill"));
        this.findAllActiveNewsAliceSkillDuration = metricRegistry.histogramRate("db.postgres.query_duration",
                Labels.of("query_name", "findAllActiveNewsAliceSkill"),
                Histograms.exponential(19, 1.5d, 10.3d / 1.5d));

        this.findByActivationPhrasesRate = metricRegistry.rate("db.postgres.query_totals", Labels.of("query_name",
                "findNewsSkillByActivationPhrases"));
        this.findByActivationPhrasesDuration = metricRegistry.histogramRate("db.postgres.query_duration",
                Labels.of("query_name", "findNewsSkillByActivationPhrases"),
                Histograms.exponential(19, 1.5d, 10.3d / 1.5d));

        this.findAllEnabledNewsFeedsRate = metricRegistry.rate("db.postgres.query_totals", Labels.of("query_name",
                "findAllEnabledNewsFeeds"));
        this.findAllEnabledNewsFeedsDuration = metricRegistry.histogramRate("db.postgres.query_duration",
                Labels.of("query_name", "findAllEnabledNewsFeeds"),
                Histograms.exponential(19, 1.5d, 10.3d / 1.5d));

        this.findAllNewsContentsFeedsRate = metricRegistry.rate("db.postgres.query_totals", Labels.of("query_name",
                "findAllNewsContentsFeedsRate"));
        this.findAllNewsContentsNewsFeedsDuration = metricRegistry.histogramRate("db.postgres.query_duration",
                Labels.of("query_name", "findAllNewsContentsNewsFeedsDuration"),
                Histograms.exponential(19, 1.5d, 10.3d / 1.5d));

        this.newsFeedsNumberGauge = metricRegistry.gaugeInt64("news.feeds.stats");
        this.newsContentsNumberGauge = metricRegistry.gaugeInt64("news.contents.stats");

        this.staleContentProvidersGauge = metricRegistry.gaugeInt64("news.stale.contents.providers");
        this.actualContentProvidersGauge = metricRegistry.gaugeInt64("news.actual.content.providers");
        this.metricRegistry = metricRegistry;

        this.onAirNotDeletedNewsSkillsGauge = metricRegistry.lazyGaugeInt64("skills.stats",
                Labels.of("state", "on_air", "channel", Channel.ALICE_NEWS_SKILL.getValue()),
                () -> skillCache.size());
        this.inflectedNamesOverrides = inflectedNamesOverrides;

        if (fillCachesOnStartUp) {
            loadCache();
        }
    }

    @Scheduled(initialDelay = 15000, fixedRate = 15000)
    void loadCache() {
        try {
            logger.trace("Preload news skills cache");

            // only nullable in tests
            Stopwatch stopwatch = Stopwatch.createStarted();

            // load skills
            findAllActiveNewsAliceSkillRate.inc();
            List<NewsSkillInfoDB> skills = requireNonNullElse(newsSkillsDao.findAllActiveAliceNewsSkill(),
                    Collections.emptyList());
            findAllActiveNewsAliceSkillDuration.record(stopwatch.elapsed().toMillis());

            // load news feeds
            findAllEnabledNewsFeedsRate.inc();
            List<NewsFeedDB> newsFeeds = requireNonNullElse(
                    newsFeedsDao.findAllEnabled(), Collections.emptyList());
            newsFeedsNumberGauge.set(newsFeeds.size());
            findAllEnabledNewsFeedsDuration.record(stopwatch.elapsed().toMillis());

            // load news feeds contents
            findAllNewsContentsFeedsRate.inc();
            Map<String, List<NewsContent>> newsContentsByFeedId = requireNonNullElse(
                    newsContentsDao.getAllNewsContentsTopPerFeed(SKILL_CONTENTS_PER_FEED_CACHE_SIZE),
                    Collections.<NewsContentDB>emptyList())
                    .stream()
                    .map(this::createNewsContent)
                    .collect(Collectors.groupingBy(NewsContent::getFeedId));
            newsContentsNumberGauge.set(newsFeeds.size());
            findAllNewsContentsNewsFeedsDuration.record(stopwatch.elapsed().toMillis());

            // process news feeds
            Map<String, List<NewsFeed>> newsFeedsBySkillId = newsFeeds
                    .stream()
                    .map(feedDB -> createNewsFeed(feedDB, newsContentsByFeedId.getOrDefault(
                            feedDB.getId(), List.of())))
                    .collect(Collectors.groupingBy(NewsFeed::getSkillId));

            // load news feed items
            Map<String, NewsSkillInfo> cache = skills.stream()
                    .map(skill -> createSkillInfo(skill, newsFeedsBySkillId))
                    .filter(skill -> SUPPORTED_FLASH_BRIEFING_TYPES.contains(skill.getFlashBriefingType()))
                    .collect(toUnmodifiableMap(NewsSkillInfo::getId, Function.identity()));

            long actualContentProvidersCount = cache.values()
                    .stream()
                    .filter(provider -> HasFreshContentProviderPredicate.INSTANCE.test(Instant.now(), provider))
                    .count();
            actualContentProvidersGauge.set(actualContentProvidersCount);
            staleContentProvidersGauge.set(cache.size() - actualContentProvidersCount);

            for (NewsSkillInfo newsSkillInfo : skillCache.values()) {
                for (NewsFeed feed : newsSkillInfo.getFeeds()) {
                    Instant mostFreshPubDate = feed.getTopContents()
                            .stream()
                            .findFirst()
                            .map(NewsContent::getPubDate)
                            // Some big lag for empty feed
                            .orElse(Instant.now().minus(MAX_CONTENT_DELAY_GAUGE_DAYS, ChronoUnit.DAYS));

                    metricRegistry.gaugeInt64("news.most.fresh.content.delay.sec",
                            Labels.of("feed", newsSkillInfo.getSlug() + "." + feed.getTopic()))
                            .set(Math.min(
                                    MAX_CONTENT_DELAY_GAUGE_DAYS * 24 * 3600,
                                    Instant.now().getEpochSecond() - mostFreshPubDate.getEpochSecond()
                            ));
                }
            }

            skillCache = cache;

            Duration allSkillsLoadTime = stopwatch.elapsed();
            logger.trace("News skills cache was loaded with {} entries in {}ms", cache.size(),
                    allSkillsLoadTime.toMillis());
            if (allSkillsLoadTime.compareTo(SKILL_REFRESH_WARN_DURATION) > 0) {
                logger.warn("News skills cache loading with {} entries took {}ms (LONG)", cache.size(),
                        allSkillsLoadTime.toMillis());
            }
            if (!firstCacheWarmUpOccurred) {
                firstCacheWarmUpOccurred = true;
            }

        } catch (Exception e) {
            logger.error("Failed to preload news cache", e);
            throw new RuntimeException("Failed to preload news skills cache", e);
        }
    }

    @Override
    public List<NewsSkillInfo> findAllActive() {
        return List.copyOf(skillCache.values());
    }

    private NewsContent createNewsContent(NewsContentDB newsContentDB) {
        return new NewsContent(
                newsContentDB.getId(),
                newsContentDB.getFeedId(),
                newsContentDB.getUid(),
                newsContentDB.getPubDate(),
                newsContentDB.getTitle(),
                newsContentDB.getStreamUrl(),
                newsContentDB.getMainText(),
                newsContentDB.getSoundId(),
                newsContentDB.getImageUrl(),
                newsContentDB.getDetailsUrl(),
                newsContentDB.getDetailsText()
        );
    }

    private NewsFeed createNewsFeed(NewsFeedDB newsFeedDB, List<NewsContent> newsContents) {
        return new NewsFeed(
                newsFeedDB.getId(),
                newsFeedDB.getSkillId(),
                newsFeedDB.getPreamble(),
                newsFeedDB.getName(),
                newsFeedDB.getTopic(),
                newsFeedDB.isEnabled(),
                // TODO: move settings to db
                1,
                newsContents
        );
    }

    @Override
    // TODO: Implement sync load
    public Optional<NewsSkillInfo> getSkill(String skillId) {
        logger.info("Getting news skill info id={} from cache", skillId);

        NewsSkillInfo fetchedSkill = skillCache.get(skillId);
        if (fetchedSkill != null) {
            logger.info("Fetched news skill info id={} from cache", skillId);
        } else {
            logger.info("Fetched news skill info id={} not found in cache", skillId);
        }

        return Optional.ofNullable(fetchedSkill);
    }

    @Override
    public Optional<NewsSkillInfo> getSkillBySlug(String slug) {
        return skillCache.values()
                .stream()
                .filter(skill -> skill.getSlug().equals(slug))
                .findFirst();
    }

    @Override
    public boolean isReady() {
        return firstCacheWarmUpOccurred;
    }

    @Override
    public Map<String, List<String>> findSkillsByPhrases(Set<String> phrases) {
        findByActivationPhrasesRate.inc();

        Stopwatch stopwatch = Stopwatch.createStarted();
        List<SkillIdPhrasesEntity> skillIds = newsSkillsDao.findByActivationPhrases(phrases.toArray(new String[0]));
        findByActivationPhrasesDuration.record(stopwatch.elapsed().toMillis());

        return skillIds.stream()
                .flatMap(skill -> skill.getInflectedActivationPhrases().stream().map(phrase -> Map.entry(phrase,
                        skill.getId().toString())))
                .filter(entry -> phrases.contains(entry.getKey()))
                .collect(groupingBy(
                        Map.Entry::getKey,
                        mapping(Map.Entry::getValue, toList())));
    }

    private NewsSkillInfo createSkillInfo(NewsSkillInfoDB skill, Map<String, List<NewsFeed>> newsFeedsBySkillId) {
        var skillFeeds = newsFeedsBySkillId.getOrDefault(skill.getId().toString(), List.of());
        Optional<NewsFeed> defaultNewsFeed = Optional.ofNullable(skill.getBackendSettings().getDefaultFeed())
                .flatMap(defFeedId -> skillFeeds.stream().filter(id -> id.getId().equals(defFeedId)).findFirst());

        return NewsSkillInfo.builder()
                .id(skill.getId().toString())
                .name(skill.getName())
                .inflectedName(getInflectedName(skill))
                .nameTts(skill.getNameTts())
                .slug(skill.getSlug())
                .channel(Channel.R.fromValueOrDefault(skill.getChannel(), Channel.UNKNOWN))
                .logoUrl(skill.getLogoUrl())
                .voice(EnumsExt.findEnum(Voice.class, x -> x.getCode().equals(skill.getVoice())).orElse(Voice.OKSANA))
                .onAir(skill.isOnAir())
                .featureFlags(new HashSet<>(skill.getFeatureFlags()))
                .userFeatureFlags(skill.getUserFeatureFlags())
                .inflectedActivationPhrases(skill.getInflectedActivationPhrases())
                .encryptedAppMetricaApiKey(skill.getAppMetricaApiKey())
                .isRecommended(Optional.ofNullable(skill.getIsRecommended()).orElse(false))
                .adBlockId(skill.getRsyPlatformId())
                .draft(skill.isDraft())
                .automaticIsRecommended(Optional.ofNullable(skill.getAutomaticIsRecommended()).orElse(false))
                .hideInStore(skill.isHideInStore())
                .defaultFeed(defaultNewsFeed.orElse(null))
                .feeds(skillFeeds)
                .flashBriefingType(Optional.ofNullable(
                        skill.getBackendSettings().getFlashBriefingType())
                        .flatMap(FlashBriefingType.R::fromValueO)
                        .orElse(FlashBriefingType.UNKNOWN))
                .build();
    }

    // TODO: move overrides to DB
    private InflectedString getInflectedName(NewsSkillInfoDB skill) {
        return inflectedNamesOverrides.getOverrideO(skill.getSlug()).orElse(InflectedString.cons(skill.getName()));
    }

    private Optional<NewsSkillInfoDB> fetchSkill(String skillId) {
        findSkillByIdCallRate.inc();
        UUID id;
        try {
            id = UUID.fromString(skillId.replace("Dialogovo:", ""));
        } catch (IllegalArgumentException e) {
            logger.error("Invalid skillId: " + skillId, e);
            return Optional.empty();
        }

        Stopwatch stopwatch = Stopwatch.createStarted();
        var skillOpt = newsSkillsDao.findAliceNewsSkillById(id)
                .filter(NewsSkillInfoDB::isOnAir);

        findSkillByIdDurationRate.record(stopwatch.elapsed().toMillis());
        return skillOpt;
    }
}
