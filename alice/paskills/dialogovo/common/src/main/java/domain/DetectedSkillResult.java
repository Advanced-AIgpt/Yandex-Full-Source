package ru.yandex.alice.paskill.dialogovo.domain;

import java.util.Optional;

import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class DetectedSkillResult {
    private final SkillInfo skillInfo;
    private final Optional<String> requestO;
    private final Optional<String> originalUtteranceO;
    private final Optional<ActivationSourceType> activationSourceTypeO;

    public static DetectedSkillResult of(SkillInfo skillInfo) {
        return new DetectedSkillResult(skillInfo, Optional.empty(), Optional.empty(), Optional.empty());
    }
}
