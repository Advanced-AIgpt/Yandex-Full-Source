package ru.yandex.alice.paskill.dialogovo.service;

import java.nio.charset.StandardCharsets;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

import com.google.common.io.BaseEncoding;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskill.dialogovo.config.RedirectApiConfig;
import ru.yandex.alice.paskill.dialogovo.utils.EncodingUtil;

// https://wiki.yandex-team.ru/users/hulan/redirector-and-infected-services/
@Component
class RedirectServiceImpl implements RedirectService {
    private static final String HMAC_MD5 = "HmacMD5";

    private final RedirectApiConfig config;
    private final SecretKeySpec secretKey;

    RedirectServiceImpl(RedirectApiConfig config) {
        this.config = config;
        this.secretKey = new SecretKeySpec(config.getKey().getBytes(StandardCharsets.UTF_8), HMAC_MD5);
    }

    @Override
    public String getSafeUrl(String url) {
        var clientId = config.getClientId();
        var sign = makeSign("client=" + clientId + "&url=" + url + clientId);

        return UriComponentsBuilder.fromUri(config.getUrl())
                .queryParam("client", clientId)
                .queryParam("sign", sign)
                .queryParam("url", EncodingUtil.encodeURIComponent(url))
                .build()
                .toUriString();
    }

    private String makeSign(String str) {
        try {
            var hmacMD5 = Mac.getInstance(HMAC_MD5);
            hmacMD5.init(secretKey);

            return BaseEncoding
                    .base16()
                    .lowerCase()
                    .encode(hmacMD5.doFinal(str.getBytes(StandardCharsets.UTF_8)));
        } catch (NoSuchAlgorithmException | InvalidKeyException e) {
            throw new RuntimeException(e);
        }
    }
}
