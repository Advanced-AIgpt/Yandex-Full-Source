package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.function.BiPredicate;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class VerboseBiPredicate<T, U> implements BiPredicate<T, U> {
    private static final Logger logger = LogManager.getLogger();

    private final BiPredicate<T, U> delegate;
    private final String predicateName;

    private VerboseBiPredicate(String predicateName, BiPredicate<T, U> delegate) {
        this.delegate = delegate;
        this.predicateName = predicateName;
    }

    public static <T, U> BiPredicate<T, U> logMatch(String predicateName, BiPredicate<T, U> delegate) {
        return new VerboseBiPredicate<>(predicateName, delegate);
    }

    @Override
    public boolean test(T t, U u) {
        return logMatch(predicateName, delegate.test(t, u));
    }

    private boolean logMatch(String predicateName, boolean test) {
        if (test) {
            logger.info("Predicate [" + predicateName + "] does match");
        }
        return test;
    }
}
