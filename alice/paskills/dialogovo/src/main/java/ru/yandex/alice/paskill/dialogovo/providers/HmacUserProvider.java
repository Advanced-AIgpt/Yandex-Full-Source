package ru.yandex.alice.paskill.dialogovo.providers;

import java.nio.charset.StandardCharsets;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.util.Optional;

import javax.annotation.Nullable;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

import com.google.common.io.BaseEncoding;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;

@Component
public class HmacUserProvider implements UserProvider {

    private String hashUserId(String salt, String id) {
        try {
            Mac hmacSHA256 = Mac.getInstance("HmacSHA256");
            SecretKeySpec secretKey = new SecretKeySpec(salt.getBytes(StandardCharsets.UTF_8), "HmacSHA256");
            hmacSHA256.init(secretKey);
            return BaseEncoding.base16().upperCase().encode(hmacSHA256.doFinal(id.getBytes(StandardCharsets.UTF_8)));
        } catch (NoSuchAlgorithmException | InvalidKeyException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public String getApplicationId(SkillInfo skill, String uuid) {
        return hashUserId(skill.getSalt(), uuid);
    }

    @Override
    public Optional<String> getPersistentUserId(SkillInfo skill, @Nullable String userId) {
        if (userId != null) {
            return Optional.of(hashUserId(skill.getPersistentUserIdSalt(), userId));
        } else {
            return Optional.empty();
        }
    }
}
