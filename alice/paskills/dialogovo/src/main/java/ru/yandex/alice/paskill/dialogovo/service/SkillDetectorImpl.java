package ru.yandex.alice.paskill.dialogovo.service;

import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import lombok.Data;
import lombok.RequiredArgsConstructor;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillActivationPhraseSearcher;
import ru.yandex.alice.paskill.dialogovo.service.normalizer.NormalizationService;

import static java.util.Comparator.comparing;
import static java.util.stream.Collectors.toList;

class SkillDetectorImpl implements SkillDetector {
    private static final Logger logger = LogManager.getLogger();

    private static final int MAX_CANDIDATE_WORD_LEN = 10;
    private static final int MAX_CANDIDATES_COUNT = 200;

    private static final Comparator<CandidateResult> FOUND_SKILLS_COMPARATOR =
            comparing(CandidateResult::getWordsCount)
                    .thenComparing(Comparator.comparing(CandidateResult::getPosition).reversed());

    private final NormalizationService normalizationService;
    private final SkillActivationPhraseSearcher skillSearcher;

    SkillDetectorImpl(
            NormalizationService normalizationService,
            SkillActivationPhraseSearcher skillSearcher
    ) {
        this.normalizationService = normalizationService;
        this.skillSearcher = skillSearcher;
        //warmup jni
        this.normalizationService.normalize("привет");
    }

    @Override
    public Optional<DetectedSkill> tryDetectSkill(String originalUtterance, boolean tokenizeFromStartOnly) {
        return detectedSkillStream(originalUtterance, tokenizeFromStartOnly)
                .peek(chosenCandidate -> logger.info("Chosen skill candidate id={}", chosenCandidate.getSkillId()))
                .findFirst();
    }

    @Override
    public List<DetectedSkill> detectSkills(String originalUtterance, boolean tokenizeFromStartOnly) {
        return detectedSkillStream(originalUtterance, tokenizeFromStartOnly)
                .collect(toList());
    }

    private Stream<DetectedSkill> detectedSkillStream(String originalUtterance, boolean tokenizeFromStartOnly) {
        String normalizedUtterance = normalizationService.normalize(originalUtterance);

        var words = normalizedUtterance.split("\\s+");

        Map<String, CandidateResult> candidates = generateCandidates(words, tokenizeFromStartOnly);
        List<CandidateResult> foundCandidates = findSkills(candidates);

        if (foundCandidates.isEmpty()) {
            logger.info("No skill found by activation phrases (tokenizeFromStartOnly={}). Normalized request: {}",
                    tokenizeFromStartOnly, normalizedUtterance);
            return Stream.empty();
        }

        return foundCandidates.stream()
                .sorted(FOUND_SKILLS_COMPARATOR.reversed())
                .map(chosenCandidate -> new DetectedSkill(
                                chosenCandidate.skillId,
                                Stream.of(words).skip(chosenCandidate.position).collect(Collectors.joining(" "))
                        )
                );
    }

    private Map<String, CandidateResult> generateCandidates(String[] words, boolean tokenizeFromStartOnly) {
        Map<String, CandidateResult> candidates = new HashMap<>();
        for (int wordsLen = 1; wordsLen <= MAX_CANDIDATE_WORD_LEN; wordsLen++) {
            for (int fromWord = 0; fromWord + wordsLen <= words.length; fromWord++) {
                // leave only first insertion
                var phrase = Stream.of(words).skip(fromWord).limit(wordsLen).collect(Collectors.joining(" "));

                candidates.put(phrase, CandidateResult.of(phrase, fromWord + wordsLen, wordsLen));

                if (tokenizeFromStartOnly) {
                    break;
                }

                if (candidates.size() >= MAX_CANDIDATES_COUNT) {
                    break;
                }
            }
            if (candidates.size() >= MAX_CANDIDATES_COUNT) {
                break;
            }
        }

        return candidates;
    }

    private List<CandidateResult> findSkills(Map<String, CandidateResult> candidates) {
        logger.debug("Searching skills with candidates: {}", candidates);
        Map<String, List<String>> skillIdsByPhrases = skillSearcher.findSkillsByPhrases(candidates.keySet());

        logger.info("Found documents: {}", skillIdsByPhrases.values());

        return skillIdsByPhrases.entrySet().stream()
                .filter(entry -> candidates.containsKey(entry.getKey()))
                .flatMap(entry ->
                        entry.getValue().stream().map(skillId -> candidates.get(entry.getKey()).withSkillID(skillId))
                )
                .collect(toList());
    }

    @Data
    @RequiredArgsConstructor
    static class CandidateResult {
        private final String phrase;
        private final int position;
        private final int wordsCount;

        @Nullable
        private final String skillId;

        static CandidateResult of(String phrase, int position, int wordsCount) {
            return new CandidateResult(phrase, position, wordsCount, null);
        }

        CandidateResult withSkillID(String skillId) {
            return new CandidateResult(phrase, position, wordsCount, skillId);
        }
    }
}
