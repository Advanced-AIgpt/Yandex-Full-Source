package ru.alice.paskills.billing.yt.cleaner

import com.beust.jcommander.JCommander
import com.beust.jcommander.Parameter
import org.apache.logging.log4j.Level
import org.apache.logging.log4j.LogManager
import org.apache.logging.log4j.core.config.Configurator
import org.apache.logging.log4j.core.config.DefaultConfiguration
import ru.yandex.inside.yt.kosher.Yt
import ru.yandex.inside.yt.kosher.cypress.YPath
import ru.yandex.inside.yt.kosher.impl.YtUtils
import ru.yandex.inside.yt.kosher.impl.transactions.utils.pinger.TransactionPinger
import ru.yandex.inside.yt.kosher.transactions.Transaction
import java.time.Duration
import java.util.Optional

const val BASE_DIR = "//home/paskills/billing/snapshots"
const val YT_PROXY = "hahn.yt.yandex.net"
val DIR_NAME_PATTERN = "\\d{4}-\\d{2}-\\d{2}".toRegex()

private val logger = LogManager.getLogger()

fun main(args: Array<String>) {
    Configurator.initialize(DefaultConfiguration())
    Configurator.setRootLevel(Level.INFO)
    Configurator.setLevel("ru.yandex.misc.io.http.apache.v4.ApacheHttpClient4Utils", Level.WARN)

    val commandLineArgs = Args()
    val jCommander: JCommander = JCommander.newBuilder()
        .addObject(commandLineArgs)
        .build()

    jCommander.parse(*args)
    if (commandLineArgs.help) {
        jCommander.usage()
    } else {
        val ytToken = System.getenv("YT_TOKEN")
            ?: throw IllegalArgumentException("Missing YT client credentials. Please set YT_TOKEN environment variable")
        SnapshotCleaner(commandLineArgs, ytToken).removeSnapshots()
    }
}

internal class SnapshotCleaner(private val args: Args, ytToken: String) {
    private val yt: Yt = YtUtils.http(YT_PROXY, ytToken)
    internal fun removeSnapshots() {

        runInsideTransaction(args) { transaction ->
            val snapshots = listDirectories(transaction)
            logger.info("found tables for removal: $snapshots")
            snapshots.forEach { removeSnapshot(transaction, it) }
        }
    }

    private fun removeSnapshot(transaction: Transaction, snapshot: String) {
        val path = "$BASE_DIR/$snapshot"
        logger.info("dropping $path")
        yt.cypress()
            .remove(Optional.of(transaction.id), true, YPath.simple(path), true, false)
    }

    private fun listDirectories(transaction: Transaction): List<String> {
        val snapshots: List<String> = yt.cypress()
            .list(Optional.of(transaction.id), true, YPath.simple(BASE_DIR))
            .filter { node ->
                DIR_NAME_PATTERN.matches(node.stringValue()) &&
                    args.minTableName?.let { node.stringValue() >= it } ?: true
            }
            .map { it.value }
            .sorted()
            .dropLast(args.numberOfSnapshotsToKeep)

        return snapshots
    }

    private fun runInsideTransaction(args: Args, function: (Transaction) -> Unit) {
        val pinger = TransactionPinger(1)
        val transaction: Transaction = pinger.enablePinging(
            yt.transactions().startAndGet(
                Duration.ofMinutes(args.ytTransactionTimeoutMinutes)
            )
        )
        try {
            function.invoke(transaction)
            if (!args.dry) {
                logger.info("Committing transaction")
                transaction.commit()
            } else {
                logger.info("Aborting as dry-run enabled")
                transaction.cancel()
            }
        } catch (e: Exception) {
            logger.error(e)
            transaction.abort()
        }
    }
}

internal class Args {

    @Parameter(
        names = ["--min-snapshot-name"],
        description = "Min snapshot name (table names are compared lexicographically)"
    )
    var minTableName: String? = null

    @Parameter(
        names = ["--keep-count"], description = "Number of snapshots to keep"
    )
    var numberOfSnapshotsToKeep: Int = 90

    @Parameter(names = ["--dry-run"])
    var dry: Boolean = false

    @Parameter(names = ["--yt-transaction-timeout-minutes"], description = "YT transaction timeout in minutes")
    var ytTransactionTimeoutMinutes: Long = 5L

    @Parameter(names = ["--help"])
    var help: Boolean = false
}
