package ru.yandex.quasar.billing.dao;

import java.time.Instant;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nonnull;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.ToString;
import lombok.With;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.PersistenceConstructor;
import org.springframework.data.relational.core.mapping.Column;
import org.springframework.data.relational.core.mapping.MappedCollection;

@Getter
// exclude keys to hide them
@ToString(exclude = {"privateKey", "publicKey"})
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@Builder
public class SkillInfo {
    /**
     * Internal primary key
     */
    @With
    @Id
    private final Long skillInfoId;

    /**
     * Skill UUID identifier from Dialog platform
     */
    @Nonnull
    private final String skillUuid;

    private final String slug;

    private final long ownerUid;
    /**
     * creation date of the skill info
     */
    @Nonnull
    private final Instant createdAt;
    /**
     * private key used to sign purchase data
     * TODO remove in PASKILLS-5195
     */
    @Nonnull
    @Deprecated
    private String privateKey;
    /**
     * public key downloadable through Dialog's platform which is used to validate signature
     * TODO remove in PASKILLS-5195
     */
    @Nonnull
    @Deprecated
    private String publicKey;
    @Column(value = "skill_info_id")
    @MappedCollection(keyColumn = "merchant_key")
    @Nonnull
    @Builder.Default
    private Map<String, SkillMerchant> merchants = new HashMap<>();

    @PersistenceConstructor()
    private SkillInfo(Long skillInfoId, long ownerUid, @Nonnull String skillUuid, String slug,
                      @Nonnull String privateKey, @Nonnull String publicKey, @Nonnull Instant createdAt) {
        this.skillInfoId = skillInfoId;
        this.skillUuid = skillUuid;
        this.ownerUid = ownerUid;
        this.slug = slug;
        this.privateKey = privateKey;
        this.publicKey = publicKey;
        this.createdAt = createdAt;
    }

    public static SkillInfoBuilder builder(String skillId) {
        return new SkillInfoBuilder()
                .skillUuid(skillId)
                .createdAt(Instant.now());
    }

    public void setKeys(String privateKey, String publicKey) {
        Objects.requireNonNull(privateKey, "Private key must be not null");
        Objects.requireNonNull(publicKey, "Public key must be not null");
        this.privateKey = privateKey;
        this.publicKey = publicKey;
    }

    public void addMerchant(SkillMerchant merchant) {
        merchants.put(merchant.getToken(), merchant);
    }

}
