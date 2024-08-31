package ru.yandex.alice.paskill.dialogovo.service.wizard;

import java.util.Collections;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.ToString;


@Data
class WizardHttpResponse {

    private final Rules rules;

    public Rules getRules() {
        return rules != null ? rules : Rules.EMPTY;
    }

    @Data
    @AllArgsConstructor
    static final class Rules {
        public static final Rules EMPTY = new Rules(GranetRule.EMPTY);

        @Nullable
        @JsonProperty("Granet")
        private final GranetRule granet;

        public GranetRule getGranet() {
            return granet != null ? granet : GranetRule.EMPTY;
        }
    }

    @Data
    @AllArgsConstructor
    static class GranetRule {
        public static final GranetRule EMPTY = new GranetRule(Collections.emptyList(), Collections.emptyList());

        private static final String BUILTIN_FORM_PREFIX = "YANDEX.";

        @Nullable
        @JsonProperty("Forms")
        private final List<Form> forms;

        @Nullable
        @JsonProperty("Tokens")
        private final List<Token> tokens;

        public List<Form> getForms() {
            return forms != null ? forms : Collections.emptyList();
        }

        public List<Token> getTokens() {
            return tokens != null ? tokens : Collections.emptyList();
        }

    }

    @Data
    @AllArgsConstructor
    static class Form {

        @JsonProperty("Name")
        private final String name;

        @JsonProperty("Tags")
        private final List<Tag> tags;

        public List<Tag> getTags() {
            return tags != null ? tags : Collections.emptyList();
        }

    }

    @Data
    @AllArgsConstructor
    @ToString(includeFieldNames = true)
    static class Tag implements Comparable<Tag> {
        @JsonProperty("Begin")
        private final int begin;

        @JsonProperty("End")
        private final int end;

        @JsonProperty("Name")
        private final String name;

        @JsonProperty("Data")
        @Nullable
        private final List<TagData> data;

        public List<TagData> getData() {
            return data != null ? data : Collections.emptyList();
        }

        /**
         * Compare tags first by begin position, then by name.
         */
        @Override
        public int compareTo(@Nullable Tag other) {
            if (other == null) {
                return -1;
            }
            int positionComparison = this.getBegin() - other.getBegin();
            return positionComparison != 0
                    ? positionComparison
                    : this.getName().compareTo(other.getName());
        }
    }

    @Data
    @AllArgsConstructor
    static class TagData {
        @JsonProperty("Begin")
        private final int begin;

        @JsonProperty("End")
        private final int end;

        @JsonProperty("Type")
        @Nullable
        private final String type;

        @JsonProperty("Value")
        @Nullable
        private final String value;

    }

    @Data
    @AllArgsConstructor
    static class Token {
        // Begin and End are token
        @JsonProperty("Begin")
        private final int begin;

        @JsonProperty("End")
        private final int end;

        @JsonProperty("Text")
        private final String text;

        @JsonProperty("Lemma")
        private final String lemma;

    }
}
