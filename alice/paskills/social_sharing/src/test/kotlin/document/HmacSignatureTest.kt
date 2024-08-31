package ru.yandex.alice.social.sharing.document

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import ru.yandex.alice.social.sharing.HmacSignature
import ru.yandex.alice.social.sharing.UTF_8
import ru.yandex.alice.social.sharing.splitQueryString
import java.net.URLDecoder
import java.util.*

class HmacSignatureTest {

    @Test
    fun testSignature() {
        val params = splitQueryString(
            "image_url=https%3A%2F%2Fyastatic.net%2Fs3%2Fhome-static%2F_%2F6%2FL%2FsRWLDRTog6jt1kgf7Kg3BQ71g.svg&skill_id=689f64c4-3134-42ba-8685-2b7cd8f06f4d&subtitle_text=%D0%9F%D0%BE%D0%B4%D0%B7%D0%B0%D0%B3%D0%BE%D0%BB%D0%BE%D0%B2%D0%BE%D0%BA&title_text=%D0%97%D0%B0%D0%B3%D0%BE%D0%BB%D0%BE%D0%B2%D0%BE%D0%BA&signature=GYHTRpU9hvGz/wokO56vwnyQgmEDHIJC8t7eWs%2BPmmA=",
        )
        val secretBase64 = "lHZAozC03YoUpq/67mx+vZaKChxva+L9zx/uj+8+t3t8nd3HQHHf80mfj8VKkU3kHq/3dTm7qfAaUGsDEHdyaW35hVUDpryOPUUJtmGwkTveBVJCnsSR5bLYKl8H2vfT"
        Assertions.assertTrue(
            HmacSignature.validate(
                params,
                secretBase64,
                Base64.getDecoder().decode(URLDecoder.decode(params[QueryParams.SIGNATURE], UTF_8))
            )
        )
    }

    @Test
    fun testInvalidSignature() {
        val params = splitQueryString(
            "image_url=https%3A%2F%2Fyastatic.net%2Fs3%2Fhome-static%2F_%2F6%2FL%2FsRWLDRTog6jt1kgf7Kg3BQ71g.svg&skill_id=689f64c4-3134-42ba-8685-2b7cd8f06f4d&subtitle_text=%D0%9F%D0%BE%D0%B4%D0%B7%D0%B0%D0%B3%D0%BE%D0%BB%D0%BE%D0%B2%D0%BE%D0%BA&title_text=%D0%97%D0%B0%D0%B3%D0%BE%D0%BB%D0%BE%D0%B2%D0%BE%D0%BA&signature=GYHTRpU9hvGz/wokO56vwnyQgmEDHIJC8t7eWs%2BPmmA=",
        )
        val secretBase64 = "ZqXCQmEryho5Ztla9CtKqXHmzhkduP3rPZtNNL8R5nJ6y0sho2ST8oc6v3PXs1n2+yiUG3fA6KBhZ7W9MJy8OkzrUiF8adkVwsT6eLpNjGQf4g36HovGLRqbw94mMPRV"
        Assertions.assertFalse(
            HmacSignature.validate(
                params,
                secretBase64,
                Base64.getDecoder().decode(URLDecoder.decode(params[QueryParams.SIGNATURE], UTF_8))
            )
        )
    }

}
