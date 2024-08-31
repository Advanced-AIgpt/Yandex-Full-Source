package ru.yandex.alice.paskill.dialogovo.service.show.yt;

import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskill.dialogovo.config.ShowConfig;
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowEpisodeEntity;
import ru.yandex.alice.paskill.dialogovo.service.show.ShowEpisodeStoreDao;
import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.MapF;
import ru.yandex.inside.yt.kosher.Yt;
import ru.yandex.inside.yt.kosher.common.GUID;
import ru.yandex.inside.yt.kosher.cypress.CypressNodeType;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.inside.yt.kosher.impl.ytree.YTreeStringNodeImpl;
import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTree;
import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTreeBuilder;
import ru.yandex.inside.yt.kosher.tables.YTableEntryTypes;
import ru.yandex.inside.yt.kosher.ytree.YTreeMapNode;
import ru.yandex.inside.yt.kosher.ytree.YTreeNode;
import ru.yandex.yt.ytclient.tables.ColumnSchema;
import ru.yandex.yt.ytclient.tables.ColumnValueType;
import ru.yandex.yt.ytclient.tables.TableSchema;

public class YtEpisodeStoreDaoImpl implements ShowEpisodeStoreDao {
    private static final Logger logger = LogManager.getLogger();
    private static final YTreeNode SCHEMA = new TableSchema.Builder()
            .setStrict(true)
            .setUniqueKeys(false)
            .add(new ColumnSchema("skill_id", ColumnValueType.STRING))
            .add(new ColumnSchema("show_type", ColumnValueType.STRING))
            .add(new ColumnSchema("pub_date", ColumnValueType.STRING))
            .add(new ColumnSchema("exp_date", ColumnValueType.STRING))
            .add(new ColumnSchema("text", ColumnValueType.STRING))
            .add(new ColumnSchema("tts", ColumnValueType.STRING))
            .add(new ColumnSchema("uid", ColumnValueType.STRING))
            .build().toYTree();
    private final YtServiceClient ytServiceClient;
    private final YPath directory;
    private final Duration ytSaveTimeout;
    private final Duration ytTtlInHours;

    public YtEpisodeStoreDaoImpl(ShowConfig showConfig, YtServiceClient ytServiceClient) {
        this.ytServiceClient = ytServiceClient;
        this.directory = YPath.simple(showConfig.getYt().getDirectory());
        this.ytSaveTimeout = Duration.of(showConfig.getYtSaveTimeout(), ChronoUnit.MILLIS);
        this.ytTtlInHours = Duration.of(showConfig.getYtTtlInHours(), ChronoUnit.HOURS);
    }

    public YtEpisodeStoreDaoImpl(
            YPath directory, YtServiceClient ytServiceClient,
            Duration saveTimeout, Duration ytTtlInHours
    ) {
        this.ytServiceClient = ytServiceClient;
        this.directory = directory;
        this.ytSaveTimeout = saveTimeout;
        this.ytTtlInHours = ytTtlInHours;
    }

    @Override
    public void storeSyncShowEpisodeEntity(@Nonnull List<ShowEpisodeEntity> showEpisodes, @Nonnull Instant saveTime) {
        if (!showEpisodes.isEmpty()) {
            Yt yt = ytServiceClient.getYtClient();
            GUID transaction = null;
            try {
                transaction = yt.transactions().start(ytSaveTimeout);
                saveBatchOfShowEpisodes(yt, transaction, showEpisodes, saveTime);
                yt.transactions().commit(transaction, false);
                logger.info("Storing of show episodes completed successfully");
            } catch (Throwable throwable) {
                if (transaction != null) {
                    yt.transactions().abort(transaction);
                }
                logger.error("Error while storing show episodes", throwable);
                throw throwable;
            }
        }
    }

    private void saveBatchOfShowEpisodes(
            Yt yt,
            GUID transaction,
            List<ShowEpisodeEntity> showEpisodes,
            Instant instant
    ) {
        YPath tablePath = getTableForInstant(instant);
        MapF<String, YTreeNode> attributes = Cf.map(
                "expiration_time",
                new YTreeStringNodeImpl(instant.plus(ytTtlInHours).truncatedTo(ChronoUnit.MICROS).toString(), null)
        );
        yt.cypress().create(tablePath, CypressNodeType.TABLE, true, true, attributes);
        yt.tables().alterTable(tablePath, Optional.of(false), Optional.of(SCHEMA));
        List<YTreeMapNode> list = showEpisodes.stream()
                .map(this::convert)
                .collect(Collectors.toList());
        yt.tables().write(Optional.of(transaction), true, tablePath, YTableEntryTypes.YSON, list.iterator());
    }

    public YPath getTableForInstant(Instant instant) {
        return directory.child(instant.toString());
    }

    private YTreeMapNode convert(ShowEpisodeEntity showEpisodeEntity) {
        YTreeBuilder builder = YTree.mapBuilder()
                .key("skill_id").value(showEpisodeEntity.getSkillId())
                .key("show_type").value(showEpisodeEntity.getShowType().toString())
                .key("pub_date").value(showEpisodeEntity.getPublicationDate().toString())
                .key("exp_date").value(
                        Optional.ofNullable(showEpisodeEntity.getExpirationDate())
                                .map(Instant::toString)
                                .orElse(null))
                .key("text").value(showEpisodeEntity.getText())
                .key("tts").value(showEpisodeEntity.getTts())
                .key("uid").value(showEpisodeEntity.getId());
        return builder.buildMap();
    }
}
