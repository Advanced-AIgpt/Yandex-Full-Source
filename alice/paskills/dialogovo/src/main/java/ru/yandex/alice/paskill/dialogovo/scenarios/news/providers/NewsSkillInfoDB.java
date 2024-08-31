package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Builder;
import lombok.Data;
import lombok.With;
import org.springframework.data.annotation.Id;
import org.springframework.data.relational.core.mapping.Table;

@Data
@Builder
@Table("skills")
public class NewsSkillInfoDB {
    @Id
    @With
    private final UUID id;
    private final String name;
    @Nullable
    private final String nameTts;
    private final String slug;
    @Nullable
    private final List<String> inflectedActivationPhrases;
    private final BackendSettings backendSettings;
    private final boolean onAir;
    private final String voice;
    @Builder.Default
    private final String channel = "aliceNewsSkill";
    @Nullable
    private final String logoUrl;
    private final List<String> featureFlags;
    private final UserFeatures userFeatureFlags;
    @Nullable
    private final String appMetricaApiKey;
    @Nullable
    private final Boolean isRecommended;
    @Nullable
    private final String rsyPlatformId;
    private final boolean draft;
    @Nullable
    private final Boolean automaticIsRecommended;
    private final boolean hideInStore;

    public List<String> getFeatureFlags() {
        return featureFlags != null ? featureFlags : Collections.emptyList();
    }

    public Map<String, ?> getUserFeatureFlags() {
        return userFeatureFlags != null && userFeatureFlags.flags != null
                ? userFeatureFlags.flags
                : Collections.emptyMap();
    }

    @Data
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class BackendSettings {
        private final String defaultFeed;
        @Nullable
        private final String flashBriefingType;

        public BackendSettings(String defaultFeed) {
            this.defaultFeed = defaultFeed;
            this.flashBriefingType = null;
        }

        public BackendSettings(String defaultFeed, String flashBriefingType) {
            this.defaultFeed = defaultFeed;
            this.flashBriefingType = flashBriefingType;
        }
    }

    public record UserFeatures(Map<String, ?> flags) {
    }
}
