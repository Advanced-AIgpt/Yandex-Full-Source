package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import javax.annotation.Nonnull;
import javax.validation.constraints.NotNull;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

import ru.yandex.alice.paskill.dialogovo.domain.Censored;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@NoArgsConstructor
@AllArgsConstructor
@JsonTypeInfo(
        use = JsonTypeInfo.Id.NAME,
        property = "type",
        visible = true,
        defaultImpl = InvalidCard.class // ignore all empty or unknown cards
)
@JsonSubTypes({
        @JsonSubTypes.Type(value = BigImageCard.class, name = "BigImage"),
        @JsonSubTypes.Type(value = ItemsListCard.class, name = "ItemsList"),
        @JsonSubTypes.Type(value = BigImageListCard.class, name = "BigImageList"),
        @JsonSubTypes.Type(value = BigImageListCard.class, name = "ImageGallery"),
})
@JsonInclude(NON_ABSENT)
@Censored
public abstract class Card {
    //check validation only for console
    @CardTypeValid(groups = SourceType.Console.class)
    @NotNull
    private CardType type;

    @Nonnull
    public CardType getType() {
        return type;
    }
}
