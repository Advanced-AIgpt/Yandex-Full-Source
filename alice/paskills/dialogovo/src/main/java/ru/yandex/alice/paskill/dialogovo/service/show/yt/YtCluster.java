package ru.yandex.alice.paskill.dialogovo.service.show.yt;

import ru.yandex.misc.enums.StringEnum;
import ru.yandex.misc.enums.StringEnumResolver;

/**
 * @author Sergey Kuptsov <kuptservol@yandex-team.ru>
 */
public enum YtCluster implements StringEnum {
    HAHN("hahn", "hahn.yt.yandex.net");

    public static final StringEnumResolver<YtCluster> R = new StringEnumResolver<>(YtCluster.class);

    private final String name;
    private final String proxyUri;

    YtCluster(String name, String proxyUri) {
        this.name = name;
        this.proxyUri = proxyUri;
    }

    @Override
    public String value() {
        return name;
    }

    public String getName() {
        return name;
    }

    public String getProxyUri() {
        return proxyUri;
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder("YtCluster{");
        sb.append("name='").append(name).append('\'');
        sb.append(", proxyUri='").append(proxyUri).append('\'');
        sb.append('}');
        return sb.toString();
    }
}

