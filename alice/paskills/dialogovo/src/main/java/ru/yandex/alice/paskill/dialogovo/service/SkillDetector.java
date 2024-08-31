package ru.yandex.alice.paskill.dialogovo.service;

import java.util.List;
import java.util.Optional;

import lombok.Data;

public interface SkillDetector {

    Optional<DetectedSkill> tryDetectSkill(String originalUtterance, boolean tokenizeFromStartOnly);

    List<DetectedSkill> detectSkills(String originalUtterance, boolean tokenizeFromStartOnly);

    @Data
    class DetectedSkill {
        private final String skillId;
        private final String suffix;
    }
}
