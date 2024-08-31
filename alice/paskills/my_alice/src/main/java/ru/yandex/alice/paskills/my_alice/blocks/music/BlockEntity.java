package ru.yandex.alice.paskills.my_alice.blocks.music;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;

import ru.yandex.alice.paskills.my_alice.layout.ImageCard;

@JsonTypeInfo(
        use = JsonTypeInfo.Id.NAME,
        property = "type",
        visible = true,
        defaultImpl = UnknownBlockEntity.class // ignore all unknown blocks
)
@JsonSubTypes({
        @JsonSubTypes.Type(value = Album.class, name = "album"),
        @JsonSubTypes.Type(value = Playlist.class, name = "playlist"),
        @JsonSubTypes.Type(value = ChartItem.class, name = "chart-item"),
        @JsonSubTypes.Type(value = MixLink.class, name = "mix-link")
})
interface BlockEntity {

    @Nullable
    String getId();
    @Nullable
    String getImageUri();
    @Nullable
    String getSuggest();
    @Nullable
    String getTitle();

    default ImageCard toImageCard() {
        return new ImageCard(
                getTitle(),
                getSuggest(),
                getImageUri(),
                null,
                ImageCard.Shape.SQUARE,
                ImageCard.Size.M
        );
    }

    default boolean isValid() {
        return getId() != null && getSuggest() != null && getImageUri() != null && getTitle() != null;
    }

}
