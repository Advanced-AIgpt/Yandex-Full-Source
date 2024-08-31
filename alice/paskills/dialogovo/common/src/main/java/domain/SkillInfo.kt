package ru.yandex.alice.paskill.dialogovo.domain

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonValue
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.domain.Surface
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.utils.StringEnum
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver
import ru.yandex.alice.paskill.dialogovo.utils.VoiceUtils
import java.util.Optional

// TODO: move to postgres table
private val SAFE_SKILLS = java.util.Set.of(
    "fd5188a6-a57e-4e71-adcd-36203e363a14", // Мышка в лабиринте
    "e855a1d7-862a-4945-b65f-de7bcfa1d870", // Счастливый фермер
    "805a582d-0ac9-4606-9182-0b2d0fae5cd9", // Хомяк повторюшка
    "b961e8ab-a839-4b24-a030-dfa184a59267", // Мой тамагочи
    "649114d8-cd8a-4be7-bafe-0965940f3347", // Закончи пословицу
    "d72eedce-c6f5-412b-8ed7-93cdccd9b716", // Игра про репку
    "5043c1f0-affb-4c7f-87fa-67f1fa52e461", // Музыкальные стулья
    "672f7477-d3f0-443d-9bd5-2487ab0b6a4c", // Города
    "cd3875e6-9114-484c-b944-c25affd1c7e6", // Слова
    "f80f9b78-18cf-4a91-9d1b-96e32dfc52e0", // Угадай персонажа
    "1efe89c4-bb69-4258-8bf2-c0247b21f126", // Смешарики в мире динозавров
    "7d221eb8-954f-4769-8807-9e5e6fb09d45" // Камень-ножницы-бумага
)

data class SkillInfo(
    val id: String,
    val name: String,
    val userId: String,
    private val nameTts: String?,
    val slug: String,
    val channel: Channel,
    val category: String?,
    val developerType: DeveloperType,
    val developerName: String?,
    val description: String?,
    val score: Double,
    val inflectedActivationPhrases: List<String> = listOf(),
    val logoUrl: String?,
    val storeUrl: String?,
    val look: Look?,
    @JsonProperty("voice")
    private val _voice: Voice,
    val salt: String,
    val persistentUserIdSalt: String,
    val webhookUrl: String?,
    val functionId: String?, /* = Collections.emptySet()*/
    val surfaces: Set<Surface> = setOf(),
    val onAir: Boolean,
    val isUseZora: Boolean,
    val isUseNlu: Boolean,
    private val socialAppName: String?,
    private val grammarsBase64: String?,
    val userFeatureFlags: Map<String, Any> = mapOf(),
    private val encryptedAppMetricaApiKey: String?,
    val monitoringType: MonitoringType?,
    /**
     * use isValidForRecommendations instead
     *
     *
     * Manual recommendation flag. Set via either developer console or by automatic process in hitman
     */
    private val isRecommended: Boolean,
    val useStateStorage: Boolean,
    private val adBlockId: String?,
    val draft: Boolean,
    /**
     * use isValidForRecommendations instead
     *
     *
     * Set by solomon alerts
     */
    private val automaticIsRecommended: Boolean,
    val hideInStore: Boolean,
    val explicitContent: Boolean,
    val exposeInternalFlags: Boolean, /* = Collections.emptySet()*/
    val featureFlags: Set<String> = setOf(), /* = SkillAccess.PUBLIC*/
    val skillAccess: SkillAccess = SkillAccess.PUBLIC, /* = Collections.emptySet()*/
    val sharedAccess: Set<String> = setOf(), /* = Collections.emptySet()*/
    val tags: Set<String> = setOf(), /* = Collections.emptyList()*/
    val activationExamples: List<ActivationExample> = listOf(),
    val publicKey: String?,
    val privateKey: String?,
    val safeForKids: Boolean,
    val editorDescription: String?
) {

    fun getSocialAppName(): Optional<String> {
        return Optional.ofNullable(socialAppName)
    }

    fun getNameTts(): String {
        return nameTts ?: VoiceUtils.normalize(name)
    }

    val isExternal: Boolean
        get() = Look.EXTERNAL == look

    fun isOnAir() = onAir
    fun isHideInStore() = hideInStore
    fun isExplicitContent() = explicitContent
    fun isExposeInternalFlags() = exposeInternalFlags

    /**
     * Composite flag for skill recommendation
     */
    val isValidForRecommendations: Boolean
        get() = isRecommended && onAir && automaticIsRecommended && !hideInStore

    val isInternal: Boolean
        get() = Look.INTERNAL == look

    val isMonitored: Boolean
        get() = monitoringType != MonitoringType.NONMONITORED

    fun hasFeatureFlag(flag: String): Boolean {
        return featureFlags.contains(flag)
    }

    fun hasUserFeatureFlag(flag: String): Boolean {
        return userFeatureFlags.containsKey(flag)
    }

    fun getEncryptedAppMetricaApiKey(): Optional<String> {
        return Optional.ofNullable(encryptedAppMetricaApiKey)
    }

    fun getGrammarsBase64(): Optional<String> {
        return Optional.ofNullable(grammarsBase64)
    }

    fun getAdBlockId(): Optional<String> = if (hasFeatureFlag(SkillFeatureFlag.AD_IN_SKILLS)) {
        Optional.of("R-IM-462015-1")
    } else {
        Optional.ofNullable(adBlockId)
    }

    fun isAccessibleBy(currentUserId: String?, request: MegaMindRequest<*>): Boolean {
        return skillAccess != SkillAccess.PRIVATE || isPrivateSkillAccessible(currentUserId, request)
    }

    private fun isPrivateSkillAccessible(currentUserId: String?, request: MegaMindRequest<*>): Boolean {
        return (
            isUserHasPrivateSkillAccess(currentUserId) ||
                isPrivateSkillAccessibleByFlag(request) ||
                isPrivateSkillAccessibleByB2BConfig(request)
            )
    }

    private fun isPrivateSkillAccessibleByB2BConfig(request: MegaMindRequest<*>): Boolean {
        return request.options.quasarAuxiliaryConfig.alice4Business?.preactivatedSkillIds?.contains(id) == true
    }

    private fun isPrivateSkillAccessibleByFlag(request: MegaMindRequest<*>): Boolean {
        return request.getExperimentsWithCommaSeparatedListValues(Experiments.ACCESSIBLE_PRIVATE_SKILLS, listOf())
            .any { accessibleByFlagSkillId: String -> accessibleByFlagSkillId == id }
    }

    private fun isUserHasPrivateSkillAccess(currentUserId: String?): Boolean {
        return currentUserId != null && (userId == currentUserId || sharedAccess.contains(currentUserId))
    }

    val isSafeForChildren: Boolean
        get() = SAFE_SKILLS.contains(id) || safeForKids

    // fun getVoice(): Voice
    val voice: Voice
        get() = when (_voice) {
            Voice.OKSANA -> Voice.OKSANA_GPU
            Voice.ZAHAR -> Voice.ZAHAR_GPU
            Voice.ERKANYAVAS -> Voice.KOLYA_GPU
            Voice.JANE -> Voice.JANE_GPU
            Voice.ERMIL -> Voice.ERMIL_GPU
            else -> if (featureFlags.contains(SkillFeatureFlag.VTB_BRAND_VOICE)) Voice.VTB_BRAND_VOICE else _voice
        }

    fun toBuilder(): SkillInfoBuilder {
        return SkillInfoBuilder()
            .id(id)
            .name(name)
            .userId(userId)
            .nameTts(nameTts)
            .slug(slug)
            .channel(channel)
            .category(category)
            .developerType(developerType)
            .developerName(developerName)
            .description(description)
            .score(score)
            .inflectedActivationPhrases(inflectedActivationPhrases)
            .logoUrl(logoUrl)
            .storeUrl(storeUrl)
            .look(look)
            .voice(_voice)
            .salt(salt)
            .persistentUserIdSalt(persistentUserIdSalt)
            .webhookUrl(webhookUrl)
            .functionId(functionId)
            .surfaces(surfaces)
            .onAir(onAir)
            .useZora(isUseZora)
            .useNlu(isUseNlu)
            .socialAppName(socialAppName)
            .grammarsBase64(grammarsBase64)
            .userFeatureFlags(userFeatureFlags)
            .encryptedAppMetricaApiKey(encryptedAppMetricaApiKey)
            .isRecommended(isRecommended)
            .useStateStorage(useStateStorage)
            .adBlockId(adBlockId)
            .draft(draft)
            .automaticIsRecommended(automaticIsRecommended)
            .hideInStore(hideInStore)
            .explicitContent(explicitContent)
            .exposeInternalFlags(exposeInternalFlags)
            .featureFlags(featureFlags)
            .skillAccess(skillAccess)
            .sharedAccess(sharedAccess)
            .tags(tags)
            .activationExamples(activationExamples)
            .publicKey(publicKey)
            .privateKey(privateKey)
            .safeForKids(safeForKids)
            .editorDescription(editorDescription)
    }

    enum class Look(@JsonValue val code: String) {
        EXTERNAL("external"), INTERNAL("internal");
    }

    enum class MonitoringType(@JsonValue val code: String) {
        NONMONITORED("nonmonitored"), MONITORED("monitored"), YANDEX("yandex"), JUSTAI("justAI");
    }

    // enum_skills_skillAccess in DB
    enum class SkillAccess(@JsonValue val value: String) :
        StringEnum {
        PUBLIC("public"), HIDDEN("hidden"), PRIVATE("private");

        override fun value(): String {
            return value
        }

        companion object {
            val R: StringEnumResolver<SkillAccess> = StringEnumResolver.resolver(SkillAccess::class.java)!!
        }
    }

    val nameAsTextWithTts: TextWithTts
        get() = TextWithTts(name, getNameTts())

    data class ActivationExample(
        val marker: String,
        val activationPhrase: String,
        val request: String? = null
    ) {

        fun asText(): String = if (request == null) {
            "$marker $activationPhrase"
        } else {
            "$marker $activationPhrase $request"
        }

        fun hasRequest() = request != null
    }

    fun isRecommended(): Boolean {
        return isRecommended
    }

    fun isAutomaticIsRecommended(): Boolean {
        return automaticIsRecommended
    }

    override fun hashCode(): Int {
        return id.hashCode()
    }

    class SkillInfoBuilder internal constructor() {
        private lateinit var id: String
        private lateinit var name: String
        private lateinit var userId: String
        private var nameTts: String? = null
        private lateinit var slug: String
        private lateinit var channel: Channel
        private var category: String? = null
        private lateinit var developerType: DeveloperType
        private var developerName: String? = null
        private var description: String? = null
        private var score = 0.0
        private var inflectedActivationPhrases: List<String> = listOf()
        private var logoUrl: String? = null
        private var storeUrl: String? = null
        private var look: Look? = null
        private lateinit var voice: Voice
        private lateinit var salt: String
        private lateinit var persistentUserIdSalt: String
        private var webhookUrl: String? = null
        private var functionId: String? = null
        private var surfaces: Set<Surface> = setOf()
        private var onAir = false
        private var useZora = false
        private var useNlu = false
        private var socialAppName: String? = null
        private var grammarsBase64: String? = null
        private var userFeatureFlags: Map<String, Any> = mapOf()
        private var encryptedAppMetricaApiKey: String? = null
        private var isRecommended = false
        private var useStateStorage = false
        private var adBlockId: String? = null
        private var draft = false
        private var automaticIsRecommended = false
        private var hideInStore = false
        private var explicitContent = false
        private var exposeInternalFlags = false
        private var featureFlags: Set<String> = setOf()
        private var skillAccess: SkillAccess = SkillAccess.PUBLIC
        private var sharedAccess: Set<String> = setOf()
        private var tags: Set<String> = setOf()
        private var activationExamples: List<ActivationExample> = listOf()
        private var publicKey: String? = null
        private var privateKey: String? = null
        private var safeForKids: Boolean = false
        private var editorDescription: String? = null
        private var monitoringType: MonitoringType? = null
        fun id(id: String): SkillInfoBuilder {
            this.id = id
            return this
        }

        fun name(name: String): SkillInfoBuilder {
            this.name = name
            return this
        }

        fun userId(userId: String): SkillInfoBuilder {
            this.userId = userId
            return this
        }

        fun nameTts(nameTts: String?): SkillInfoBuilder {
            this.nameTts = nameTts
            return this
        }

        fun slug(slug: String): SkillInfoBuilder {
            this.slug = slug
            return this
        }

        fun channel(channel: Channel): SkillInfoBuilder {
            this.channel = channel
            return this
        }

        fun category(category: String?): SkillInfoBuilder {
            this.category = category
            return this
        }

        fun developerType(developerType: DeveloperType): SkillInfoBuilder {
            this.developerType = developerType
            return this
        }

        fun developerName(developerName: String?): SkillInfoBuilder {
            this.developerName = developerName
            return this
        }

        fun description(description: String?): SkillInfoBuilder {
            this.description = description
            return this
        }

        fun score(score: Double): SkillInfoBuilder {
            this.score = score
            return this
        }

        fun inflectedActivationPhrases(inflectedActivationPhrases: List<String>): SkillInfoBuilder {
            this.inflectedActivationPhrases = inflectedActivationPhrases
            return this
        }

        fun logoUrl(logoUrl: String?): SkillInfoBuilder {
            this.logoUrl = logoUrl
            return this
        }

        fun storeUrl(storeUrl: String?): SkillInfoBuilder {
            this.storeUrl = storeUrl
            return this
        }

        fun look(look: Look?): SkillInfoBuilder {
            this.look = look
            return this
        }

        fun voice(voice: Voice): SkillInfoBuilder {
            this.voice = voice
            return this
        }

        fun salt(salt: String): SkillInfoBuilder {
            this.salt = salt
            return this
        }

        fun persistentUserIdSalt(persistentUserIdSalt: String): SkillInfoBuilder {
            this.persistentUserIdSalt = persistentUserIdSalt
            return this
        }

        fun webhookUrl(webhookUrl: String?): SkillInfoBuilder {
            this.webhookUrl = webhookUrl
            return this
        }

        fun functionId(functionId: String?): SkillInfoBuilder {
            this.functionId = functionId
            return this
        }

        fun surfaces(surfaces: Set<Surface>): SkillInfoBuilder {
            this.surfaces = surfaces
            return this
        }

        fun onAir(onAir: Boolean): SkillInfoBuilder {
            this.onAir = onAir
            return this
        }

        fun useZora(useZora: Boolean): SkillInfoBuilder {
            this.useZora = useZora
            return this
        }

        fun useNlu(useNlu: Boolean): SkillInfoBuilder {
            this.useNlu = useNlu
            return this
        }

        fun socialAppName(socialAppName: String?): SkillInfoBuilder {
            this.socialAppName = socialAppName
            return this
        }

        fun grammarsBase64(grammarsBase64: String?): SkillInfoBuilder {
            this.grammarsBase64 = grammarsBase64
            return this
        }

        fun userFeatureFlags(userFeatureFlags: Map<String, Any>): SkillInfoBuilder {
            this.userFeatureFlags = userFeatureFlags
            return this
        }

        fun encryptedAppMetricaApiKey(encryptedAppMetricaApiKey: String?): SkillInfoBuilder {
            this.encryptedAppMetricaApiKey = encryptedAppMetricaApiKey
            return this
        }

        fun isRecommended(isRecommended: Boolean): SkillInfoBuilder {
            this.isRecommended = isRecommended
            return this
        }

        fun useStateStorage(useStateStorage: Boolean): SkillInfoBuilder {
            this.useStateStorage = useStateStorage
            return this
        }

        fun adBlockId(adBlockId: String?): SkillInfoBuilder {
            this.adBlockId = adBlockId
            return this
        }

        fun draft(draft: Boolean): SkillInfoBuilder {
            this.draft = draft
            return this
        }

        fun automaticIsRecommended(automaticIsRecommended: Boolean): SkillInfoBuilder {
            this.automaticIsRecommended = automaticIsRecommended
            return this
        }

        fun hideInStore(hideInStore: Boolean): SkillInfoBuilder {
            this.hideInStore = hideInStore
            return this
        }

        fun explicitContent(explicitContent: Boolean): SkillInfoBuilder {
            this.explicitContent = explicitContent
            return this
        }

        fun exposeInternalFlags(exposeInternalFlags: Boolean): SkillInfoBuilder {
            this.exposeInternalFlags = exposeInternalFlags
            return this
        }

        fun featureFlags(featureFlags: Set<String>): SkillInfoBuilder {
            this.featureFlags = featureFlags
            return this
        }

        fun skillAccess(skillAccess: SkillAccess): SkillInfoBuilder {
            this.skillAccess = skillAccess
            return this
        }

        fun sharedAccess(sharedAccess: Set<String>): SkillInfoBuilder {
            this.sharedAccess = sharedAccess
            return this
        }

        fun tags(tags: Set<String>): SkillInfoBuilder {
            this.tags = tags
            return this
        }

        fun activationExamples(activationExamples: List<ActivationExample>): SkillInfoBuilder {
            this.activationExamples = activationExamples
            return this
        }

        fun publicKey(publicKey: String?): SkillInfoBuilder {
            this.publicKey = publicKey
            return this
        }

        fun privateKey(privateKey: String?): SkillInfoBuilder {
            this.privateKey = privateKey
            return this
        }

        fun safeForKids(safeForKids: Boolean): SkillInfoBuilder {
            this.safeForKids = safeForKids
            return this
        }

        fun editorDescription(editorDescription: String?): SkillInfoBuilder {
            this.editorDescription = editorDescription
            return this
        }

        fun monitoringType(monitoringType: MonitoringType?): SkillInfoBuilder {
            this.monitoringType = monitoringType
            return this
        }

        fun build(): SkillInfo {
            return SkillInfo(
                id = id,
                name = name,
                userId = userId,
                nameTts = nameTts,
                slug = slug,
                channel = channel,
                category = category,
                developerType = developerType,
                developerName = developerName,
                description = description,
                score = score,
                inflectedActivationPhrases = inflectedActivationPhrases,
                logoUrl = logoUrl,
                storeUrl = storeUrl,
                look = look,
                _voice = voice,
                salt = salt,
                persistentUserIdSalt = persistentUserIdSalt,
                webhookUrl = webhookUrl,
                functionId = functionId,
                surfaces = surfaces,
                onAir = onAir,
                isUseZora = useZora,
                isUseNlu = useNlu,
                socialAppName = socialAppName,
                grammarsBase64 = grammarsBase64,
                userFeatureFlags = userFeatureFlags,
                encryptedAppMetricaApiKey = encryptedAppMetricaApiKey,
                isRecommended = isRecommended,
                useStateStorage = useStateStorage,
                adBlockId = adBlockId,
                draft = draft,
                automaticIsRecommended = automaticIsRecommended,
                hideInStore = hideInStore,
                explicitContent = explicitContent,
                exposeInternalFlags = exposeInternalFlags,
                featureFlags = featureFlags,
                skillAccess = skillAccess,
                sharedAccess = sharedAccess,
                tags = tags,
                activationExamples = activationExamples,
                publicKey = publicKey,
                privateKey = privateKey,
                safeForKids = safeForKids,
                editorDescription = editorDescription,
                monitoringType = monitoringType
            )
        }
    }

    companion object {

        @JvmStatic
        fun builder(): SkillInfoBuilder {
            return SkillInfoBuilder()
        }
    }
}
