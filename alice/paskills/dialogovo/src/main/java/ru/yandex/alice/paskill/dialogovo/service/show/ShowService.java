package ru.yandex.alice.paskill.dialogovo.service.show;

import java.time.Duration;
import java.util.List;
import java.util.Optional;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowEpisodeEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType;

public interface ShowService {
    // On show launch we check for valid show episodes, episode must remain valid for some extra time since then.
    // So when this episode is requested during show it's still valid
    Duration EPISODE_WILL_REMAIN_VALID_THRESHOLD = Duration.ofHours(1);

    void updateUnpersonalizedShows(ShowType showType);

    List<String> getPersonalizedShowConfigAndStartPrepare(
            ShowType showType, List<String> mementoConfigSkillIds, ClientInfo clientInfo);

    Optional<ShowEpisodeEntity> getRelevantEpisode(ShowType showType, String skillId, ClientInfo clientInfo);
}
