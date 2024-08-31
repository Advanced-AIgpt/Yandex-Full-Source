package ru.yandex.alice.paskill.dialogovo.service.show;

import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.Interfaces;
import ru.yandex.alice.kronstadt.core.layout.TextCard;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto;
import ru.yandex.alice.paskill.dialogovo.config.ShowConfig;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.domain.show.MorningShowEpisodeEntity;
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowEpisodeEntity;
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowInfo;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.show.ShowPullRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowItemMeta;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.ShowProvider;
import ru.yandex.alice.paskill.dialogovo.utils.VoiceUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskills.common.solomon.utils.Instrument;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.GaugeInt64;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

public class ShowServiceImpl implements ShowService {
    private static final String SYSTEM_UUID = "4E148F90712F1EC2EE67301BCF579E4E4E256ED91B789947C22E2EC3FA7CA98D";
    private static final int MAX_CONTENT_DELAY_GAUGE_DAYS = 30;
    private static final Logger logger = LogManager.getLogger();
    private final ShowProvider showProvider;
    private final SkillRequestProcessor skillRequestProcessor;
    private final MorningShowEpisodeDao morningShowEpisodeDao;

    private final ShowEpisodeStoreDao store;
    private final DialogovoInstrumentedExecutorService executorFetchService;
    private final Instrument ytInstrument;
    private final Duration skillRequestTimeout;
    private final Set<String> experiments;

    private final MetricRegistry metricRegistry;
    private final GaugeInt64 numberOfActiveMorningShowSkills;
    private final GaugeInt64 numberOfUnpersonalizedMorningShowEpisodes;
    private final Rate getPersonalEpisodeFromCacheFailureCounter;

    @SuppressWarnings("ParameterNumber")
    public ShowServiceImpl(
            ShowProvider showProvider,
            SkillRequestProcessor skillRequestProcessor,
            ShowEpisodeStoreDao store,
            DialogovoInstrumentedExecutorService executorFetchService,
            ShowConfig showConfig,
            MetricRegistry metricRegistry,
            MorningShowEpisodeDao morningShowEpisodeDao
    ) {
        this.showProvider = showProvider;
        this.skillRequestProcessor = skillRequestProcessor;
        this.morningShowEpisodeDao = morningShowEpisodeDao;
        this.store = store;
        this.executorFetchService = executorFetchService;
        this.skillRequestTimeout = Duration.ofMillis(showConfig.getSkillRequestTimeout());
        this.experiments = new HashSet<>(showConfig.getExperiments());

        this.numberOfActiveMorningShowSkills = metricRegistry.gaugeInt64(
                "morning_show.stats",
                Labels.of("entity", "morning_shows")
        );
        this.numberOfUnpersonalizedMorningShowEpisodes = metricRegistry.gaugeInt64(
                "morning_show.stats",
                Labels.of("entity", "morning_show_episodes")
        );
        getPersonalEpisodeFromCacheFailureCounter = metricRegistry.rate("get_personalized_episode_from_cache_failure");

        this.ytInstrument = new Instrument(new NamedSensorsRegistry(metricRegistry, "morning_show.yt_save"));
        this.metricRegistry = metricRegistry;
    }

    @Override
    public List<String> getPersonalizedShowConfigAndStartPrepare(
            ShowType showType, List<String> mementoConfigSkillIds, ClientInfo clientInfo
    ) {
        List<String> skillIds = new ArrayList<>(mementoConfigSkillIds);
        skillIds.retainAll(showProvider.getActivePersonalizedShowSkills(showType).stream().map(
                info -> info.getSkillInfo().getId()).toList());
        try {
            executorFetchService.execute(() -> preparePersonalizedShows(showType, skillIds, clientInfo));
        } catch (RejectedExecutionException ex) {
            logger.error("Error preparing personalized show episodes(skill ids: {}) for user(id: {})",
                    skillIds, clientInfo.getUuid(), ex);
        }
        return skillIds;
    }

    private void preparePersonalizedShows(ShowType showType, List<String> skillIds, ClientInfo clientInfo) {
        Instant now = Instant.now();
        Context context = new Context(SourceType.SYSTEM);

        CompletableFuture<ShowEpisodeEntity>[] skillPullAndSaveFutures = new CompletableFuture[skillIds.size()];
        for (int i = 0; i < skillIds.size(); i++) {
            Optional<ShowInfo> showInfoO = showProvider.getShowFeedBySkillId(skillIds.get(i), showType);
            if (showInfoO.isEmpty()) {
                logger.error(String.format("Can't find showInfo for skill(id: %s)", skillIds.get(i)));
                continue;
            }
            skillPullAndSaveFutures[i] = pullAndSavePersonalizedShowEpisode(showInfoO.get(), now, context, clientInfo);
        }

        try {
            // Таймаут выставлен на задачу, и у каждой подзадачи также выставлен таймаут.
            CompletableFuture.allOf(skillPullAndSaveFutures)
                    .orTimeout(skillRequestTimeout.toMillis(), TimeUnit.MILLISECONDS)
                    .exceptionally(throwable -> {
                        logger.error("Can't process updating of personalized {} show episodes", showType, throwable);
                        return null;
                    }).get(skillRequestTimeout.toMillis(), TimeUnit.MILLISECONDS);
        } catch (ExecutionException | TimeoutException | InterruptedException e) {
            logger.error("FetchAll personalized {} show episodes future got exception", showType, e);
        }
    }

    @Override
    public void updateUnpersonalizedShows(ShowType showType) {
        Instant now = Instant.now();
        Context context = new Context(SourceType.SYSTEM);
        List<ShowInfo> showInfos = showProvider.getActiveShowSkills(showType);
        numberOfActiveMorningShowSkills.set(showInfos.size());

        CompletableFuture<ShowEpisodeEntity>[] skillRequestFutures = new CompletableFuture[showInfos.size()];
        for (int i = 0; i < showInfos.size(); i++) {
            ShowInfo showInfo = showInfos.get(i);
            skillRequestFutures[i] = pullShowEpisode(showInfo, now, context, null);
        }

        CompletableFuture<Void> fetchAllFuture = CompletableFuture.allOf(skillRequestFutures)
                .orTimeout(skillRequestTimeout.toMillis(), TimeUnit.MILLISECONDS)
                .exceptionally(throwable -> {
                    logger.error("Can't process updating of unpersonalized {} show episodes", showType, throwable);
                    return null;
                });
        try {
            fetchAllFuture.get(skillRequestTimeout.toMillis(), TimeUnit.MILLISECONDS); // таймаут выставлен на
            // задачу, и у каждой подзадачи
            // также выставлен таймаут.
        } catch (ExecutionException | TimeoutException | InterruptedException e) {
            logger.error("FetchAll unpersonalized {} show episodes future got exception", showType, e);
        }
        if (fetchAllFuture.isCompletedExceptionally()) {
            numberOfUnpersonalizedMorningShowEpisodes.set(0L);
            return;
        }

        List<ShowEpisodeEntity> episodes = Arrays.stream(skillRequestFutures)
                .map(f -> f.getNow(null))
                .filter(Objects::nonNull)
                .collect(Collectors.toList());

        episodes.forEach(this::collectUnpersonalizedEpisodeMetrics);

        numberOfUnpersonalizedMorningShowEpisodes.set(episodes.size());

        if (!episodes.isEmpty()) {
            // Deprecated storing to YT, look PASKILLS-8251
            logger.info("Storing {} new episodes of {} show to YT", episodes.size(), showType);
            ytInstrument.time(() -> {
                try {
                    store.storeSyncShowEpisodeEntity(episodes, now);
                    logger.info("Successfully saved new episodes of {} shows", showType);
                } catch (Throwable throwable) {
                    logger.error("Failed to save new episodes of {} shows", showType, throwable);
                }
            });
            // Store to YDB
            if (showType == ShowType.MORNING) {
                logger.info("Storing {} new unpersonalized episodes of {} show to YDB", episodes.size(), showType);
                episodes.forEach((episode ->
                        morningShowEpisodeDao.storeEpisodeAsync(episode.toMorningShowEpisodeEntity(null)))
                );
            }
        } else {
            logger.info("No new episodes of {} show to store", showType);
        }
    }

    private CompletableFuture<ShowEpisodeEntity> pullAndSavePersonalizedShowEpisode(ShowInfo showInfo,
                                                                                    Instant now,
                                                                                    Context context,
                                                                                    ClientInfo clientInfo) {
        if (showInfo.getShowType() != ShowType.MORNING) {
            logger.error("Can't update episode of skill(id: {}). Unknown show type: {}",
                    showInfo.getSkillInfo().getId(), showInfo.getShowType());
            return CompletableFuture.completedFuture(null);
        }

        // If cached personal episode exists, and it won't expire soon, we return it
        MorningShowEpisodeEntity cachedMorningShowEpisode =
                morningShowEpisodeDao.getEpisode(showInfo.getSkillInfo().getId(), clientInfo.getUuid(), null);
        if (cachedMorningShowEpisode != null) {
            ShowEpisodeEntity showEpisodeEntity = cachedMorningShowEpisode.toShowEpisodeEntity();
            if (isEpisodeNotExpired(showEpisodeEntity)) {
                return CompletableFuture.completedFuture(showEpisodeEntity);
            }
        }

        CompletableFuture<ShowEpisodeEntity> newEpisodeFuture = pullShowEpisode(showInfo, now, context, clientInfo);
        return newEpisodeFuture.thenApply((ShowEpisodeEntity episode) -> {
                    if (episode != null) {
                        morningShowEpisodeDao.storeEpisodeAsync(
                                episode.toMorningShowEpisodeEntity(clientInfo.getUuid())
                        );
                    }
                    return episode;
                }
        );
    }

    private boolean isEpisodeNotExpired(ShowEpisodeEntity episode) {
        return episode.getExpirationDate() == null ||
                episode.getExpirationDate().isAfter(
                        Instant.now().plus(EPISODE_WILL_REMAIN_VALID_THRESHOLD.toMillis(), ChronoUnit.MILLIS)
                );
    }

    private CompletableFuture<ShowEpisodeEntity> pullShowEpisode(
            ShowInfo showInfo,
            Instant now,
            Context context,
            @Nullable ClientInfo clientInfo
    ) {
        return executorFetchService.supplyAsyncInstrumented(
                () -> processResponse(
                        showInfo, skillRequestProcessor.process(context, createRequest(showInfo, now, clientInfo))
                ),
                skillRequestTimeout,
                () -> {
                    logger.error("Timeout on pulling {} show from skill {}", showInfo.getShowType(),
                            showInfo.getSkillInfo().getId());
                    return null;
                }
        ).exceptionally(ex -> {
            logger.error("Can't request skill {} for {} show episodes",
                    showInfo.getSkillInfo().getId(), showInfo.getShowType(), ex);
            return null;
        });
    }

    /**
     * Returns valid personalized episode from cache. If none exists, return valid unpersonalized episode.
     * Return empty optional if there is no valid personalized or unpersonalized episode from skill.
     */
    @Override
    public Optional<ShowEpisodeEntity> getRelevantEpisode(ShowType showType, String skillId, ClientInfo clientInfo) {
        String userId = clientInfo.getUuid();
        if (showType != ShowType.MORNING) {
            logger.error("Unknown show type: {}", showType);
            return Optional.empty();
        }

        Optional<ShowInfo> showInfoO = showProvider.getShowFeedBySkillId(skillId, showType);
        if (showInfoO.isPresent() && showInfoO.get().getPersonalizationEnabled()) {
            MorningShowEpisodeEntity cachedPersonalizedEpisode =
                    morningShowEpisodeDao.getEpisode(skillId, userId, null);
            if (cachedPersonalizedEpisode != null) {
                return Optional.of(cachedPersonalizedEpisode.toShowEpisodeEntity());
            }
            getPersonalEpisodeFromCacheFailureCounter.inc();
            logger.error("Valid personalized episode from skill(id: {}) for user(id: {}) not present in cache!",
                    skillId, userId);
        }

        MorningShowEpisodeEntity cachedUnpersonalizedEpisode =
                morningShowEpisodeDao.getUnpersonalizedEpisode(skillId, null);
        if (cachedUnpersonalizedEpisode != null) {
            return Optional.of(cachedUnpersonalizedEpisode.toShowEpisodeEntity());
        }
        logger.error("Valid unpersonalized episode from skill(id: {}) for user(id: {}) not present in cache!",
                skillId, userId);
        return Optional.empty();
    }

    private void collectUnpersonalizedEpisodeMetrics(ShowEpisodeEntity episode) {
        metricRegistry.gaugeInt64("show.episodes.fresh.content.delay.sec",
                        Labels.of("skill_slug", episode.getSkillSlug()))
                .set(Math.min(
                        MAX_CONTENT_DELAY_GAUGE_DAYS * 24 * 3600,
                        Instant.now().getEpochSecond() - episode.getPublicationDate().getEpochSecond()
                ));
    }

    private SkillProcessRequest createRequest(ShowInfo showInfo, Instant instant, @Nullable ClientInfo clientInfo) {
        return SkillProcessRequest.builder()
                .clientInfo(Objects.requireNonNullElseGet(
                        clientInfo,
                        () -> new ClientInfo(
                                null,
                                null,
                                null,
                                null,
                                SYSTEM_UUID,
                                null,
                                "ru-RU",
                                "Europe/Moscow",
                                null,
                                null,
                                new Interfaces()
                        )))
                .session(Optional.empty())
                .locationInfo(Optional.empty())
                .normalizedUtterance("")
                .originalUtterance("")
                .viewState(Optional.empty())
                .activationSourceType(ActivationSourceType.SCHEDULER)
                .skill(showInfo.getSkillInfo())
                .requestTime(instant)
                .showPullRequest(new ShowPullRequest(showInfo.getShowType()))
                .experiments(experiments)
                .mementoData(RequestProto.TMementoData.getDefaultInstance())
                .build();
    }

    private ShowEpisodeEntity processResponse(ShowInfo showInfo, SkillProcessResult resp) {
        ShowItemMeta itemMeta = resp.getShowEpisode().get();
        TextCard joinedTextCard = Objects.requireNonNull(resp.getLayout().joinTextCards());
        String responseText = joinedTextCard.getText();

        String textTts = Optional.ofNullable(resp.getRawTts())
                .orElseGet(() -> VoiceUtils.normalize(responseText));

        String titleTts = Optional.ofNullable(itemMeta.getTitleTts())
                .orElseGet(() -> Optional.ofNullable(itemMeta.getTitle())
                        .map(VoiceUtils::normalize)
                        .orElse(null)
                );

        String showText = itemMeta.getTitle() != null ? itemMeta.getTitle() + "\n" + responseText : responseText;
        String showTts = titleTts != null && !titleTts.isBlank() ? titleTts + " sil<[500]> " + textTts : textTts;

        return new ShowEpisodeEntity(
                itemMeta.getId(),
                showInfo.getSkillInfo().getId(),
                showInfo.getSkillInfo().getSlug(),
                showInfo.getShowType(),
                showText, showTts,
                itemMeta.getPublicationDate(),
                itemMeta.getExpirationDate()
        );
    }
}
