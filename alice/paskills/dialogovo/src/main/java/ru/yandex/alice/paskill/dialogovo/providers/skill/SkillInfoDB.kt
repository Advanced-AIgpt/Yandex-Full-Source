package ru.yandex.alice.paskill.dialogovo.providers.skill

import com.fasterxml.jackson.annotation.JsonInclude
import org.apache.logging.log4j.util.Strings
import org.springframework.data.annotation.Id
import org.springframework.data.relational.core.mapping.Column
import org.springframework.data.relational.core.mapping.Table
import java.util.UUID

@Table("skills")
data class SkillInfoDB @JvmOverloads constructor(
    @field:Id val id: UUID,
    val name: String,
    val userId: String,
    val nameTts: String?,
    val slug: String,
    val inflectedActivationPhrases: List<String> = listOf(),
    val backendSettings: BackendSettings,
    val onAir: Boolean,
    val useZora: Boolean,
    val salt: String,
    val persistentUserIdSalt: String,
    val voice: String,
    val look: String,
    val logoUrl: String?,
    val useNlu: Boolean,
    val socialAppName: String? = null,
    val developerType: String?,
    val grammarsBase64: String? = null,
    @Column("user_feature_flags")
    private val _userFeatureFlags: UserFeatures = UserFeatures(mapOf()),
    val appMetricaApiKey: String? = null,
    val isRecommended: Boolean? = null,
    val exactSurfaces: List<String> = listOf(),
    val surfaceWhitelist: List<String> = listOf(),
    val surfaceBlacklist: List<String> = listOf(),
    val requiredInterfaces: List<String> = listOf(),
    val useStateStorage: Boolean = false,
    val rsyPlatformId: String? = null,
    val draft: Boolean,
    val automaticIsRecommended: Boolean?,
    val hideInStore: Boolean,
    val exposeInternalFlags: Boolean,
    val score: Double = 0.0,
    val skillAccess: String?,
    val sharedAccess: List<String> = listOf(),
    val tags: List<String> = listOf(),
    val testingBackendSettings: BackendSettings? = null,
    val channel: String = "channel",
    val publishingSettings: PublishingSettings? = null,
    val featureFlags: List<String> = listOf(),
    val publicKey: String? = null,
    val privateKey: String? = null,
    val safeForKids: Boolean = false,
    val editorDescription: String? = null,
    val monitoringType: String?,
) {
    fun withPublishingSettings(publishingSettings: PublishingSettings) =
        this.copy(publishingSettings = publishingSettings)

    fun withFeatureFlags(featureFlags: List<String>) = this.copy(featureFlags = featureFlags)

    val userFeatureFlags: Map<String, Any>
        get() = _userFeatureFlags.flags

    fun isOnAir() = onAir

    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    data class BackendSettings(
        val uri: String? = null,
        val functionId: String? = null,
    )

    data class PublishingSettings(
        val description: String? = null,
        val category: String? = null,
        val examples: List<String>? = null,
        val developerName: String? = null,
        val brandVerificationWebsite: String? = null,
        val explicitContent: Boolean? = null,
        val structuredExamples: List<StructuredExample> = listOf(),
    ) {
        val activationExamples: List<StructuredExample>
            get() = structuredExamples
    }

    // all fields non-null
    // https://a.yandex-team.ru/arc_vcs/frontend/services/paskills-api/src/types.ts?rev=b946702406e1743592f5edb3835f62d41d49a0d7#L29
    data class StructuredExample(
        val marker: String,
        val activationPhrase: String,
        val request: String? = null,
    ) {
        val isValid: Boolean
            get() = !Strings.isEmpty(marker) && !Strings.isEmpty(activationPhrase)

        fun hasRequest() = request != null && request.isNotEmpty()
    }

    data class UserFeatures(val flags: Map<String, Any> = mapOf())
}
