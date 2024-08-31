package ru.yandex.alice.paskills.my_alice.blackbox;

import java.math.BigInteger;

import com.google.gson.annotations.SerializedName;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NonNull;
import org.springframework.lang.Nullable;

@Data
@NonNull
@AllArgsConstructor
public class User {
    private final BigInteger uid;
    @Nullable
    private final String login;
    @Nullable
    private final String displayName;
    private final Avatar avatar;
    private final Attributes attributes;

    @Data
    @NonNull
    @AllArgsConstructor
    public static class Avatar {
        static final Avatar EMPTY = new Avatar(null, true);

        @Nullable
        @SerializedName("default")
        private final String defaultId;
        private final Boolean empty;
    }

    @Data
    @NonNull
    @AllArgsConstructor
    public static class Attributes {
        static final Attributes DEFAULT = new Attributes(false, false, null);

        private final boolean havePlus;
        private final boolean betaTester;
        @Nullable
        private final String yaStaffLogin;
    }
}
