package ru.yandex.alice.paskills.yt_merger;

import java.util.function.BiFunction;
import java.util.function.Function;

import lombok.Data;

import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTree;
import ru.yandex.inside.yt.kosher.operations.OperationContext;
import ru.yandex.inside.yt.kosher.operations.Statistics;
import ru.yandex.inside.yt.kosher.operations.Yield;
import ru.yandex.inside.yt.kosher.operations.map.Mapper;
import ru.yandex.inside.yt.kosher.ytree.YTreeMapNode;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@BenderBindAllFields
public class YtMergeMapper implements Mapper<YTreeMapNode, YTreeMapNode> {

    private final TableRowMapper rowMapper;

    public static final String COLUMN_TABLE_NAME = "table_name";
    public static final String COLUMN_TABLE_ROW = "content";

    public YtMergeMapper(String tableName) {
        this.rowMapper = new TableRowMapper(tableName);
    }

    @Override
    public void map(YTreeMapNode entry, Yield<YTreeMapNode> yield, Statistics statistics, OperationContext context) {
        yield.yield(rowMapper.apply(entry));
    }

    @Data
    public static class YtRowWithTableName {
        private final String tableName;
        private final YTreeMapNode row;
    }

    private static class RowMapperImpl implements BiFunction<String, YTreeMapNode, YTreeMapNode> {

        static final RowMapperImpl INSTANCE = new RowMapperImpl();

        private RowMapperImpl() {
        }

        @Override
        public YTreeMapNode apply(String tableName, YTreeMapNode entry) {
            return YTree.mapBuilder()
                    .key(COLUMN_TABLE_NAME).value(tableName)
                    .key(COLUMN_TABLE_ROW).value(entry)
                    .buildMap();
        }
    }

    public static class TableRowMapper implements Function<YTreeMapNode, YTreeMapNode> {

        private final String tableName;

        public TableRowMapper(String tableName) {
            this.tableName = tableName;
        }

        @Override
        public YTreeMapNode apply(YTreeMapNode entry) {
            return RowMapperImpl.INSTANCE.apply(tableName, entry);
        }

    }

    public static class YtRowWithTableNameMapper implements Function<YtRowWithTableName, YTreeMapNode> {

        public static final YtRowWithTableNameMapper INSTANCE = new YtRowWithTableNameMapper();

        private YtRowWithTableNameMapper() {
        }

        @Override
        public YTreeMapNode apply(YtRowWithTableName ytRowWithTableName) {
            return RowMapperImpl.INSTANCE.apply(ytRowWithTableName.getTableName(), ytRowWithTableName.getRow());
        }

    }
}
