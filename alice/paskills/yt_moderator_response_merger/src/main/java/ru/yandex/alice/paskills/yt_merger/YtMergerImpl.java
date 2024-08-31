package ru.yandex.alice.paskills.yt_merger;

import java.time.Duration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.function.BiConsumer;
import java.util.stream.Collectors;

import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.ToString;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.inside.yt.kosher.Yt;
import ru.yandex.inside.yt.kosher.cypress.CypressNodeType;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.inside.yt.kosher.impl.YtUtils;
import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTree;
import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTreeBuilder;
import ru.yandex.inside.yt.kosher.operations.specs.SortSpec;
import ru.yandex.inside.yt.kosher.tables.YTableEntryTypes;
import ru.yandex.inside.yt.kosher.transactions.Transaction;
import ru.yandex.inside.yt.kosher.ytree.YTreeMapNode;
import ru.yandex.inside.yt.kosher.ytree.YTreeNode;
import ru.yandex.inside.yt.kosher.ytree.YTreeStringNode;

import static ru.yandex.alice.paskills.yt_merger.YtMergeMapper.COLUMN_TABLE_NAME;

class YtMergerImpl {

    private static final Logger logger = LogManager.getLogger("YtMergerImpl");

    private static final String YT_ATTRIBUTE_TYPE = "type";
    private static final String YT_ATTRIBUTE_ROW_COUNT = "row_count";
    private static final String YT_NODE_TYPE_TABLE = "table";

    private final CommandLineArgs args;
    private final String ytToken;

    YtMergerImpl(CommandLineArgs args, String ytToken) {
        this.args = args;
        this.ytToken = ytToken;
    }

    public void run() {
        Yt ytClient = YtUtils.http(args.getYtProxy(), ytToken);
        logger.info("Starting YtMerger with following arguments: {}", args);
        runInsideTransaction(ytClient, (yt, transaction) -> {
            createOutputTable(yt, transaction);
            ListDirResult inputTablesWithRowCount = listInputDir(yt, transaction);
            logger.info("{} tables will be processed: {}",
                    inputTablesWithRowCount.tables.size(),
                    inputTablesWithRowCount.tables);
            mergeTablesInMemory(yt, transaction, inputTablesWithRowCount.tables);
            sortOutputTable(yt, transaction);
            removeInputTables(yt, transaction, inputTablesWithRowCount.tables);
            validateRowCount(yt, transaction, inputTablesWithRowCount.rowCount);
        });
    }

    private void runInsideTransaction(Yt yt, BiConsumer<Yt, Transaction> function) {
        Transaction transaction = yt.transactions().startAndGet(
                Duration.ofMinutes(args.getYtTransactionTimeoutMinutes()));
        TransactionPinger pinger = new TransactionPinger(
                transaction,
                Duration.ofMinutes(args.getYtTransactionPingIntervalMinutes()));
        new Thread(pinger).start();
        logger.info("Started transaction {} with timeout {}", transaction.getId(), transaction.getTimeout());
        try {
            function.accept(yt, transaction);
            logger.info("Committing transaction");
            pinger.stop();
            transaction.commit();
        } catch (Exception e) {
            pinger.stop();
            transaction.abort();
            throw e;
        }
    }

    private ListDirResult listInputDir(Yt yt, Transaction transaction) {
        var tables = yt.cypress()
                .list(
                        Optional.of(transaction.getId()),
                        true,
                        YPath.simple(args.getInputDir()),
                        Cf.set(YT_ATTRIBUTE_TYPE, YT_ATTRIBUTE_ROW_COUNT))
                .stream()
                .filter(node -> node.containsAttribute(YT_ATTRIBUTE_TYPE))
                .filter(node -> YT_NODE_TYPE_TABLE.equals(
                        node.getAttribute(YT_ATTRIBUTE_TYPE).orElse(null).stringValue()))
                .filter(node -> node.stringValue().compareTo(args.getMaxTableName()) <= 0)
                .filter(node -> args.getMinTableName() == null ||
                        node.stringValue().compareTo(args.getMinTableName()) >= 0)
                .collect(Collectors.toList());
        List<String> tableNames = tables.stream()
                .map(YTreeStringNode::getValue)
                .collect(Collectors.toList());
        long rowCount = tables.stream()
                .map(tbl -> tbl.getAttribute(YT_ATTRIBUTE_ROW_COUNT))
                .map(o -> o.orElse(null))
                .mapToLong(YTreeNode::longValue)
                .sum();
        return new ListDirResult(tableNames, rowCount);
    }

    private void mergeTablesInMemory(Yt yt, Transaction transaction, List<String> tables) {
        logger.info("Starting mappers");
        List<YTreeMapNode> outputTable = tables.parallelStream()
                .flatMap(
                        tbl -> yt.tables().read(
                                Optional.of(transaction.getId()),
                                true,
                                args.getInputTableAbsoluteYPath(tbl),
                                YTableEntryTypes.YSON
                        ).stream()
                        .map(row -> new YtMergeMapper.YtRowWithTableName(tbl, row))
                        .map(YtMergeMapper.YtRowWithTableNameMapper.INSTANCE)
                )
                .collect(Collectors.toList());
        yt.tables().write(
                Optional.of(transaction.getId()),
                true,
                args.getOutputTableYPath(),
                YTableEntryTypes.YSON,
                outputTable.iterator()
        );
        logger.info("All tables were merged to output table");
    }

    private void sortOutputTable(Yt yt, Transaction transaction) {
        logger.info("Sorting output table");
        SortSpec spec = SortSpec.builder()
                .setInputTables(Cf.list(args.getOutputTableYPath()))
                .setOutputTable(args.getOutputTableYPath())
                .setSortBy(List.of(COLUMN_TABLE_NAME))
                .build();
        yt.operations()
                .sortAndGetOp(Optional.of(transaction.getId()), true, spec)
                .awaitAndThrowIfNotSuccess();
    }

    private void removeInputTables(Yt yt, Transaction transaction, List<String> tables) {
        if (args.isDoNotDeleteInput()) {
            return;
        }
        logger.info("Removing input tables");
        tables.parallelStream().forEach(table -> {
            logger.info("Removing table {}", table);
            yt.cypress().remove(
                    transaction.getId(),
                    true,
                    args.getInputTableAbsoluteYPath(table)
            );
        });
    }

    private void createOutputTable(Yt yt, Transaction transaction) {
        YPath outputPath = args.getOutputTableYPath();
        if (yt.cypress().exists(
                transaction.getId(),
                true,
                outputPath)
        ) {
            if (args.isForce()) {
                logger.info("Recreating output table {} due to --force flag", args.getOutputTable());
                yt.cypress().remove(
                        transaction.getId(),
                        true,
                        args.getOutputTableYPath()
                );
            } else {
                throw new NodeAlreadyExists(args.getOutputTable());
            }
        }
        Map<String, YTreeNode> attributes = new HashMap<>();
        attributes.put("schema", schema());
        logger.info("attributes {}", attributes);
        yt.cypress().create(
                Optional.of(transaction.getId()),
                true,
                outputPath,
                CypressNodeType.TABLE,
                false,
                false,
                attributes
        );
    }

    private YTreeNode schema() {
        YTreeBuilder schemaBuilder = YTree.listBuilder();
        addColumn(schemaBuilder, "table_name", "string", YtTypeV3.STRING);
        addColumn(schemaBuilder, "content", "any", YtTypeV3.YSON);
        return YTree.builder()
                .beginAttributes()
                .key("strict").value(true)
                .key("unique_keys").value(false)
                .endAttributes()
                .value(schemaBuilder.buildList())
                .build();
    }

    private void addColumn(YTreeBuilder builder, String name, String type, YtTypeV3 typeV3) {
        builder.beginMap();
        builder.key("name").value(name);
        builder.key("type").value(type);
        builder.key("type_v3").value(typeV3.toYtree());
        builder.endMap();
    }

    private void validateRowCount(Yt yt, Transaction transaction, long expectedRowCount) {
        long outputTableRowCount = yt.cypress()
                .get(
                        Optional.of(transaction.getId()),
                        true,
                        args.getOutputTableYPath(),
                        Cf.set(YT_ATTRIBUTE_ROW_COUNT))
                .getAttribute(YT_ATTRIBUTE_ROW_COUNT)
                .orElse(null)
                .longValue();
        if (outputTableRowCount != expectedRowCount) {
            throw new RuntimeException("Failed to validate output table row count: expected " +
                    Long.toString(expectedRowCount) + ", got " + Long.toString(outputTableRowCount));
        }
    }

    @Getter
    @EqualsAndHashCode(callSuper = true)
    @ToString
    private static class NodeAlreadyExists extends RuntimeException {
        private final String path;

        private NodeAlreadyExists(String path) {
            super("Node " + path + " already exists");
            this.path = path;
        }
    }

    @Data
    private static class ListDirResult {
        private final List<String> tables;
        private final long rowCount;
    }

    private enum YtTypeV3 {
        STRING,
        YSON;

        public YTreeNode toYtree() {
            switch (this) {
                case STRING:
                    return YTree.stringNode("string");
                case YSON:
                    return YTree.builder()
                            .beginMap()
                            .key("type_name").value("optional")
                            .key("item").value("yson")
                            .endMap()
                            .build();
                default:
                    throw new UnsupportedOperationException();
            }
        }
    }

}
