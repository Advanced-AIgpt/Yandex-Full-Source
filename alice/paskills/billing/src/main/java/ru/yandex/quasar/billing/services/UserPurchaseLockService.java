package ru.yandex.quasar.billing.services;

import java.text.MessageFormat;
import java.time.Instant;
import java.util.function.Supplier;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.dao.UserPurchaseLockDAO;

@Component
public class UserPurchaseLockService {

    private static final Logger log = LogManager.getLogger();
    // expiration time in seconds after which a lock should be removed by the job
    private static final long LOCK_EXPIRATION_TIME_SECONDS = 30;
    private final UserPurchaseLockDAO userPurchaseLockDao;

    public UserPurchaseLockService(UserPurchaseLockDAO userPurchaseLockDao) {
        this.userPurchaseLockDao = userPurchaseLockDao;
    }

    /**
     * Execute action inside a UserPurchaseLock
     *
     * @param uid          userId
     * @param providerName provider name
     * @param action       action to be executed
     * @return action
     * @throws UserPurchaseLockException if cant acquire lock
     */
    public <T> T processWithLock(Long uid, String providerName, Supplier<T> action) throws UserPurchaseLockException {
        // use locks as WebView might send several requests for
        boolean lockAcquired = userPurchaseLockDao.acquireLock(uid, providerName);
        if (!lockAcquired) {
            throw new UserPurchaseLockException(uid, providerName);
        }

        try {
            return action.get();
        } finally {
            userPurchaseLockDao.removeLock(uid, providerName);
        }
    }

    public void processWithLock(Long uid, String providerName, Runnable action) throws UserPurchaseLockException {
        processWithLock(uid, providerName, () -> {
            action.run();
            return 0;
        });
    }

    /**
     * Removes locks older than {@value #LOCK_EXPIRATION_TIME_SECONDS} minutes
     * If everything goes ok, there should very few records in the table as locks are removed automatically
     * Full table scan is optimal in such case as don't pay for index maintenance on DML and performance
     * of the delete is not important
     */
    @Scheduled(fixedDelay = LOCK_EXPIRATION_TIME_SECONDS * 1000, initialDelay = LOCK_EXPIRATION_TIME_SECONDS * 1000)
    void removeExpiredLocks() {
        int deleted =
                userPurchaseLockDao.removeAcuiredBeforeMoment(Instant.now().minusSeconds(LOCK_EXPIRATION_TIME_SECONDS));

        if (deleted > 0) {
            // locks should be deleted automatically
            log.warn("UserPurchaseLock rows deleted: {}", deleted);
        }
    }

    public static class UserPurchaseLockException extends RuntimeException {
        private final String providerName;
        private final Long uid;

        UserPurchaseLockException(Long uid, String providerName) {
            super(MessageFormat.format("Purchase operation is already in progress for uid = {0} and providerName = " +
                    "{1}", uid, providerName));
            this.providerName = providerName;
            this.uid = uid;
        }

        public String getProviderName() {
            return providerName;
        }

        public Long getUid() {
            return uid;
        }
    }
}
