package ru.yandex.quasar.billing.util;

import java.time.Instant;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class CSRFHelperTest {
    private CSRFHelper helper = new CSRFHelper("my_huge_super_key".getBytes());

    private String yaUid = "1234567890";
    private String passportId = "09876543210987654321";

    @Test
    public void smokeSign() {
        assertEquals("bcf6ecb117231221f7f6eb35a4d60cac63d534aa",
                new CSRFHelper("secret".getBytes()).sign("abcdef".getBytes()));
    }

    @Test
    public void smokeGenerate() {
        assertEquals("ae33c83bbf9de9710cc87887fd6458b7e1c84d98:1528928016",
                helper.generate(
                        "6447714881391768016",
                        "123142323",
                        1528928016
                )
        );
    }

    @Test
    public void smokeValid() {
        long now = Instant.now().toEpochMilli() / 1000;

        String token = helper.generate(yaUid, passportId, now);

        assertTrue(helper.isValid(token, yaUid, passportId));
    }

    @Test
    public void smokeInvalidFromFuture() {
        long tomorrow = Instant.now().toEpochMilli() / 1000 + 60 * 60 * 24;

        String token = helper.generate(yaUid, passportId, tomorrow);

        assertFalse(helper.isValid(token, yaUid, passportId));
    }

    @Test
    public void smokeInvalidExpired() {
        long aDayAgo = Instant.now().toEpochMilli() / 1000 - 60 * 60 * 24 - 1;

        String token = helper.generate(yaUid, passportId, aDayAgo);

        assertFalse(helper.isValid(token, yaUid, passportId));
    }

    @Test
    public void smokeInvalidTamperedWith() {
        long now = Instant.now().toEpochMilli() / 1000;

        String token = "1" + helper.generate(yaUid, passportId, now);

        assertFalse(helper.isValid(token, yaUid, passportId));
    }

    @Test
    public void smokeInvalidTamperedWith2() {
        long now = Instant.now().toEpochMilli() / 1000;

        String validToken = helper.generate(yaUid, passportId, now);

        String token = (validToken.charAt(0) == 'b' ? 'c' : 'b') + validToken.substring(1);

        assertFalse(helper.isValid(token, yaUid, passportId));
    }

    @Test
    public void smokeInvalidBadFormat() {
        assertFalse(helper.isValid(":", yaUid, passportId));
        assertFalse(helper.isValid("abcdef", yaUid, passportId));
        assertFalse(helper.isValid(null, yaUid, passportId));
        assertFalse(helper.isValid("", yaUid, passportId));
    }
}
