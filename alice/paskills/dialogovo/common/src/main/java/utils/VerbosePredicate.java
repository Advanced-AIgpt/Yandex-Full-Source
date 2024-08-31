package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.function.Predicate;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class VerbosePredicate<T> implements Predicate<T> {
    private static final Logger logger = LogManager.getLogger();

    private final Predicate<T> delegate;
    private final String predicateName;

    private VerbosePredicate(String predicateName, Predicate<T> delegate) {
        this.delegate = delegate;
        this.predicateName = predicateName;
    }

    public static <T> Predicate<T> logMismatch(String predicateName, Predicate<T> delegate) {
        return new VerbosePredicate<>(predicateName, delegate);
    }

    @Override
    public boolean test(T t) {
        return logMismatch(predicateName, delegate.test(t));
    }

    private boolean logMismatch(String predicateName, boolean test) {
        if (!test) {
            logger.info("Predicate [" + predicateName + "] does not match");
        }
        return test;
    }
}
