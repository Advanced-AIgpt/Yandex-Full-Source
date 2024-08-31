package ru.yandex.alice.paskills.my_alice.blocks.skills;

import java.util.Collections;
import java.util.List;
import java.util.Objects;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.EqualsAndHashCode;
import org.apache.logging.log4j.util.Strings;
import org.springframework.lang.Nullable;
import org.springframework.util.StringUtils;

import ru.yandex.alice.paskills.my_alice.layout.SkillCard;

@EqualsAndHashCode
@AllArgsConstructor
class SkillInfo {

    private final String id;
    private final String name;
    private final String description;
    private final String category;
    @Nullable
    private final List<String> examples;
    @Nullable
    private final List<String> tags;
    @Nullable
    private final String editorName;
    @Nullable
    private final String editorDescription;
    @Nullable
    private final List<String> homepageBadgeTypes;

    private final Logo logo;

    public String getName() {
        return !Strings.isEmpty(editorName) ? editorName : name;
    }

    public String getDescription() {
        return !Strings.isEmpty(editorDescription) ? editorDescription : description;
    }

    @Nullable
    private String getHomepageBadgeType() {
        return homepageBadgeTypes != null && !homepageBadgeTypes.isEmpty()
                ? homepageBadgeTypes.get(0)
                : null;
    }

    public List<String> getTags() {
        return Objects.requireNonNullElse(tags, Collections.emptyList());
    }

    public SkillCard toSkillCard() {
        return new SkillCard(voiceSuggest(),
                getDescription(),
                getHomepageBadgeType(),
                logo.getFullUrl()
        );
    }

    String voiceSuggest() {
        if (Category.GAMES_TRIVIA_ACCESSORIES.getName().equals(category)) {
            if (name.toLowerCase().startsWith("игра ")) {
                String shortName = name.replaceFirst("^(?iu)игра ", "");
                return "Играем в игру " + shortName;
            } else {
                return "Играем в " + getName();
            }
        } else if (examples == null || examples.isEmpty()) {
            return defaultVoiceSuggest();
        } else {
            String shortest = examples
                    .stream()
                    .min((a, b) -> Integer.compare(a.length(), b.length()))
                    .map(StringUtils::capitalize)
                    .get();
            return StringUtils.capitalize(shortest);
        }
    }

    private String defaultVoiceSuggest() {
        return "Запусти навык " + getName();
    }

    @Data
    public static class Logo {
        private final String avatarId;
        private final String color;
        private final String fullUrl;

        @JsonCreator
        Logo(
                @JsonProperty("avatarId") String avatarId,
                @JsonProperty("color") String color) {
            this.avatarId = avatarId;
            this.color = color;
            this.fullUrl = "https://avatars.mds.yandex.net/get-dialogs/" + avatarId + "/orig";
        }

    }

}
