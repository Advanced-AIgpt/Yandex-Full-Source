package ru.yandex.quasar.billing.util;

import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;

import javax.annotation.Nullable;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.quasar.billing.exception.InternalErrorException;

import static java.nio.charset.StandardCharsets.US_ASCII;

/**
 * A helper class to validate csrf tokens.
 * <p>
 * This class is thread-safe.
 */
public class CSRFHelper {
    private static final String HMAC_SHA1_ALGORITHM = "HmacSHA1";

    private final SecretKeySpec key;

    private static final Logger logger = LogManager.getLogger();

    private final ThreadLocal<Mac> macThreadLocal = new ThreadLocal<>() {
        @Override
        protected Mac initialValue() {
            try {
                Mac mac = Mac.getInstance(HMAC_SHA1_ALGORITHM);
                mac.init(key);
                return mac;
            } catch (NoSuchAlgorithmException e) {
                throw new RuntimeException(e);  // java is required to support this, see javax.crypto.Mac's javadoc
            } catch (InvalidKeyException e) {
                throw new InternalErrorException("Invalid key for CSRF token generation", e);  // should not happen
            }
        }
    };

    public CSRFHelper(byte[] key) {
        this.key = new SecretKeySpec(key, HMAC_SHA1_ALGORITHM);
    }

    /**
     * Sign a message
     *
     * @param bytes message to sign
     * @return hex string for signed bytes
     */
    public String sign(byte[] bytes) {
        return String.valueOf(Hex.encodeHex(macThreadLocal.get().doFinal(bytes)));
    }

    /**
     * Generate a new CSRF token as it should be sent in the HTTP request.
     */
    public String generate(String yandexUID, String passportID, long unixTimeSeconds) {
        return String.format("%s:%s",
                // explicitly using ASCII
                sign(String.format("%s:%s:%d", passportID, yandexUID, unixTimeSeconds).getBytes(US_ASCII)),
                unixTimeSeconds
        );
    }

    /**
     * Check if given token is still valid (with regard to current time).
     */
    public boolean isValid(@Nullable String token, @Nullable String yandexUID, @Nullable String passportID) {
        if (token == null) {
            logger.warn("CSRF token is null");
            return false;
        }

        String[] tokenParts = token.split(":");

        if (tokenParts.length != 2) {
            logger.warn("CSRF token length is invalid: " + tokenParts.length);
            return false;
        }

        try {
            long tokenTimeSeconds = Long.parseLong(tokenParts[1]);
            long now = System.currentTimeMillis() / 1000;

            if (tokenTimeSeconds > now) {
                logger.warn("CSRF token is from future");
                return false;  // token from future
            }

            // token is valid for 24 hours
            if (tokenTimeSeconds + 60 * 60 * 24 <= now) {
                logger.warn("CSRF token is expired. token tokenTimeSeconds: " + tokenTimeSeconds);
                return false;  // token expired
            }

            if (yandexUID == null) {
                logger.warn("CSRF token is valid but yandexUID is null");
                return false;
            } else if (passportID == null) {
                logger.warn("CSRF token is valid but passportID is null");
                return false;
            }

            String expected = generate(yandexUID, passportID, tokenTimeSeconds);
            boolean equals = token.equals(expected);
            if (!equals) {
                logger.warn("CSRF token is " + token + " but has to be " + expected);
            }
            return equals;
        } catch (NumberFormatException e) {
            return false;
        }
    }
}
