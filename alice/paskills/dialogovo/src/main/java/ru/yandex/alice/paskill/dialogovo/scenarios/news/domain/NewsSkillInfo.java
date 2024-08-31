package ru.yandex.alice.paskill.dialogovo.scenarios.news.domain;

import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.domain.Voice;
import ru.yandex.alice.kronstadt.core.text.InflectedString;
import ru.yandex.alice.paskill.dialogovo.domain.Channel;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.utils.VoiceUtils;

@Data
@Builder(toBuilder = true)
public class NewsSkillInfo {
    private final String id;
    private final String name;
    private final InflectedString inflectedName;
    @Nullable
    private final String nameTts;
    private final String slug;
    private final Channel channel;
    @Nullable
    private final String logoUrl;
    private final Voice voice;
    private final boolean onAir;
    @Nullable
    private final List<String> inflectedActivationPhrases;
    private final Map<String, ?> userFeatureFlags;
    private final String encryptedAppMetricaApiKey;
    private final boolean isRecommended;
    @Nullable
    private final String adBlockId;
    private final boolean draft;
    private final boolean automaticIsRecommended;
    private final boolean hideInStore;
    @Builder.Default
    private final Set<String> featureFlags = Collections.emptySet();
    @Nullable
    private final NewsFeed defaultFeed;
    private final List<NewsFeed> feeds;
    private final FlashBriefingType flashBriefingType;

    /**
     * use isValidForRecommendations instead
     * <p>
     * Manual recommendation flag. Set via either developer console or by automatic process in hitman
     */
    private boolean getIsRecommended() {
        return isRecommended;
    }

    /**
     * use isValidForRecommendations instead
     * <p>
     * Set by solomon alerts
     */
    private boolean getAutomaticIsRecommended() {
        return automaticIsRecommended;
    }


    public String getNameTts() {
        return nameTts != null ? nameTts : VoiceUtils.normalize(name);
    }

    /**
     * Composite flag for skill recommendation
     */
    public boolean isValidForRecommendations() {
        return isRecommended && onAir && automaticIsRecommended && !hideInStore;
    }

    public boolean hasFeatureFlag(String flag) {
        return getFeatureFlags().contains(flag);
    }

    public boolean hasUserFeatureFlag(String flag) {
        return getUserFeatureFlags().containsKey(flag);
    }

    public Optional<String> getEncryptedAppMetricaApiKey() {
        return Optional.ofNullable(encryptedAppMetricaApiKey);
    }

    public Optional<String> getAdBlockId() {
        return hasFeatureFlag(SkillFeatureFlag.AD_IN_SKILLS) ?
                Optional.of("R-IM-462015-1") :
                Optional.ofNullable(adBlockId);
    }

    public Optional<NewsFeed> getDefaultFeed() {
        return Optional.ofNullable(defaultFeed);
    }

    public List<String> getInflectedActivationPhrases() {
        return inflectedActivationPhrases != null ? inflectedActivationPhrases : Collections.emptyList();
    }

    public FlashBriefingType getFlashBriefingType() {
        return Optional.ofNullable(flashBriefingType).orElse(FlashBriefingType.UNKNOWN);
    }

    public InflectedString getInflectedName() {
        return Optional.ofNullable(inflectedName).orElse(InflectedString.cons(name));
    }
}
