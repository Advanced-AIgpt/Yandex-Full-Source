package ru.yandex.alice.paskill.dialogovo.utils;

import java.nio.charset.StandardCharsets;
import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.Signature;
import java.security.SignatureException;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.util.Arrays;
import java.util.Base64;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;

import com.google.common.io.BaseEncoding;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class CryptoUtils {
    private static final Logger logger = LogManager.getLogger();

    private static final String ALGORITHM = "AES";

    private CryptoUtils() {
        throw new UnsupportedOperationException();
    }

    public static String encrypt(String secret, String data) {
        try {
            Cipher cipher = Cipher.getInstance(ALGORITHM);

            byte[] keyBytes = Arrays.copyOf(secret.getBytes(StandardCharsets.UTF_8), 16);

            SecretKeySpec secretKeySpec = new SecretKeySpec(keyBytes, ALGORITHM);
            cipher.init(Cipher.ENCRYPT_MODE, secretKeySpec);

            byte[] encrypted = cipher.doFinal(data.getBytes(StandardCharsets.UTF_8));

            return Base64.getEncoder().encodeToString(encrypted);
        } catch (Exception e) {
            logger.error("Exception while encrypting", e);
            throw new RuntimeException(e);
        }
    }

    public static String decrypt(String secret, String data) {
        try {
            Cipher cipher = Cipher.getInstance(ALGORITHM);

            byte[] keyBytes = Arrays.copyOf(secret.getBytes(StandardCharsets.UTF_8), 16);

            SecretKeySpec secretKeySpec = new SecretKeySpec(keyBytes, ALGORITHM);
            cipher.init(Cipher.DECRYPT_MODE, secretKeySpec);

            byte[] decrypted = cipher.doFinal(Base64.getDecoder().decode(data));
            return new String(decrypted, StandardCharsets.UTF_8);
        } catch (Exception e) {
            logger.error("Exception while decrypting", e);
            throw new RuntimeException(e);
        }
    }

    /**
     * generate signature for the string as base64 representation
     */
    public static String signData(String privateKeyString, String dataToSign) {
        PKCS8EncodedKeySpec key = new PKCS8EncodedKeySpec(BaseEncoding.base64().decode(privateKeyString));

        try {
            PrivateKey privateKey = KeyFactory.getInstance("RSA").generatePrivate(key);
            Signature signature = Signature.getInstance("SHA256withRSA");
            signature.initSign(privateKey);

            signature.update(dataToSign.getBytes());
            byte[] sign = signature.sign();

            return BaseEncoding.base64().encode(sign);
        } catch (InvalidKeySpecException | NoSuchAlgorithmException | InvalidKeyException | SignatureException e) {
            throw new RuntimeException(e);
        }
    }
}
