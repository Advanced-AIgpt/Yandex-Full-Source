package ru.yandex.alice.paskill.dialogovo.service.show;

import java.time.Instant;
import java.util.List;

import ru.yandex.alice.paskill.dialogovo.domain.show.ShowEpisodeEntity;

public interface ShowEpisodeStoreDao {
    void storeSyncShowEpisodeEntity(List<ShowEpisodeEntity> showEpisodes, Instant saveTime);
}
