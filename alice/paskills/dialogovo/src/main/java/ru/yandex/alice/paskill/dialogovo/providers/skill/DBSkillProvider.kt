package ru.yandex.alice.paskill.dialogovo.providers.skill

import com.google.common.base.Stopwatch
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.scheduling.annotation.Scheduled
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.domain.Surface
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.paskill.dialogovo.domain.Channel
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo.ActivationExample
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo.SkillAccess
import ru.yandex.alice.paskill.dialogovo.domain.UserAgreement
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillInfoDB.StructuredExample
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters
import ru.yandex.alice.paskill.dialogovo.solomon.CallMeter
import ru.yandex.alice.paskill.dialogovo.solomon.measureMillis
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.util.Optional
import java.util.UUID

@Component
internal class DBSkillProvider(
    @Value("\${storeUrl}")
    private val storeUrl: String,
    private val skillsDao: SkillsDao,
    private val userAgreementDao: UserAgreementDao,
    private val showFeedDao: ShowFeedDao,
    metricRegistry: MetricRegistry,
    @Value("\${fillCachesOnStartUp}")
    fillCachesOnStartUp: Boolean,
) : SkillProvider, UserAgreementProvider, ShowProvider {

    private val findSkillById = CallMeter(
        metricRegistry.rate(
            "db.postgres.query_totals", Labels.of("query_name", "findAliceSkillById")
        ),
        metricRegistry.histogramRate(
            "db.postgres.query_duration", Labels.of("query_name", "findAliceSkillById"),
            Histograms.exponential(19, 1.5, 10.3 / 1.5)
        )
    )

    private val findAllActiveAliceSkillRate = metricRegistry.rate(
        "db.postgres.query_totals", Labels.of("query_name", "findAllActiveAliceSkill")
    )
    private val findAllActiveAliceSkillDuration = metricRegistry.histogramRate(
        "db.postgres.query_duration", Labels.of("query_name", "findAllActiveAliceSkill"),
        Histograms.exponential(19, 1.5, 10.3 / 1.5)
    )

    private val findByActivationPhrases = CallMeter(
        metricRegistry.rate(
            "db.postgres.query_totals", Labels.of("query_name", "findByActivationPhrases")
        ),
        metricRegistry.histogramRate(
            "db.postgres.query_duration", Labels.of("query_name", "findByActivationPhrases"),
            Histograms.exponential(19, 1.5, 10.3 / 1.5)
        )
    )

    private val findSkillDraftById = CallMeter(
        metricRegistry.rate(
            "db.postgres.query_totals", Labels.of("query_name", "findAliceSkillDraftById")
        ),
        metricRegistry.histogramRate(
            "db.postgres.query_duration", Labels.of("query_name", "findAliceSkillDraftById"),
            Histograms.exponential(19, 1.5, 10.3 / 1.5)
        )
    )

    private val loadUserAgreementsMeter = CallMeter(
        metricRegistry.rate(
            "db.postgres.query_totals", Labels.of("query_name", "findAllPublishedUserAgreements")
        ),
        metricRegistry.histogramRate(
            "db.postgres.query_duration", Labels.of("query_name", "findAllPublishedUserAgreements"),
            Histograms.exponential(19, 1.5, 10.3 / 1.5)
        )
    )

    private val loadShowFeedsMeter = CallMeter(
        metricRegistry.rate(
            "db.postgres.query_totals", Labels.of("query_name", "findAllActiveShowFeeds")
        ),
        metricRegistry.histogramRate(
            "db.postgres.query_duration", Labels.of("query_name", "findAllActiveShowFeeds"),
            Histograms.exponential(19, 1.5, 10.3 / 1.5)
        )
    )

    private val findDraftUserAgreementsMeter = CallMeter(
        metricRegistry.rate(
            "db.postgres.query_totals", Labels.of("query_name", "findDraftUserAgreements")
        ),
        metricRegistry.histogramRate(
            "db.postgres.query_duration", Labels.of("query_name", "findDraftUserAgreements"),
            Histograms.exponential(19, 1.5, 10.3 / 1.5)
        )
    )

    private val onAirNotDeletedSkillsGauge = metricRegistry.lazyGaugeInt64(
        "skills.stats",
        Labels.of("state", "on_air", "channel", Channel.ALICE_SKILL.value())
    ) { skillCache.size.toLong() }
    private val onAirNotDeletedRecommendedSkillsGauge = metricRegistry.lazyGaugeInt64(
        "skills.stats",
        Labels.of("state", "recommended")
    ) { skillCache.values.stream().filter(SkillFilters.VALID_FOR_RECOMMENDATIONS).count() }
    private val onAirExposedPublicSkillsGauge = metricRegistry.lazyGaugeInt64(
        "skills.stats.exposeInternalFlags", Labels.of("state", "on_air", "skillAccess", "public")
    ) {
        skillCache.values
            .count { it.onAir && it.exposeInternalFlags && it.skillAccess == SkillAccess.PUBLIC }
            .toLong()
    }
    private val onAirExposedPrivateSkillsGauge = metricRegistry.lazyGaugeInt64(
        "skills.stats.exposeInternalFlags", Labels.of("state", "on_air", "skillAccess", "private")
    ) {
        skillCache.values
            .count { it.onAir && it.exposeInternalFlags && it.skillAccess == SkillAccess.PRIVATE }
            .toLong()
    }
    private val onAirExposedHiddenSkillsGauge = metricRegistry.lazyGaugeInt64(
        "skills.stats.exposeInternalFlags", Labels.of("state", "on_air", "skillAccess", "hidden")
    ) {
        skillCache.values
            .count { it.onAir && it.exposeInternalFlags && it.skillAccess == SkillAccess.HIDDEN }
            .toLong()
    }

    private val userAgreementsCacheSize = metricRegistry.lazyGaugeInt64("skills.stats.agreements") {
        userAgreementCache.size.toLong()
    }

    @Volatile
    private var skillCache: Map<String, SkillInfo> = mapOf()

    @Volatile
    private var showFeedCache: List<ShowInfo> = listOf()

    @Volatile
    private var userAgreementCache: Map<UUID, List<UserAgreement>> = mapOf()

    @Volatile
    private var skillsByTagsCache: Map<SkillTagsKey, List<SkillInfo>> = mapOf()

    @Volatile
    private var skillsByCategoryCache: Map<SkillCategoryKey, List<SkillInfo>> = mapOf()

    @Volatile
    private var firstSkillsCacheWarmUpOccurred = false

    // doesn't contain entries with empty set
    @Volatile
    private var activationIntentsCache: Map<String, Set<String>> = mapOf()

    @Volatile
    private var firstActivationIntentsCacheWarmUpOccurred = false
    private val findAllActivationIntentsSkillRate = metricRegistry.rate(
        "db.postgres.query_totals",
        Labels.of("query_name", "findAllActivationIntents")
    )
    private val findAllActivationIntentsSkillDuration = metricRegistry.histogramRate(
        "db.postgres.query_duration",
        Labels.of("query_name", "findAllActivationIntents"),
        Histograms.exponential(19, 1.5, 10.3 / 1.5)
    )

    // first run is made on app startup if not disabled by flag
    @Scheduled(initialDelay = 30000, fixedRate = 30000)
    internal fun loadCaches() {
        loadSkillsCache()
        loadActivationIntentsCache()
        loadUserAgreementsCache()
        loadShowFeedCache()
    }

    private fun loadSkillsCache() {
        try {
            logger.trace("Preload skills cache")

            val stopwatch = Stopwatch.createStarted()

            findAllActiveAliceSkillRate.inc()
            val skills = findAllActiveAliceSkillDuration.measureMillis {
                // only nullable in tests
                skillsDao.findAllActiveAliceSkill() ?: listOf()
            }

            val newSkillCache = skills.associate { skill -> skill.id.toString() to createSkillInfo(skill) }
            val newSkillsByTagsCache = formSkillTagsCache(newSkillCache)
            val newSkillsByCategoryCache = formSkillsByCategoryCache(newSkillCache)

            skillCache = newSkillCache
            skillsByTagsCache = newSkillsByTagsCache
            skillsByCategoryCache = newSkillsByCategoryCache

            val allSkillsLoadTime = stopwatch.elapsed()
            logger.trace(
                "Skills cache was loaded with {} entries in {}ms", newSkillCache.size,
                allSkillsLoadTime.toMillis()
            )
            if (allSkillsLoadTime > SKILL_REFRESH_WARN_DURATION) {
                logger.warn(
                    "Skills cache loading with {} entries took {}ms (LONG)", newSkillCache.size,
                    allSkillsLoadTime.toMillis()
                )
            }
            if (!firstSkillsCacheWarmUpOccurred) {
                firstSkillsCacheWarmUpOccurred = true
            }
        } catch (e: Exception) {
            logger.error("Failed to preload skills cache", e)
            throw RuntimeException("Failed to preload skills cache", e)
        }
    }

    private fun formSkillTagsCache(skills: Map<String, SkillInfo>): Map<SkillTagsKey, List<SkillInfo>> {

        return skills.values
            .filter { it.tags.isNotEmpty() && SkillTagsKey.keyByTags(it.tags).isNotEmpty() }
            .flatMap { skill -> SkillTagsKey.keyByTags(skill.tags).map { tagsKey -> skill to tagsKey } }
            .groupBy({ (_, tagsKey) -> tagsKey }, { (skill, _) -> skill })
    }

    private fun formSkillsByCategoryCache(skills: Map<String, SkillInfo>): Map<SkillCategoryKey, List<SkillInfo>> {

        return skills.values
            .filter { it.category != null && SkillCategoryKey.keyByCategory(it.category!!) != null }
            .map { skill -> skill to SkillCategoryKey.keyByCategory(skill.category!!) }
            .groupBy({ (_, skillCategoryKey) -> skillCategoryKey!! }, { (skill, _) -> skill })
    }

    private fun loadActivationIntentsCache() {
        try {
            logger.trace("Preload activation intents cache")
            val stopwatch = Stopwatch.createStarted()
            findAllActivationIntentsSkillRate.inc()

            val skillActivationIntents = findAllActivationIntentsSkillDuration.measureMillis {
                skillsDao.findAllActivationIntents() ?: listOf()
            }

            val cache = skillActivationIntents
                .filter { it.formNames.isNotEmpty() }
                .associate { it.id.toString() to it.formNames.toSet() }

            activationIntentsCache = cache
            val allSkillsActivationIntentsLoadTime = stopwatch.elapsed()
            logger.trace(
                "Skills activation intents cache was loaded with {} entries in {}ms",
                cache.size, allSkillsActivationIntentsLoadTime.toMillis()
            )
            if (allSkillsActivationIntentsLoadTime > ACTIVATION_INTENTS_REFRESH_WARN_DURATION) {
                logger.warn(
                    "Skills activation intents cache loading with {} entries took {}ms (LONG)",
                    cache.size, allSkillsActivationIntentsLoadTime.toMillis()
                )
            }
            if (!firstActivationIntentsCacheWarmUpOccurred) {
                firstActivationIntentsCacheWarmUpOccurred = true
            }
        } catch (e: Exception) {
            logger.error("Failed to preload skills activation intents cache", e)
            throw RuntimeException("Failed to preload skills activation intents cache", e)
        }
    }

    override fun getSkill(skillId: String): Optional<SkillInfo> {
        logger.info("Getting skill info id={} from cache", skillId)
        val fetchedSkill = skillCache[skillId]
            ?.also { logger.info("Fetched skill info id={} from cache", skillId) }

            ?: (
                fetchSkill(skillId)
                    ?.also { logger.info("Fetched skill info id={} from db", skillId) }
                    ?.let { createSkillInfo(it) }
                )

        if (fetchedSkill == null) {
            logger.warn("Skill info with id={} not found neither in cache nor in db", skillId)
            return Optional.empty()
        }

        return Optional.of(fetchedSkill)
    }

    override fun getSkillDraft(skillId: String): Optional<SkillInfo> {
        logger.info("Loading skill draft id={} from DB", skillId)

        val skillOpt: Optional<SkillInfo> = findSkillDraftById.call {
            skillsDao.findAliceSkillDraftById(UUID.fromString(skillId))
                .map { skill: SkillInfoDB -> createSkillInfo(skill) }
        }

        if (skillOpt.isPresent) {
            logger.info("Fetched skill draft info id={} from db", skillId)
        } else {
            logger.warn("Skill info draft with id={} not found in db", skillId)
        }
        return skillOpt
    }

    override fun getActivationIntentFormNames(skillIds: List<String>): Map<String, Set<String>> {
        logger.info("Loading skill activation intents from cache for ids={}", skillIds)

        if (skillIds.isEmpty()) {
            return mapOf()
        }

        val result = skillIds.filter { activationIntentsCache.containsKey(it) }
            .associateWith { activationIntentsCache[it]!! }

        logger.info("Found activation intents for {} of {} skills", result.size, skillIds.size)
        return result
    }

    override fun findAllSkills(): List<SkillInfo> {
        return skillCache.values.toList()
    }

    override fun getSkillsByTags(tagsKey: SkillTagsKey): List<SkillInfo> {
        return skillsByTagsCache[tagsKey] ?: listOf()
    }

    override fun getSkillsByCategory(categoryKey: SkillCategoryKey): List<SkillInfo> {
        return skillsByCategoryCache[categoryKey] ?: listOf()
    }

    override fun getActiveShowSkills(showType: ShowType): List<ShowInfo> {
        return if (showType == ShowType.MORNING) ArrayList(showFeedCache) else listOf()
    }

    override fun getActivePersonalizedShowSkills(showType: ShowType): List<ShowInfo> =
        getActiveShowSkills(showType).filter { it.personalizationEnabled }

    override fun getShowFeed(feedId: String, showType: ShowType): Optional<ShowInfo> {
        return Optional.ofNullable(showFeedCache.find { it.id == feedId && it.showType == showType })
    }

    override fun getShowFeedBySkillId(skillId: String, showType: ShowType): Optional<ShowInfo> {
        return Optional.ofNullable(showFeedCache.find { it.skillInfo.id == skillId && it.showType == showType })
    }

    override fun getShowFeedsByIds(feedId: Collection<String>): List<ShowInfo> {
        val set = feedId.toHashSet()
        return showFeedCache.filter { set.contains(it.id) }
    }

    override fun isReady(): Boolean {
        return firstSkillsCacheWarmUpOccurred && firstActivationIntentsCacheWarmUpOccurred
    }

    override fun findSkillsByPhrases(phrases: Set<String>): Map<String, List<String>> {

        val skillIds: List<SkillIdPhrasesEntity> = findByActivationPhrases.call {
            skillsDao.findByActivationPhrases(phrases.toTypedArray())
        }

        return skillIds.flatMap { (id, activationPhrases) ->
            activationPhrases.map { activationPhrase -> activationPhrase to id.toString() }
        }
            .filter { (activationPhrase, _) -> phrases.contains(activationPhrase) }
            .groupBy({ (activationPhrase, _) -> activationPhrase }, { (_, id) -> id })
    }

    private fun createSkillInfo(skill: SkillInfoDB): SkillInfo {
        val publishingSettings = skill.publishingSettings
        return SkillInfo(
            id = skill.id.toString(),
            name = skill.name,
            userId = skill.userId,
            nameTts = skill.nameTts,
            slug = skill.slug,
            channel = Channel.R.fromValueOrDefault(skill.channel, Channel.UNKNOWN),
            category = publishingSettings?.category,
            developerName = skill.publishingSettings?.developerName,
            explicitContent = skill.publishingSettings?.explicitContent == true,
            developerType = DeveloperType.fromString(skill.developerType),
            description = publishingSettings?.description,
            logoUrl = skill.logoUrl,
            storeUrl = storeUrl + "/skills/" + skill.slug,
            look = if (skill.look == "internal") SkillInfo.Look.INTERNAL else SkillInfo.Look.EXTERNAL,
            _voice = Voice.values().firstOrNull { it.code == skill.voice } ?: Voice.OKSANA,
            salt = skill.salt,
            persistentUserIdSalt = skill.persistentUserIdSalt,
            webhookUrl = skill.backendSettings.uri,
            functionId = skill.backendSettings.functionId,
            surfaces = getSupportedSurfaces(skill),
            onAir = skill.isOnAir(),
            isUseZora = skill.useZora,
            featureFlags = HashSet(skill.featureFlags),
            isUseNlu = skill.useNlu,
            socialAppName = skill.socialAppName,
            grammarsBase64 = skill.grammarsBase64,
            userFeatureFlags = skill.userFeatureFlags,
            encryptedAppMetricaApiKey = skill.appMetricaApiKey,
            isRecommended = skill.isRecommended == true,
            adBlockId = skill.rsyPlatformId,
            useStateStorage = skill.useStateStorage,
            draft = skill.draft,
            automaticIsRecommended = skill.automaticIsRecommended == true,
            hideInStore = skill.hideInStore,
            inflectedActivationPhrases = skill.inflectedActivationPhrases,
            score = skill.score,
            skillAccess = SkillAccess.R.fromValueOrDefault(
                skill.skillAccess,
                if (skill.hideInStore) SkillAccess.HIDDEN else SkillAccess.PUBLIC
            ),

            sharedAccess = HashSet(skill.sharedAccess),
            tags = HashSet(skill.tags),
            activationExamples =
            skill.publishingSettings?.activationExamples
                ?.mapNotNull { structuredExampleToActivationExample(it) } ?: listOf(),
            privateKey = skill.privateKey,
            publicKey = skill.publicKey,
            exposeInternalFlags = skill.exposeInternalFlags,
            safeForKids = skill.safeForKids,
            editorDescription = skill.editorDescription,
            monitoringType = SkillInfo.MonitoringType.values().firstOrNull { it.code == skill.monitoringType }
                ?: SkillInfo.MonitoringType.NONMONITORED
        )
    }

    // https://github.yandex-team.ru/paskills/api/blob/dev/src/services/surface.ts
    internal fun getSupportedSurfaces(skillInfoDB: SkillInfoDB): Set<Surface> {
        if (skillInfoDB.exactSurfaces.isNotEmpty()) {
            return skillInfoDB.exactSurfaces
                .map { Surface.byCode(it) }
                .filter { it.isPresent }
                .map { it.get() }
                .toSet()
        }
        val supportedSurfaces: MutableSet<Surface> = mutableSetOf(Surface.MOBILE, Surface.DESKTOP)
        supportedSurfaces.addAll(surfacesFromInterfaces(skillInfoDB.requiredInterfaces))

        skillInfoDB.surfaceWhitelist
            .map { Surface.byCode(it) }
            .filter { it.isPresent }
            .map { it.get() }
            .forEach { supportedSurfaces.add(it) }

        skillInfoDB.surfaceBlacklist.stream()
            .map { Surface.byCode(it) }
            .filter { it.isPresent }
            .map { it.get() }
            .forEach { supportedSurfaces.remove(it) }

        return supportedSurfaces
    }

    private fun surfacesFromInterfaces(requiredInterfaces: List<String>): Set<Surface> {
        return Surface.values()
            .filter { surface ->
                !surface.isImplicit &&
                    INTERFACES_BY_SURFACE[surface]?.containsAll(requiredInterfaces) == true
            }
            .toSet()
    }

    private fun fetchSkill(skillId: String): SkillInfoDB? {
        return findSkillById.call {
            val id: UUID = try {
                UUID.fromString(skillId.replace("Dialogovo:", ""))
            } catch (e: IllegalArgumentException) {
                logger.error("Invalid skillId: $skillId", e)
                return@call Optional.empty()
            }

            skillsDao.findAliceSkillById(id)
        }
            .filter { obj: SkillInfoDB -> obj.isOnAir() }
            .orElse(null)
    }

    private fun structuredExampleToActivationExample(dbExample: StructuredExample): ActivationExample? {
        return if (dbExample.isValid) {
            if (dbExample.hasRequest()) {
                ActivationExample(dbExample.marker, dbExample.activationPhrase, dbExample.request)
            } else {
                ActivationExample(dbExample.marker, dbExample.activationPhrase)
            }
        } else {
            null
        }
    }

    private fun createShowInfo(morningShow: ShowInfoDB, showType: ShowType, skillInfo: SkillInfo) = ShowInfo(
        id = morningShow.id.toString(),
        name = morningShow.name,
        nameTts = morningShow.nameTts,
        description = morningShow.description,
        showType = showType,
        skillInfo = skillInfo,
        onAir = morningShow.onAir && skillInfo.onAir
    )

    private fun loadShowFeedCache() {
        try {
            showFeedCache = loadShowFeedsMeter.call {
                showFeedDao.findAllActiveShowFeeds()
            }.map { showInfoDB ->
                val skillInfo = skillCache[showInfoDB.skillId.toString()]!!
                ShowInfo(
                    id = showInfoDB.id.toString(),
                    skillInfo = skillInfo,
                    name = showInfoDB.name,
                    nameTts = showInfoDB.nameTts,
                    description = showInfoDB.description,
                    onAir = showInfoDB.onAir,
                    showType = showInfoDB.type,
                    personalizationEnabled = skillInfo.featureFlags.contains(
                        SkillFeatureFlag.PERSONALIZED_MORNING_SHOW_ENABLED
                    )
                )
            }
        } catch (e: Exception) {
            logger.error("Failed to preload show info cache", e)
            throw RuntimeException("Failed to preload show info cache", e)
        }
    }

    private fun loadUserAgreementsCache() {
        try {
            userAgreementCache = loadUserAgreementsMeter.call {
                userAgreementDao.findAllPublishedUserAgreements()
            }
                .map { obj: UserAgreementDb -> obj.toUserAgreement() }
                .groupBy { agreement -> agreement.skillId }
                .mapValues { (_, agreements) -> agreements.sorted() }
        } catch (e: Exception) {
            logger.error("Failed to preload skill user agreement cache", e)
            throw RuntimeException("Failed to preload skill user agreement cache", e)
        }
    }

    override fun getDraftUserAgreements(skillId: UUID): List<UserAgreement> {
        val userAgreementDrafts = findDraftUserAgreementsMeter.call {
            userAgreementDao.findDraftUserAgreements(skillId.toString())
        }
        return userAgreementDrafts
            .map { it.toUserAgreement() }
            .sorted()
    }

    override fun getPublishedUserAgreements(skillId: UUID): List<UserAgreement> {
        return userAgreementCache[skillId] ?: listOf()
    }

    companion object {
        private val logger = LogManager.getLogger(
            DBSkillProvider::class.java
        )
        private val SKILL_REFRESH_WARN_DURATION = Duration.ofSeconds(1)
        private val ACTIVATION_INTENTS_REFRESH_WARN_DURATION = Duration.ofSeconds(1)
        private val INTERFACES_BY_SURFACE = java.util.Map.of(
            Surface.NAVIGATOR,
            setOf(),
            Surface.QUASAR,
            setOf(),
            Surface.AUTO,
            setOf<String>()
        )
    }

    init {
        if (fillCachesOnStartUp) {
            logger.info("Loading caches on startup")
            loadCaches()
            logger.info("Caches loaded")
        }
    }
}
