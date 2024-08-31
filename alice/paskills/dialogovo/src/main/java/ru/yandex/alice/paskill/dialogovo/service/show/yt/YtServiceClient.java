package ru.yandex.alice.paskill.dialogovo.service.show.yt;

import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig;
import ru.yandex.alice.paskill.dialogovo.config.ShowConfig;
import ru.yandex.inside.yt.kosher.Yt;
import ru.yandex.inside.yt.kosher.impl.YtUtils;

public class YtServiceClient {
    private final Yt yt;

    public YtServiceClient(
            SecretsConfig secretsConfig,
            ShowConfig showConfig
    ) {
        this.yt = YtUtils.http(
                clusterFromString(showConfig.getYt().getCluster()).getProxyUri(),
                secretsConfig.getYtToken()
        );
    }

    public YtServiceClient(
            Yt yt
    ) {
        this.yt = yt;
    }

    private static YtCluster clusterFromString(String cluster) {
        return YtCluster.R.fromValueO(cluster).getOrThrow("Unknown yt cluster " + cluster);
    }

    public Yt getYtClient() {
        return yt;
    }
}
