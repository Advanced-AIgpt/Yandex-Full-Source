package ru.yandex.alice.paskills.yt_merger;

import java.time.Duration;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.inside.yt.kosher.transactions.Transaction;

public class TransactionPinger implements Runnable {

    private static final Logger logger = LogManager.getLogger("YtMergerImpl");

    private final Transaction transaction;
    private final Duration sleepInterval;
    private volatile boolean isRunning;

    public TransactionPinger(Transaction transaction, Duration sleepInterval) {
        this.transaction = transaction;
        this.sleepInterval = sleepInterval;
        this.isRunning = true;
    }

    @Override
    public void run() {
        logger.info("Starting transaction pinger");
        while (isRunning) {
            try {
                this.transaction.ping();
                Thread.sleep(sleepInterval.toMillis());
            } catch (InterruptedException e) {
                logger.error("Pinger thread was interrupted", e);
                isRunning = false;
            }
        }
    }

    public void stop() {
        logger.info("Stopping transaction pinger");
        isRunning = false;
    }
}
