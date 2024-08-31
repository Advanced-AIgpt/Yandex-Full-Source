package ru.yandex.alice.paskills.my_alice.blocks.recommender;

import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.paskills.my_alice.layout.Card;
import ru.yandex.alice.paskills.my_alice.layout.PageLayout;

@Data
class RecommenderItem {

    @Nullable
    private final String id;

    @Nullable
    private final String activation;

    @Nullable
    private final String description;

    @Nullable
    @JsonProperty("logo_avatar_id")
    private final String logoAvatarId;

    @Nullable
    @JsonProperty("logo_prefix")
    private final String logoPrefix;

    @Nullable
    private final String look;

    @Nullable
    private final String name;

    @Nullable
    @JsonProperty("logo_bg_image_quasar_id")
    private final String logoBgImageQuasarId;

    @Nullable
    @JsonProperty("logo_fg_round_image_url")
    private final String logoFgRoundImageUrl;

    @Nullable
    @JsonProperty("logo_amelie_bg_url")
    private final String logoAmelieBgUrl;

    @Nullable
    @JsonProperty("search_app_card_item_text")
    private final String searchAppCardItemText;

    @Nullable
    @JsonProperty("logo_my_alice_url")
    private final String logoMyAliceUrl;

    @Nullable
    private final String intro;

    @Nullable
    @JsonProperty("logo_bg_color")
    private final String logoBgColor;

    private boolean isValid() {
        return activation != null && intro != null && description != null && logoMyAliceUrl != null;
    }

    Optional<PageLayout.Card> toCard() {
        if (!isValid()) {
            return Optional.empty();
        }
        return Optional.of(new Card(
                Card.Kind.SCENARIO,
                getActivation(),
                getIntro(),
                getDescription(),
                getLogoMyAliceUrl(),
                Objects.requireNonNullElse(getLogoBgColor(), "#e8eeff"),
                null
        ));
    }

}
