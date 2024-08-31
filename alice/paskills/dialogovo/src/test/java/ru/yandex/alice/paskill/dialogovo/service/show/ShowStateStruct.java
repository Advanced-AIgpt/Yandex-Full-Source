package ru.yandex.alice.paskill.dialogovo.service.show;

import java.util.Collections;
import java.util.List;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.domain.show.ShowEpisodeEntity;

@Data
public class ShowStateStruct {
    private final List<ShowEpisodeEntity> episodes;

    @JsonCreator
    public ShowStateStruct(
            @JsonProperty("episodes") List<ShowEpisodeEntity> episodes
    ) {
        this.episodes = episodes;
    }

    public List<ShowEpisodeEntity> getEpisodes() {
        return episodes != null ? episodes : Collections.emptyList();
    }

}
