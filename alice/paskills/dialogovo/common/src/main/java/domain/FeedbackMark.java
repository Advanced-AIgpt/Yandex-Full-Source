package ru.yandex.alice.paskill.dialogovo.domain;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.AllArgsConstructor;

@AllArgsConstructor
public enum FeedbackMark {
    EXCELLENT(5, "alice.external_skill_feedback_excellent"),
    GOOD(4, "alice.external_skill_feedback_good"),
    ACCEPTABLE(3, "alice.external_skill_feedback_acceptable"),
    BAD(2, "alice.external_skill_feedback_bad"),
    VERY_BAD(1, "alice.external_skill_feedback_very_bad");

    private static Map<Integer, FeedbackMark> valueToMarkMap = new HashMap<>();

    static {
        valueToMarkMap = List.of(FeedbackMark.values()).stream()
                .collect(Collectors.toMap(FeedbackMark::getMarkValue, x -> x));
    }

    private final Integer markValue;

    private final String nluFrameName;

    public Integer getMarkValue() {
        return markValue;
    }

    public String getNluFrameName() {
        return nluFrameName;
    }

    public static FeedbackMark ofValue(Integer markValue) {
        return valueToMarkMap.get(markValue);
    }

    @JsonValue
    public String getTitle() {
        return markValue.toString();
    }

}
