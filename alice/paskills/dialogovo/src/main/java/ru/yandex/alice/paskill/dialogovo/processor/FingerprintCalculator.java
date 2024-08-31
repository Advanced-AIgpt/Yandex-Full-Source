package ru.yandex.alice.paskill.dialogovo.processor;

import java.nio.charset.StandardCharsets;
import java.util.Base64;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.providers.UserProvider;

@Component
class FingerprintCalculator {

    private final ObjectMapper objectMapper;
    private final UserProvider userProvider;

    FingerprintCalculator(ObjectMapper objectMapper, UserProvider userProvider) {
        this.objectMapper = objectMapper;
        this.userProvider = userProvider;
    }

    // see PASKILLS-7922
    // VTB specific implementation of fingerprint
    String calculateVtbFingerprint(SkillInfo skill, String applicationId) {
        String fingerprintJson = objectMapper.createObjectNode()
                .put("application_id",
                        userProvider.getApplicationId(skill, applicationId))
                .toString();
        return Base64.getEncoder()
                .encodeToString(fingerprintJson.getBytes(StandardCharsets.UTF_8));
    }
}
