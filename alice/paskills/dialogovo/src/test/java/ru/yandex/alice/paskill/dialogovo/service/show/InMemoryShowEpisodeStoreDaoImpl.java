package ru.yandex.alice.paskill.dialogovo.service.show;

import java.time.Instant;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;

import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.domain.show.ShowEpisodeEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType;

public class InMemoryShowEpisodeStoreDaoImpl implements ShowEpisodeStoreDao {
    private final Map<ShowEpisodeKey, ShowEpisodeEntity> episodes = new LinkedHashMap<>();

    public void load(@Nonnull List<ShowEpisodeEntity> list) {
        storeSyncShowEpisodeEntity(list, Instant.now());
    }

    public void clear() {
        episodes.clear();
    }

    public List<ShowEpisodeEntity> getEpisodes() {
        return new ArrayList<>(episodes.values());
    }

    @Override
    public void storeSyncShowEpisodeEntity(@Nonnull List<ShowEpisodeEntity> list, @Nonnull Instant saveTime) {
        list.forEach(episode ->
                episodes.put(
                        new ShowEpisodeKey(
                                episode.getSkillId(),
                                episode.getShowType(),
                                episode.getId()
                        ),
                        episode
                )
        );
    }

    @Data
    private static class ShowEpisodeKey {
        private final String skillId;
        private final ShowType showType;
        private final String episodeId;
    }
}
