package ru.yandex.alice.paskill.dialogovo.utils;

import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.NoSuchAlgorithmException;
import java.security.Signature;
import java.security.SignatureException;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.X509EncodedKeySpec;

import com.google.common.io.BaseEncoding;
import org.junit.Assert;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertTrue;

class CryptoUtilTest {
    public static final String PUBLIC_KEY = "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwtHTyEcF+1rXFU2U5mSv/+b8wO/" +
            "wrhOMsjNrgMcDsywBSfu37CR24RJSrmOh14uY+Ziob0KIkCVBnEWrJkhAHONgfk6ahHWqUxltikOIvXkpeQXZImF/izG0UcGMl2FHf61" +
            "uwLkOcWTfXwgAE2D/N7JPgsH6MTXvE1iI1YN1X209I4iSYjw2Wrt1lsbYIS6t6PnZ4OZC/9ZqQRzypmR+sWMEiT99QLXqWWvqRN0Xn3Q" +
            "2V5N1Km+e1T9OIrFGvhEhSOOwbzIHKtsadyQHNe26TsWYgUnXFCsQo/EnOOM3eUMMnVNyAtTLBBtcUwvakJ76nxn+taMpqgy86WG2dMY" +
            "ciQIDAQAB";
    public static final String PRIVATE_KEY = "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDC0dPIRwX7WtcVTZTmZK" +
            "//5vzA7/CuE4yyM2uAxwOzLAFJ+7fsJHbhElKuY6HXi5j5mKhvQoiQJUGcRasmSEAc42B+TpqEdapTGW2KQ4i9eSl5BdkiYX+LMbRRwY" +
            "yXYUd/rW7AuQ5xZN9fCAATYP83sk+CwfoxNe8TWIjVg3VfbT0jiJJiPDZau3WWxtghLq3o+dng5kL/1mpBHPKmZH6xYwSJP31AtepZa+" +
            "pE3RefdDZXk3Uqb57VP04isUa+ESFI47BvMgcq2xp3JAc17bpOxZiBSdcUKxCj8Sc44zd5QwydU3IC1MsEG1xTC9qQnvqfGf61oymqDL" +
            "zpYbZ0xhyJAgMBAAECggEAPDYbjVeeGqxds0DSF07hMmcSkRLXQQXbwyuvOxLHKvYbw+DfXEV81F0UXr9+Qp7rfaDX1eMrT9mj6IeDuQ" +
            "Y1gngn14G1seCn7pz6RPRQa7bpwXS0QjL992g6QdZe/F5debGBfGD1fMhfXQCc/WBKIM2kU1ZeXB0+Ma14RP0nJoGTTgh4sRV/zQWN2n" +
            "gDw4H8+VfRr/pK6OFZqrGqQU43cARrmnz6X8iSr89M8SZrOH7UL3+idgk01iAlIb8t22Cw1Pm+YvkmHZuqxCKVq5fe74XLKXXM30jJ+m" +
            "+kAxHpOuWZ/KcskVGGNZY9QFL1uILuHX2oyTJX2/Mp1fmX2FsM7QKBgQDs4iFhSMbacKc/jnWpnkehHUxz/X0KPNVsejvECZ3fa6kKf9" +
            "CEFMMMynjKH1wksQht2OdJFV9BqkAwJ9gJu07dy92c7ldx0+EGQulqfaZltYXHCt9jTbemW9A2T6bmjj+47Hegwg6aJu1hBDnvqRxQag" +
            "NZavq6k8GOtDLX0wUA7wKBgQDSiq+drD5hqJE5OtYWxNrvlUHNlH6IQgihgz6ylnNSnWD0Eo0oGk42karPJqlzqL4Ns3eiC3zlOJVjzF" +
            "vkI9Pq3nMLN0BDBqWlimvn+2JVQDmAta+4u1H+pXvsKZs0brZE8CbqKHF3SUllu4lobW+l6uA+mk5uk+xxPhNOfIJKBwKBgQC8p7Nx6S" +
            "YFniZ05Z4rwhTF6bGTxtQorR5EUGz8ybPj1bhA1l5YcriMnUvpnWQjDKLr6Qz6FX1RrKeGMpVJ2tUKq2wJqYAW9WmHNQ70crFs2055oN" +
            "0cFy8l3IVMW89OWwfA3QLBEwvSFSKEW3tQtVwJZwTX1+1rXnWOk1DStgqMHQKBgQCzil7OF4cu4n/NDPZqfj6xVlfCByKL8bvId9Jsms" +
            "4HInv+Rx+mliAier/tXOvd3IUNzAB16FP+aO1EKI4oE8FwpXf3lYswXSe+7jasofut9VHUB5us7dizTc1KjAR1hv/Z8+1Le2efMJjtry" +
            "ozKjeuhofp+s5tj6luvF/ca2Lz2wKBgASjebJcv9vqP4rHjsndxMk2E+ospdEMkisVl/2CfigJOINcs3CHLy3TveoCjper6Fgv43Oouf" +
            "EENxTfHcOK+XlvhOJiE7E7RtOUKA5kzNiYlpYi9pZ/Pma/o5B1ANdC21CH0ik2wueCGSduvPrMLxph9+Vlh/aAag2yo1+Qjotx";

    @Test
    void testEncryption() {
        String key = "secret";

        String encrypted = CryptoUtils.encrypt(key, "Hello world");
        String decrypted = CryptoUtils.decrypt(key, encrypted);

        Assert.assertEquals("Hello world", decrypted);
    }

    @Test
    void testSignData()
            throws NoSuchAlgorithmException, InvalidKeySpecException, InvalidKeyException, SignatureException {
        String key = "secret";

        String encryptKey = CryptoUtils.signData(PRIVATE_KEY, key);

        Signature sign = Signature.getInstance("SHA256withRSA");
        sign.initVerify(KeyFactory.getInstance("RSA").generatePublic(
                new X509EncodedKeySpec(
                        BaseEncoding.base64().decode(PUBLIC_KEY)
                )
        ));
        sign.update(key.getBytes());
        assertTrue(sign.verify(BaseEncoding.base64().decode(encryptKey)));
    }
}
