package ru.yandex.alice.paskills.yt_merger;

import javax.annotation.Nullable;

import com.beust.jcommander.Parameter;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.ToString;

import ru.yandex.inside.yt.kosher.cypress.YPath;

@EqualsAndHashCode
@ToString
@Getter
class CommandLineArgs {
    @Parameter(names = {"--input-dir", "-i"}, description = "Directory with tables to merge", required = true)
    private String inputDir;

    @Parameter(names = {"--output-table", "-o"}, description = "Output table", required = true)
    private String outputTable;

    @Parameter(names = {"--force", "-f"}, description = "Overwrite output table if it exists")
    private boolean force = false;

    @Parameter(names = "--do-not-delete-input", description = "Do not delete processed input tables")
    private boolean doNotDeleteInput = false;

    @Parameter(names = "--min-table-name", description = "Min table name (table names are compared lexicographically)")
    @Nullable
    private String minTableName;

    @Parameter(names = "--max-table-name", required = true, description = "Max table name (table names are compared " +
            "lexicographically)")
    private String maxTableName;

    @Parameter(names = {"--proxy", "-p"}, description = "YT proxy with (FQDN like hahn.yt.yandex.net)")
    private String ytProxy = "hahn.yt.yandex.net";

    @Parameter(names = "--yt-transaction-timeout-minutes", description = "YT transaction timeout in minutes")
    private int ytTransactionTimeoutMinutes = 5;

    @Parameter(
            names = "--yt-transaction-ping-interval-minutes",
            description = "YT transaction ping interval in minutes"
    )
    private int ytTransactionPingIntervalMinutes = 1;

    @Parameter(names = "--help")
    private boolean help = false;

    public YPath getOutputTableYPath() {
        return YPath.simple(getOutputTable());
    }

    public YPath getInputTableAbsoluteYPath(String tableName) {
        return YPath.simple(getInputDir() + "/" + tableName);
    }
}
