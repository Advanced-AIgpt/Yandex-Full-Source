package ru.yandex.alice.nlu.libs.fstnormalizer;

import java.util.Objects;
import javax.annotation.ParametersAreNonnullByDefault;
import javax.annotation.Nonnull;

/**
* Binding for fst_normalizer
*/
@ParametersAreNonnullByDefault
public class FstNormalizer {
    
    static {
        System.loadLibrary("fstnormalizer_java");
    }

    /**
    * 
    */
    @Nonnull
    public String normalize(Lang lang, String text) {
        Objects.requireNonNull(lang, "lang argument is null");
        Objects.requireNonNull(text, "text argument is null");

        return normalize(lang.getCode(), text);
    }

    private native String normalize(String lang, String text);
}